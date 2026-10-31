#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Enforce same-change documentation using docs/docs-map.toml.

This read-only repository gate supports spec section 15.7, outside the runtime.
CI owns label authorization; never infer authorization from PR prose alone.
Git paths are NUL-delimited and compared from the base to the working tree.
"""

import argparse
import fnmatch
import os
from pathlib import Path
import re
import subprocess
import sys
import tomllib


def git(root, *args):
    """Return NUL-separated git paths; command failures propagate to the CLI."""
    result = subprocess.run(['git', '-C', str(root), *args], check=True,
                            capture_output=True)
    return set(os.fsdecode(path) for path in result.stdout.split(b'\0') if path)


def load_areas(root):
    """Validate the owned mapping data; malformed rules fail closed with context."""
    with (root / 'docs/docs-map.toml').open('rb') as stream:
        areas = tomllib.load(stream)['areas']
    if not isinstance(areas, list):
        raise ValueError('areas must be an array of tables')
    for area in areas:
        if not isinstance(area.get('path'), str) or not area['path']:
            raise ValueError('each area needs a non-empty path glob')
        docs = area.get('docs')
        if not isinstance(docs, list) or not docs:
            raise ValueError('each area needs a non-empty docs array')
        for doc in docs:
            if (not isinstance(doc, str) or not doc or
                    Path(doc).is_absolute() or '..' in Path(doc).parts):
                raise ValueError('docs must contain repository-relative paths')
    return areas


def justified():
    """Accept a CI label only with a filled justification when PR_BODY is supplied."""
    if os.environ.get('NO_DOC_CHANGE') != '1':
        return False
    if 'PR_BODY' not in os.environ:
        return True
    body = re.sub(r'<!--.*?-->', '', os.environ['PR_BODY'], flags=re.S)
    return bool(re.search(r'^[ \t]*No-doc-change justification:[ \t]*([^\s\n][^\n]*)$',
                          body, re.I | re.M))


def check(root, base, areas):
    """Return diagnostics for uncovered directories and unsatisfied changed areas."""
    changed = git(root, 'diff', '--name-only', '--no-renames', '-z', base, '--')
    changed |= git(root, 'ls-files', '--others', '--exclude-standard', '-z')
    tracked = git(root, 'ls-files', '-z') | changed
    old = git(root, 'ls-tree', '-r', '--name-only', '-z', base, '--', 'src')
    old_modules = {path.split('/')[1] for path in old if path.count('/') >= 2}
    errors = []
    directories = {str(parent) for path in tracked
                   for parent in Path(path).parents
                   if len(parent.parts) >= 2 and parent.parts[0] in ('src', 'tools')}
    for directory in sorted(directories):
        if not any(fnmatch.fnmatchcase(directory + '/__file__', area['path'])
                   for area in areas):
            errors.append('unmapped directory: ' + directory +
                          '; add an area to docs/docs-map.toml')
    exempt = justified()
    if os.environ.get('NO_DOC_CHANGE') == '1' and not exempt:
        errors.append('NO_DOC_CHANGE requires a non-empty '
                      'No-doc-change justification: line in PR_BODY')
    for path in sorted(changed):
        obligations = [area['docs'] for area in areas
                       if fnmatch.fnmatchcase(path, area['path'])]
        parts = path.split('/')
        if len(parts) >= 3 and parts[0] == 'src' and parts[1] not in old_modules:
            obligations.append(['ARCHITECTURE.md'])
        for docs in obligations:
            if exempt or any(doc in changed for doc in docs):
                continue
            expected = [doc + (' (to be created)' if not (root / doc).exists()
                               else '') for doc in docs]
            errors.append(path + ': change at least one of: ' + ', '.join(expected))
    return errors


def main():
    """Run on the caller's repository; return 1 for gate failures, 2 for tool errors."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', required=True, help='Git revision to compare against')
    args = parser.parse_args()
    try:
        result = subprocess.run(['git', 'rev-parse', '--show-toplevel'],
                                check=True, capture_output=True, text=True)
        root = Path(result.stdout.strip())
        errors = check(root, args.base, load_areas(root))
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError) as error:
        print('docs-touched: cannot check mapping or git revision: ' + str(error),
              file=sys.stderr)
        return 2
    for error in errors:
        print('docs-touched: ' + error)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
