#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Check spec section 12 licence headers on files changed since a base revision.

GPL-3.0 section 5a needs every file smzuggler changes to say so with a date, and
Outsider's own copyright and SPDX lines must survive untouched. This read-only gate
compares the working tree (tracked and untracked files) with REV; the scope is
explained in docs/DECISIONS.md (DEC-M0-T014).

Default mode, for smzuggler branches:
  * an added file, or one whose base header was already smzuggler-only, needs
    `SPDX-License-Identifier: GPL-3.0-or-later` and
    `Copyright (C) 2026 The smzuggler contributors`, and never the GeneralArcade
    commercial `LicenseRef`, which only General Arcade may grant;
  * a changed upstream file keeps every base copyright and SPDX line verbatim and
    adds the smzuggler copyright line and one more `Modified YYYY-MM-DD: why` note.
`--upstream` mode, for up/ branches: a changed file keeps its base header lines
exactly, an added file carries a copyright and SPDX line, and neither has the
smzuggler copyright line or a Modified note (docs/DECISIONS.md DEC-SETUP-005).

Only the first HEAD_LINES lines are read as the header. All regular files are
checked, including prose, configuration and extensionless scripts, except these
formats without comment syntax: JSON/JSONL, PNG/JPEG/GIF/WebP/ICO/BMP, audio/video,
fonts, PDF, archives and RPG Maker saves (SKIPPED_SUFFIXES). A renamed file is
checked as a change against its source path's base header. Deleted files and
symlinks are ignored. Checked files must decode as UTF-8; invalid bytes are errors.
Exit status: 0 clean, 1 header violations, 2 usage or git errors.
"""

import argparse
import datetime
from pathlib import Path
import re
import subprocess
import sys

HEAD_LINES = 40
SMZ_COPYRIGHT = 'Copyright (C) 2026 The smzuggler contributors'
SMZ_SPDX = 'GPL-3.0-or-later'
COMMERCIAL = 'LicenseRef-GeneralArcade-Commercial'
SKIPPED_SUFFIXES = {
    '.json', '.jsonl', '.png', '.jpg', '.jpeg', '.gif', '.webp', '.ico', '.bmp',
    '.wav', '.ogg', '.mp3', '.flac', '.opus', '.mp4', '.webm', '.avi',
    '.ttf', '.otf', '.woff', '.woff2', '.pdf', '.zip', '.gz', '.xz', '.7z',
    '.rmmzsave', '.rpgsave'}
COMMENT_MARK = re.compile(r'^\s*(?:<!--|-->|/\*+|\*+/|\*+|//+|#+)?\s*')
SPDX_RE = re.compile(r'^SPDX-License-Identifier:\s*(.+?)\s*(?:\*/|-->)?$')
COPYRIGHT_RE = re.compile(r'^Copyright \(c\)', re.IGNORECASE)
MODIFIED_RE = re.compile(r'^Modified (\d{4}-\d{2}-\d{2}): \S')


def git(root, *args):
    """Run read-only git in ROOT and return raw stdout bytes; failures raise."""
    return subprocess.run(['git', '-C', str(root), *args], check=True,
                          capture_output=True).stdout


def changed_files(root, base):
    """Return sorted (path, base_path) pairs for files changed from BASE.

    BASE_PATH is None for an added file. A rename git detects (default 50%
    similarity) keeps its source path, so a moved Outsider file is checked against
    its own base header and keeps upstream's lines (spec section 12) instead of
    being re-headed as new. Untracked, non-ignored files count as added because
    git cannot pair them with a deleted source.
    """
    out = git(root, 'diff', '--name-status', '--find-renames', '-z', base, '--')
    fields = out.decode().split('\0')
    pairs = {}
    i = 0
    while i + 1 < len(fields):
        status = fields[i]
        if status.startswith('R'):
            pairs[fields[i + 2]] = fields[i + 1]
            i += 3
            continue
        if status.startswith('A'):
            pairs[fields[i + 1]] = None
        elif status.startswith(('M', 'T')):
            pairs[fields[i + 1]] = fields[i + 1]
        i += 2
    others = git(root, 'ls-files', '--others', '--exclude-standard', '-z')
    pairs.update((p, None) for p in others.decode().split('\0') if p)
    return sorted(pairs.items())


def is_checked(path):
    """Check all paths except explicit formats that cannot carry comments."""
    return Path(path).suffix.lower() not in SKIPPED_SUFFIXES


def header(text):
    """Return the comment-stripped first HEAD_LINES lines of TEXT, in order."""
    lines = text.splitlines()[:HEAD_LINES]
    return [re.sub(r'\s*(?:\*/|-->)\s*$', '',
                   COMMENT_MARK.sub('', line, count=1)).rstrip() for line in lines]


def licence_lines(lines):
    """Return the copyright and SPDX lines of a stripped header, in order."""
    return [line for line in lines if COPYRIGHT_RE.match(line) or SPDX_RE.match(line)]


def spdx_values(lines):
    """Return every SPDX licence expression found in a stripped header."""
    return [m.group(1) for m in map(SPDX_RE.match, lines) if m]


def modified_notes(lines):
    """Return valid dated Modified notes; an impossible date is not a note."""
    notes = []
    for line in lines:
        match = MODIFIED_RE.match(line)
        if not match:
            continue
        try:
            datetime.date.fromisoformat(match.group(1))
        except ValueError:
            continue
        notes.append(line)
    return notes


def is_smzuggler_only(lines):
    """Say whether a header names only smzuggler as copyright holder."""
    holders = [line for line in lines if COPYRIGHT_RE.match(line)]
    return bool(holders) and all(line == SMZ_COPYRIGHT for line in holders)


def check_new(lines):
    """Return problems with a smzuggler-owned file's header (default mode)."""
    problems = []
    values = spdx_values(lines)
    if any(COMMERCIAL in value for value in values):
        problems.append(f'new file must not carry {COMMERCIAL}')
    if values != [SMZ_SPDX]:
        problems.append(f'needs exactly one "SPDX-License-Identifier: {SMZ_SPDX}"')
    if SMZ_COPYRIGHT not in lines:
        problems.append(f'missing "{SMZ_COPYRIGHT}"')
    return problems


def check_changed_upstream(base_lines, lines):
    """Return problems with a changed upstream file's header (default mode)."""
    problems = [f'upstream line removed or altered: "{line}"'
                for line in licence_lines(base_lines) if line not in lines]
    if SMZ_COPYRIGHT not in lines:
        problems.append(f'missing "{SMZ_COPYRIGHT}"')
    if len(modified_notes(lines)) <= len(modified_notes(base_lines)):
        problems.append('needs a new "Modified YYYY-MM-DD: <what changed>" note')
    return problems


def check_upstream_mode(base_lines, lines):
    """Return problems for an up/ branch; BASE_LINES is None for an added file."""
    problems = []
    if SMZ_COPYRIGHT in lines:
        problems.append(f'upstream offer must not carry "{SMZ_COPYRIGHT}"')
    if modified_notes(lines) != (modified_notes(base_lines) if base_lines else []):
        problems.append('upstream offer must not add a smzuggler Modified note')
    if base_lines is None:
        if not spdx_values(lines) or not any(map(COPYRIGHT_RE.match, lines)):
            problems.append('added file needs upstream copyright and SPDX lines')
    elif licence_lines(lines) != licence_lines(base_lines):
        problems.append('upstream copyright/SPDX lines must stay untouched')
    return problems


def check_file(root, base, path, base_path, upstream):
    """Return the header problems of one changed PATH; skipped types give [].

    BASE_PATH names the file at BASE (differs from PATH after a rename), or is
    None when the file is new.
    """
    target = root / path
    if not is_checked(path) or target.is_symlink() or not target.is_file():
        return []
    try:
        text = target.read_text(encoding='utf-8')
    except UnicodeDecodeError:
        return ['invalid UTF-8 in checked file; cannot validate licensing header']
    lines = header(text)
    base_lines = None
    if base_path is not None:
        # The base is trusted history, so stray bytes there are tolerated; only
        # the new content is gated, which is why it alone is decoded strictly.
        base_text = git(root, 'show', f'{base}:{base_path}').decode('utf-8', 'replace')
        base_lines = header(base_text)
    if upstream:
        return check_upstream_mode(base_lines, lines)
    if base_lines is None or is_smzuggler_only(base_lines):
        return check_new(lines)
    return check_changed_upstream(base_lines, lines)


def run(root, base, upstream):
    """Check every changed file and print one line per problem; return the count."""
    count = 0
    for path, base_path in changed_files(root, base):
        for problem in check_file(root, base, path, base_path, upstream):
            print(f'{path}: {problem}')
            count += 1
    return count


def main():
    """Parse arguments, run the gate from the repository root, and set exit status."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--base', required=True, help='revision to compare with')
    parser.add_argument('--upstream', action='store_true',
                        help='check an up/ branch: upstream headers, no smzuggler line')
    args = parser.parse_args()
    try:
        root = Path(git(Path.cwd(), 'rev-parse', '--show-toplevel').decode().strip())
        git(root, 'rev-parse', '--verify', '--quiet', f'{args.base}^{{commit}}')
        count = run(root, args.base, args.upstream)
    except (OSError, subprocess.CalledProcessError) as error:
        detail = getattr(error, 'stderr', b'') or b''
        print(f'headers: git failed for base {args.base!r}: {error} '
              f'{detail.decode(errors="replace").strip()}', file=sys.stderr)
        return 2
    if count:
        print(f'headers: {count} problem(s); see spec section 12', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
