# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Local upstream extraction policy, described in docs/UPSTREAM.md.

Never access remotes; validate before publishing a local branch. Temporary
worktrees are owned here and removed on success and failure.
"""
import argparse
import pathlib
import re
import subprocess
import tempfile
import sys


def git(repo, *args):
    """Run local Git in repo; return bytes, raising on any failed operation."""
    return subprocess.check_output(['git', '-C', str(repo), *args])


def allowed_paths(identifier):
    """Read exact row paths from the shipped tracker; reject unknown IDs."""
    tracker = pathlib.Path(__file__).resolve().parents[2] / 'docs/UPSTREAM.md'
    for line in tracker.read_text().splitlines():
        cells = [cell.strip() for cell in line.split('|')]
        if len(cells) > 3 and cells[1] == identifier:
            return set(re.findall(r'`([^`]+)`', cells[3]))
    raise ValueError('unknown upstream PR ID')


def validate(repo, commit, allowed):
    """Reject merges, extra paths and fork headers before replay; return full SHA."""
    sha = git(repo, 'rev-parse', '--verify', commit + '^{commit}').decode().strip()
    parents = git(repo, 'rev-list', '--parents', '-n', '1', sha).split()
    if len(parents) != 2:
        raise ValueError('only single-parent commits can be extracted')
    paths = git(repo, 'diff-tree', '--no-commit-id', '--name-only',
                '--no-renames', '-r', '-z', sha).split(b'\0')
    for raw in filter(None, paths):
        path = raw.decode('utf-8')
        if path not in allowed:
            raise ValueError('unlisted path: ' + path)
        entries = git(repo, 'ls-tree', sha, '--', path)
        if entries and b'The smzuggler contributors' in git(repo, 'show', sha + ':' + path):
            raise ValueError('smzuggler copyright in ' + path)
    return sha


def build(work):
    """Build QuickJS offline using the local cache; fail if cache or tests fail."""
    import os
    deps = pathlib.Path(os.environ.get('SMZ_DEPS_DIR',
                                      str(pathlib.Path.home() / '.cache/smzuggler-dev/deps')))
    # Match the forge build's documented QuickJS/GCC compatibility option.
    args = ['cmake', '-S', str(work), '-B', str(work / 'build'),
            '-DRMMZ_BUILD_TESTS=ON', '-DFETCHCONTENT_FULLY_DISCONNECTED=ON',
            '-DCMAKE_C_FLAGS=-Wno-error=discarded-qualifiers']
    for name in ('sdl3', 'quickjs', 'soloud', 'glslang', 'spirv_cross', 'sdl_shadercross'):
        source = deps / (name + '-src')
        if not source.is_dir():
            raise ValueError('missing offline dependency: ' + name)
        args.append('-DFETCHCONTENT_SOURCE_DIR_' + name.upper() + '=' + str(source))
    subprocess.run(args, check=True)
    subprocess.run(['cmake', '--build', str(work / 'build')], check=True)
    subprocess.run(['ctest', '--test-dir', str(work / 'build'), '--output-on-failure'], check=True)


def main():
    """Parse ID, safe slug and ordered commits; publish only after successful replay."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', action='store_true')
    parser.add_argument('id', choices=['U' + str(n) for n in range(1, 15)])
    parser.add_argument('slug')
    parser.add_argument('commits', nargs='+')
    args = parser.parse_args()
    if not re.fullmatch(r'[a-z0-9]+(?:-[a-z0-9]+)*', args.slug):
        raise ValueError('slug must contain lowercase words separated by hyphens')
    repo = pathlib.Path(git('.', 'rev-parse', '--show-toplevel').decode().strip())
    branch = 'up/' + args.id + '-' + args.slug
    if git(repo, 'branch', '--list', branch).strip():
        raise ValueError('branch already exists: ' + branch)
    allowed = allowed_paths(args.id)
    commits = [validate(repo, commit, allowed) for commit in args.commits]
    with tempfile.TemporaryDirectory(prefix='upstream-cut-') as temp:
        work = pathlib.Path(temp) / 'tree'
        git(repo, 'worktree', 'add', '--detach', str(work), 'upstream-main')
        try:
            for commit in commits:
                git(work, '-c', 'core.hooksPath=/dev/null', 'cherry-pick', '-x', commit)
                validate(work, 'HEAD', allowed)
            if args.build:
                build(work)
            git(repo, 'branch', branch, git(work, 'rev-parse', 'HEAD').decode().strip())
        finally:
            git(repo, 'worktree', 'remove', '--force', str(work))
    print('Created ' + branch)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, subprocess.CalledProcessError) as error:
        print('cut_up_branch: ' + str(error), file=sys.stderr)
        sys.exit(1)
