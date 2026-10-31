#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-10-31: skip only the five-column header, not a member named name.
"""Check documented browser/NW.js inventory against an offline runtime dump.

See ARCHITECTURE.md and docs/compat-surface.md. Paths are repository-relative;
this read-only CLI runs synchronously and never evaluates game or shim code.
"""
import argparse
from pathlib import Path
import sys


def cell(value):
    """Unwrap Markdown code spans so documented paths retain their literal meaning."""
    value = value.strip()
    return value[1:-1] if value.startswith('`') and value.endswith('`') else value


def shim_path(value):
    """Resolve loader basenames to repository shim paths, preserving explicit paths."""
    path = Path(value)
    if len(path.parts) == 1 and (Path('src/shims') / path).is_file():
        return (Path('src/shims') / path).as_posix()
    return path.as_posix()


def table_rows(path):
    """Yield five-column inventory records; reject malformed rows with line context."""
    for number, line in enumerate(path.read_text().splitlines(), 1):
        if not line.startswith('|'):
            continue
        fields = [cell(part) for part in line.strip().strip('|').split('|')]
        if fields == ['name', 'status', 'shim file', 'test', 'notes'] or set(fields[0]) <= set('-: '):
            continue
        if len(fields) != 5 or fields[1] not in {'real', 'partial', 'stub', 'missing'}:
            raise ValueError(f'{path}:{number}: invalid compatibility row')
        yield fields


def check(dump, shims):
    """Return diagnostics for selected (name, shim) pairs and their file contracts."""
    documented, runtime, errors = set(), set(), []
    for name, status, shim, test, _notes in table_rows(Path('docs/compat-surface.md')):
        if shims and shim_path(shim) not in shims:
            continue
        shim = shim_path(shim)
        pair = (name, shim)
        if pair in documented:
            errors.append(f'duplicate table row: {name} ({shim})')
        documented.add(pair)
        if not Path(shim).is_file():
            errors.append(f'{name}: missing shim file {shim}')
        if status in {'real', 'partial'} and not Path(test).is_file():
            errors.append(f'{name}: missing test file {test}')
    for number, line in enumerate(dump.read_text().splitlines(), 1):
        if not line:
            continue
        fields = line.split('\t')
        if len(fields) != 3:
            raise ValueError(f'{dump}:{number}: expected name, kind and shim TSV fields')
        name, _kind, shim = fields
        shim = shim_path(shim)
        if not shims or shim in shims:
            runtime.add((name, shim))
    for name, shim in sorted(runtime - documented):
        errors.append(f'dump member missing from table: {name} ({shim})')
    for name, shim in sorted(documented - runtime):
        errors.append(f'table member missing from dump: {name} ({shim})')
    return errors, len(documented)


def main():
    """Expose the offline gate; malformed inputs and mismatches exit nonzero."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dump_file', type=Path)
    parser.add_argument('--shim', action='append', nargs='+', default=[], metavar='FILE')
    args = parser.parse_args()
    try:
        errors, count = check(args.dump_file, {shim_path(p) for group in args.shim for p in group})
    except (OSError, ValueError) as error:
        print(f'compat-check: {error}', file=sys.stderr)
        return 1
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    print(f'compat-check: {count} rows match dump')
    return 0


if __name__ == '__main__':
    sys.exit(main())
