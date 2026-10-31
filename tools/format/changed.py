# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Git range plumbing for repository-local format checks (docs/style.md).
# Callers own returned data; run synchronously from any directory in the repo.
import pathlib
import re
import subprocess


def git(*args):
    """Return Git output or raise with context; borrow arguments, retain no state."""
    return subprocess.check_output(['git', *args], stderr=subprocess.PIPE)


def changed(base, suffixes):
    """Yield existing paths and new-side line sets versus base, including staged edits.

    Paths are relative to the repository root; deleted lines cannot be formatted.
    Git failures propagate to the CLI; no objects or caller inputs are retained.
    """
    root = pathlib.Path(git('rev-parse', '--show-toplevel').decode().strip())
    names = git('-C', str(root), 'diff', '--name-status', '-z', '--find-renames',
                '--diff-filter=ACMR', base, '--')
    records = iter(names.split(b'\0'))
    for status in records:
        if not status:
            continue
        old_name = next(records).decode('utf-8')
        name = next(records).decode('utf-8') if status.startswith((b'R', b'C')) else old_name
        if pathlib.Path(name).suffix not in suffixes:
            continue
        patch = git('-C', str(root), 'diff', '--no-ext-diff', '--no-textconv', '--unified=0', '--find-renames',
                    base, '--', old_name, name).decode('utf-8')
        lines = set()
        for start, count in re.findall(r'^@@ .*?\+(\d+)(?:,(\d+))? @@', patch, re.M):
            lines.update(range(int(start), int(start) + int(count or '1')))
        if lines:
            yield root / name, name, lines
