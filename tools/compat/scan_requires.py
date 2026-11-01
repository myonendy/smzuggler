#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""List require() and process.* uses in a game's js/plugins/*.js.

Why this exists: spec §5.3 (SI-10) requires a local re-scan of the 57 plugins
before unknown require() throws MODULE_NOT_FOUND. The scanner reads the test
copy named by SMZUGGLER_GAME_DIR and prints plugin basenames and counts only,
so reports never carry game source or install paths. See docs/shims.md.

Rules a reader must not break:
  * Output is basename, require count, process count. Never print a match,
    a module name, a path or file contents.
  * Steam/Proton locations are refused. Missing env or js/plugins is exit 2.
  * Comments are not uses. The scan is the calling thread; it opens files
    only to count and closes them before printing.

Exit status: 0 on success, 2 on a usage or setup error.
"""
from __future__ import annotations

import os
from pathlib import Path
import sys

STEAM_PARTS = {'steam', '.steam', 'steamapps', 'steamlibrary', 'compatdata', 'pfx'}
REQUIRE = 'require'
PROCESS = 'process'


def steam_path(parts):
    """True when any component names a Steam or Proton location.

    parts is an iterable of path components from both the given and resolved
    path. Checked on the caller thread; never throws.
    """
    return any(p.lower() in STEAM_PARTS or 'proton' in p.lower() for p in parts)


def strip_comments(source):
    """Return SOURCE with JS comments replaced by spaces, strings kept.

    Why: comment text must not count as require() or process.* uses. Quotes
    inside comments stay comments; comment markers inside strings stay
    strings. Newlines are preserved so callers can still reason about lines.
    source is a Unicode string; the result is a new string. Never throws.
    """
    out = []
    i = 0
    n = len(source)
    while i < n:
        c = source[i]
        nxt = source[i + 1] if i + 1 < n else ''
        if c in ('"', "'"):
            out.append(c)
            i += 1
            while i < n:
                out.append(source[i])
                if source[i] == '\\' and i + 1 < n:
                    out.append(source[i + 1])
                    i += 2
                    continue
                if source[i] == c:
                    i += 1
                    break
                i += 1
            continue
        if c == '/' and nxt == '/':
            while i < n and source[i] != '\n':
                out.append(' ')
                i += 1
            continue
        if c == '/' and nxt == '*':
            out.append('  ')
            i += 2
            while i < n:
                if source[i] == '*' and i + 1 < n and source[i + 1] == '/':
                    out.append('  ')
                    i += 2
                    break
                out.append('\n' if source[i] == '\n' else ' ')
                i += 1
            continue
        out.append(c)
        i += 1
    return ''.join(out)


def count_uses(source):
    """Return (require_count, process_count) for stripped SOURCE.

    require( and process. are identifier uses, not matches inside longer
    names. source is already comment-stripped. Returns two ints; never throws.
    """
    requires = 0
    processes = 0
    i = 0
    n = len(source)
    while i < n:
        if source.startswith(REQUIRE, i) and (i == 0 or not _ident(source[i - 1])):
            end = i + len(REQUIRE)
            if end >= n or not _ident(source[end]):
                j = end
                while j < n and source[j] in ' \t\r\n':
                    j += 1
                if j < n and source[j] == '(':
                    requires += 1
                    i = j + 1
                    continue
        if source.startswith(PROCESS, i) and (i == 0 or not _ident(source[i - 1])):
            end = i + len(PROCESS)
            if end >= n or not _ident(source[end]):
                j = end
                while j < n and source[j] in ' \t\r\n':
                    j += 1
                if j < n and source[j] == '.':
                    processes += 1
                    i = j + 1
                    continue
        i += 1
    return requires, processes


def _ident(ch):
    """True when CH can continue a JavaScript identifier. Never throws."""
    return ch.isalnum() or ch in ('_', '$')


def game_dir():
    """Resolve SMZUGGLER_GAME_DIR or raise ValueError with a path-free message.

    Refuses Steam/Proton locations. The caller owns the returned Path.
    """
    raw = os.environ.get('SMZUGGLER_GAME_DIR')
    if not raw:
        raise ValueError('SMZUGGLER_GAME_DIR must name a local test copy')
    game = Path(raw).resolve()
    if not game.is_dir():
        raise ValueError('SMZUGGLER_GAME_DIR is not a directory')
    if steam_path((*Path(raw).parts, *game.parts)):
        raise ValueError('Steam/Proton paths are refused; use a test copy')
    return game


def scan(game):
    """Yield (basename, require_count, process_count) for each js/plugins/*.js.

    game is a directory Path. Missing js/plugins raises ValueError. Files are
    read as UTF-8 (surrogates replaced) and closed before the next. Sorted
    by basename. Does not recurse.
    """
    plugins = game / 'js' / 'plugins'
    if not plugins.is_dir():
        raise ValueError('js/plugins is missing')
    for path in sorted(plugins.glob('*.js'), key=lambda p: p.name.lower()):
        if not path.is_file() or path.is_symlink():
            continue
        text = path.read_text(encoding='utf-8', errors='replace')
        req, proc = count_uses(strip_comments(text))
        yield path.name, req, proc


def main(_argv):
    """Scan SMZUGGLER_GAME_DIR and print basename plus counts to stdout.

    The game path comes from the environment so a report cannot echo a
    caller path. _argv is the process argument list and is ignored.
    Returns 0 or 2. Main thread only.
    """
    try:
        game = game_dir()
        rows = list(scan(game))
    except ValueError as error:
        print(f'scan_requires: {error}', file=sys.stderr)
        return 2
    except OSError:
        print('scan_requires: cannot read js/plugins', file=sys.stderr)
        return 2
    for name, req, proc in rows:
        print(f'{name}\trequire={req}\tprocess={proc}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
