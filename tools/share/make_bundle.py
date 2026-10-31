#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Pack a scrubbed zip of a run so a human can share it (spec §14.7, §16).

Why this exists: outside testers and other developers need logs, perf reports
and frame-time files without home paths, user names, host names, saves or game
assets. This is an offline packager; it never uploads or posts. See
docs/sharing.md. Architecture: tools/share/ in ARCHITECTURE.md.

Rules a reader must not break:
  * Load the deny-list only to scrub and scan; never print, log or copy it.
  * Findings name a file and a line or chunk, never the matched text.
  * A missing deny-list is exit 2, matching tools/privacy/scan.py.
  * Screenshots are game content: include them only with --include-screenshots.
  * Write the zip only after every staged file passes the scan.

Exit status: 0 packed, 1 privacy hit (no zip written), 2 usage or setup error.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import socket
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools' / 'privacy'))
import scan

SAVE_SUFFIXES = ('.rmmzsave', '.rpgsave')
ASSET_SUFFIXES = ('.ogg', '.m4a', '.webm', '.mp4', '.rpgmvp', '.png_')
ASSET_DIRS = {'data', 'js', 'img', 'audio', 'movies'}
SHOT_SUFFIXES = ('.png', '.jpg', '.jpeg')
PNG_DROP = {b'tEXt', b'zTXt', b'iTXt', b'eXIf', b'tIME'}
STATE_APP = 'smzuggler-rt'


def resolve_denylist(path: str | None) -> str | None:
    """Turn --denylist into an absolute path before any staging work starts.

    Relative paths are resolved against the process cwd so later temp
    directories cannot change which list is loaded. None keeps scan.py's
    default lookup. The file is not read here.
    """
    if not path:
        return None
    return str(Path(path).expanduser().resolve())


def identity_names() -> list[str]:
    """Collect the local user and host names that must become `[redacted]`.

    Values come from the process environment and the host APIs the runtime
    itself would see. Empty and single-character strings are skipped so a
    broken USER=x cannot shred every text file. Longest first.
    """
    names = []
    for key in ('USER', 'LOGNAME', 'USERNAME', 'HOSTNAME', 'COMPUTERNAME'):
        value = os.environ.get(key) or ''
        if len(value) > 1:
            names.append(value)
    try:
        host = socket.gethostname()
    except OSError:
        host = ''
    if len(host) > 1:
        names.append(host)
        short = host.split('.')[0]
        if len(short) > 1:
            names.append(short)
    unique = {item for item in names}
    return sorted(unique, key=len, reverse=True)


def home_variants(home: str) -> list[str]:
    """Return slash-style variants of HOME so both POSIX and Windows logs match.

    Trailing separators are stripped before replacing the home prefix with `~`.
    Shorter duplicates are dropped. The caller still owns `home`.
    """
    stripped = home.rstrip('/\\')
    variants = [stripped, stripped.replace('/', '\\'), stripped.replace('\\', '/')]
    seen, out = set(), []
    for item in variants:
        if item and item not in seen:
            seen.add(item)
            out.append(item)
    return out


def replace_ci(text: str, needle: str, repl: str) -> str:
    """Replace every case-insensitive occurrence of needle; text is not copied out.

    Used for user and host names, which the logger may emit in mixed case.
    An empty needle is ignored so the caller can pass optional identity fields.
    """
    if not needle:
        return text
    out, i, lower, n = [], 0, text.lower(), needle.lower()
    while True:
        found = lower.find(n, i)
        if found < 0:
            out.append(text[i:])
            return ''.join(out)
        out.append(text[i:found])
        out.append(repl)
        i = found + len(needle)


def scrub_text(text: str, home: str, names: list[str], patterns: list[re.Pattern]) -> str:
    """Rewrite home to `~` and identity plus deny-list hits to `[redacted]`.

    Home is replaced first so a user name that is also a path component does
    not break the `~` rewrite. Deny-list patterns are applied last and never
    logged. The original string is not retained.
    """
    if home:
        for variant in home_variants(home):
            text = text.replace(variant, '~')
    for name in names:
        text = replace_ci(text, name, '[redacted]')
    for pattern in patterns:
        text = pattern.sub('[redacted]', text)
    return text


def strip_png(data: bytes) -> bytes:
    """Drop PNG text/time/EXIF chunks, copying every other chunk unchanged.

    Pixel bytes live in IDAT; they are not decompressed or recompressed.
    Truncated input is returned as far as it parsed so the scan can still run.
    """
    sig = b'\x89PNG\r\n\x1a\n'
    if not data.startswith(sig):
        return data
    out = bytearray(sig)
    i, n = 8, len(data)
    while i + 8 <= n:
        length = int.from_bytes(data[i:i + 4], 'big')
        end = i + 12 + length
        if end > n:
            break
        ctype = data[i + 4:i + 8]
        if ctype not in PNG_DROP:
            out.extend(data[i:end])
        if ctype == b'IEND':
            break
        i = end
    return bytes(out)


def jpeg_skip_marker(data: bytes, i: int) -> int | None:
    """Return the index after a JPEG marker that has no payload length.

    Stand-alone fill bytes, RSTn and SOI-style markers must not consume the
    next two bytes as a length, or SOS scan data would be mis-parsed.
    """
    marker = data[i + 1]
    if marker == 0x00 or marker == 0xFF:
        return i + 1
    if 0xD0 <= marker <= 0xD7:
        return i + 2
    return None


def strip_jpeg(data: bytes) -> bytes:
    """Drop JPEG EXIF/XMP (APP1) and COM segments; copy the rest bit-for-bit.

    SOS and everything after it is copied verbatim so Huffman tables and
    scan data stay pixel-identical. Non-JPEG input is returned unchanged.
    """
    if not data.startswith(b'\xff\xd8'):
        return data
    out = bytearray(data[:2])
    i, n = 2, len(data)
    while i + 1 < n:
        if data[i] != 0xFF:
            out.extend(data[i:])
            break
        marker = data[i + 1]
        if marker in (0xDA, 0xD9):
            out.extend(data[i:])
            break
        skipped = jpeg_skip_marker(data, i)
        if skipped is not None:
            out.extend(data[i:skipped])
            i = skipped
            continue
        if i + 4 > n:
            out.extend(data[i:])
            break
        length = int.from_bytes(data[i + 2:i + 4], 'big')
        end = i + 2 + length
        if marker not in (0xE1, 0xFE):
            out.extend(data[i:end])
        i = end
    return bytes(out)


def is_text_blob(data: bytes) -> bool:
    """True when the blob can be scrubbed as UTF-8 text rather than bytes.

    A NUL in the first 8 KiB matches scan.py's binary heuristic: those files
    cannot be rewritten safely, so the later deny-list byte scan must refuse.
    """
    if b'\0' in data[:8192]:
        return False
    try:
        data.decode('utf-8')
    except UnicodeDecodeError:
        return False
    return True


def is_save(path: Path) -> bool:
    """True for RPG Maker save files and the `global*` sidecar names."""
    name = path.name
    lower = name.lower()
    return lower.endswith(SAVE_SUFFIXES) or lower.startswith('global')


def is_game_asset(rel: str) -> bool:
    """True for audio/video/encrypted images, and JSON under game asset dirs.

    JSON at the run root (perf reports) stays collectible; only JSON whose
    path walks through data/, js/, img/, audio/ or movies/ is treated as a
    game file. Other asset suffixes are never run reports.
    """
    path = Path(rel)
    suffix = path.suffix.lower()
    if suffix in ASSET_SUFFIXES:
        return True
    if suffix == '.json' and any(part.lower() in ASSET_DIRS for part in path.parts[:-1]):
        return True
    return False


def is_screenshot(path: Path) -> bool:
    """True for PNG/JPEG names; inclusion still requires --include-screenshots."""
    return path.suffix.lower() in SHOT_SUFFIXES


def is_collectible(path: Path) -> bool:
    """True for the run artefacts this packager is allowed to ship.

    Logs, perf-report-* JSON/Markdown, frame-time CSV/JSONL and screenshots
    (when opted in) are the set in the M0-T023 card. Other files are ignored.
    """
    name, suffix = path.name, path.suffix.lower()
    if suffix == '.log':
        return True
    if name.startswith('perf-report-') and suffix in {'.json', '.md'}:
        return True
    if suffix in {'.csv', '.jsonl'}:
        return True
    return is_screenshot(path)


def is_under(path: Path, root: Path) -> bool:
    """True when path resolves inside root; used to keep the game dir out."""
    try:
        path.resolve().relative_to(root.resolve())
    except (ValueError, OSError):
        return False
    return True


def file_kind(zip_name: str) -> str:
    """One-line description of a packed file for README.txt."""
    base = Path(zip_name).name.lower()
    if base.endswith('.log'):
        return 'runtime log'
    if base.startswith('perf-report-') and base.endswith('.json'):
        return 'performance report'
    if base.startswith('perf-report-') and base.endswith('.md'):
        return 'performance summary'
    if base.endswith('.csv') or base.endswith('.jsonl'):
        return 'frame times'
    if base.endswith(SHOT_SUFFIXES):
        return 'screenshot'
    return 'run file'


def last_run_dir() -> Path:
    """Pick the newest directory under `$XDG_STATE_HOME/smzuggler-rt/runs`.

    Why: `smzuggler-rt --share-bundle` has no run path and must pack the
    last run the runtime wrote. Missing or empty state is a setup error
    (exit 2), not an empty zip. Symlink entries are ignored so a link
    cannot pull in a directory outside the state tree.
    """
    xdg = os.environ.get('XDG_STATE_HOME')
    if xdg:
        base = Path(xdg) / STATE_APP / 'runs'
    else:
        home = os.environ.get('HOME') or str(Path.home())
        base = Path(home) / '.local' / 'state' / STATE_APP / 'runs'
    if not base.is_dir():
        print('share: no last run under the state directory', file=sys.stderr)
        raise SystemExit(2)
    dirs = [p for p in base.iterdir() if p.is_dir() and not p.is_symlink()]
    if not dirs:
        print('share: no last run under the state directory', file=sys.stderr)
        raise SystemExit(2)
    return max(dirs, key=lambda item: item.stat().st_mtime)


def consider(path: Path, run_root: Path, game_root: Path | None, include_shots: bool) -> tuple[str, str]:
    """Decide include/exclude/skip for one filesystem entry under a run.

    Exclusions are listed by run-relative name only. Symlinks are never
    followed, so a link cannot smuggle a file from outside the run.
    """
    rel = path.relative_to(run_root).as_posix()
    if path.is_symlink():
        return 'exclude', rel
    if game_root is not None and is_under(path, game_root):
        return 'exclude', rel
    if is_save(path) or is_game_asset(rel):
        return 'exclude', rel
    if is_screenshot(path) and not include_shots:
        return 'exclude', rel
    if is_collectible(path):
        return 'include', rel
    return 'skip', rel


def collect_run(run_dir: Path, game_root: Path | None, include_shots: bool) -> tuple[list[tuple[str, Path]], list[str]]:
    """Walk one run directory without following links; return (files, exclusions).

    Directory symlinks are recorded as exclusions and pruned so os.walk
    cannot descend into them. The returned paths are still the on-disk
    files; staging copies and scrubs them later.
    """
    included: list[tuple[str, Path]] = []
    excluded: list[str] = []
    root = run_dir.resolve()
    for dirpath, dirnames, filenames in os.walk(root, followlinks=False):
        current = Path(dirpath)
        keep = []
        for name in dirnames:
            child = current / name
            if child.is_symlink():
                excluded.append(child.relative_to(root).as_posix())
            else:
                keep.append(name)
        dirnames[:] = keep
        for name in filenames:
            action, rel = consider(current / name, root, game_root, include_shots)
            if action == 'exclude':
                excluded.append(rel)
            elif action == 'include':
                included.append((rel, current / name))
    return included, excluded


def prepare_bytes(path: Path, home: str, names: list[str], patterns: list[re.Pattern]) -> bytes:
    """Read one file, strip image metadata or scrub text, leave binaries intact.

    Images are rewritten only by dropping metadata chunks/segments. Text is
    scrubbed in memory. Binary blobs are returned unchanged so a deny-list
    hit inside them fails the later scan instead of being silently packed.
    """
    data = path.read_bytes()
    suffix = path.suffix.lower()
    if suffix == '.png':
        return strip_png(data)
    if suffix in {'.jpg', '.jpeg'}:
        return strip_jpeg(data)
    if is_text_blob(data):
        return scrub_text(data.decode('utf-8'), home, names, patterns).encode('utf-8')
    return data


def contains_deny(data: bytes, patterns: list[re.Pattern]) -> bool:
    """True when any deny-list pattern matches the bytes as latin-1 text.

    latin-1 never fails, so a term planted in a binary file is still found.
    The matched text is discarded; only the boolean is returned.
    """
    text = data.decode('latin-1')
    return any(pattern.search(text) for pattern in patterns)


def git_one(*args: str) -> str:
    """Read one git value from the repository; `unknown` if git is unavailable.

    Used for README provenance (commit, version). Failures are not fatal: a
    bundle built from an unpacked tarball still packs, with `unknown` fields.
    """
    try:
        result = subprocess.run(
            ['git', '-C', str(ROOT), *args], capture_output=True, text=True)
    except OSError:
        return 'unknown'
    value = result.stdout.strip()
    return value if result.returncode == 0 and value else 'unknown'


def format_machine_state(value: object) -> str:
    """Render a report's §14.4 machine_state field as plain key: value lines."""
    if isinstance(value, dict):
        return '\n'.join(f'{key}: {item}' for key, item in value.items())
    return str(value)


def machine_state_block(staged: list[tuple[str, bytes]]) -> str:
    """Pull §14.4 machine-state text out of staged perf-report JSON files.

    Staged bytes are already scrubbed, so the block cannot re-introduce a
    home path or user name that the JSON originally contained.
    """
    blocks = []
    for name, data in staged:
        base = Path(name).name
        if not (base.startswith('perf-report-') and base.endswith('.json')):
            continue
        try:
            payload = json.loads(data.decode('utf-8'))
        except (UnicodeDecodeError, json.JSONDecodeError):
            continue
        state = payload.get('machine_state')
        if state is not None:
            blocks.append(format_machine_state(state))
    return '\n'.join(blocks) if blocks else 'not recorded'


def build_readme(staged: list[tuple[str, bytes]], excluded: list[str]) -> str:
    """Compose README.txt: collector identity, machine state, files, exclusions.

    One line per included file says what it is. Exclusions are relative names
    only, so a game-dir path cannot leak the install location. No deny-list
    text is included.
    """
    lines = [
        f'Collector commit: {git_one("rev-parse", "HEAD")}',
        f'smzuggler version: {git_one("describe", "--always", "--dirty")}',
        '',
        'Machine state (spec section 14.4):',
        machine_state_block(staged),
        '',
        'Files:',
    ]
    for name, _data in staged:
        lines.append(f'{name}: {file_kind(name)}')
    if excluded:
        lines.append('')
        lines.append('Excluded (relative names only):')
        lines.extend(excluded)
    lines.append('')
    lines.append('This archive was packed locally. Posting it is a human step.')
    lines.append('The packager never uploads or posts anything.')
    return '\n'.join(lines) + '\n'


def scan_staged(stage: Path, names: list[str], denylist: str | None, patterns: list[re.Pattern]) -> int:
    """Fail closed if a staged file still matches the deny-list or scan.py.

    scan.py skips NUL binaries, so this also searches latin-1 text of every
    staged file. Hits print `privacy: file:line-or-chunk: rule` and never
    the matched text. Returns 0, 1 (findings) or 2 (scanner setup error).
    """
    code = 0
    for rel in names:
        data = (stage / rel).read_bytes()
        if contains_deny(data, patterns):
            print(f'privacy: {rel}:chunk: deny-list')
            code = 1
    cmd = [sys.executable, str(ROOT / 'tools' / 'privacy' / 'scan.py'),
           '--repo', str(ROOT), '--paths']
    cmd.extend(str(stage / rel) for rel in names)
    if denylist:
        cmd[4:4] = ['--denylist', denylist]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.stdout:
        sys.stdout.write(result.stdout)
    if result.stderr:
        sys.stderr.write(result.stderr)
    if result.returncode == 2:
        return 2
    if result.returncode != 0:
        code = 1
    return code


def write_zip(out: Path, staged: list[tuple[str, bytes]]) -> None:
    """Write the zip next to OUT and replace OUT only after a complete archive.

    A privacy refusal therefore cannot clobber a previous good bundle. The
    temporary file lives in OUT's directory so os.replace stays on one mount.
    """
    out.parent.mkdir(parents=True, exist_ok=True)
    handle, tmp_name = tempfile.mkstemp(prefix='share-', suffix='.zip', dir=out.parent)
    os.close(handle)
    tmp = Path(tmp_name)
    try:
        with zipfile.ZipFile(tmp, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            for name, data in staged:
                archive.writestr(name, data)
        os.replace(tmp, out)
    except Exception:
        tmp.unlink(missing_ok=True)
        raise


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    """CLI for one or more run directories; `-o` is required, upload is absent."""
    parser = argparse.ArgumentParser(
        description='Pack a scrubbed zip of a smzuggler run. Never uploads.')
    parser.add_argument('run_dirs', nargs='*', help='run directories to pack')
    parser.add_argument('-o', '--output', required=True, help='destination zip')
    parser.add_argument('--denylist', help='deny-list file (scan.py default otherwise)')
    parser.add_argument('--include-screenshots', action='store_true',
                        help='include PNG/JPEG screenshots (game content; check terms)')
    parser.add_argument('--game-dir', help='exclude anything under this game folder')
    parser.add_argument('--last-run', action='store_true',
                        help='pack the newest $XDG_STATE_HOME/smzuggler-rt/runs/*')
    return parser.parse_args(argv)


def run_dirs_from_args(args: argparse.Namespace) -> list[Path]:
    """Resolve positional run dirs and optional --last-run; exit 2 if none exist."""
    dirs = [Path(item) for item in args.run_dirs]
    if args.last_run:
        dirs.append(last_run_dir())
    if not dirs:
        print('share: give RUN_DIR... or --last-run', file=sys.stderr)
        raise SystemExit(2)
    resolved = []
    for item in dirs:
        if not item.is_dir():
            print(f'share: not a directory: {item.as_posix()}', file=sys.stderr)
            raise SystemExit(2)
        resolved.append(item.resolve())
    return resolved


def stage_runs(runs: list[Path], game_root: Path | None, include_shots: bool, home: str, names: list[str], patterns: list[re.Pattern]) -> tuple[list[tuple[str, bytes]], list[str]]:
    """Scrub every collected file from each run into (`zip-name`, bytes) pairs."""
    staged: list[tuple[str, bytes]] = []
    excluded: list[str] = []
    for index, run in enumerate(runs, 1):
        files, skipped = collect_run(run, game_root, include_shots)
        excluded.extend(skipped)
        prefix = f'run-{index}'
        for rel, path in files:
            staged.append((f'{prefix}/{rel}', prepare_bytes(path, home, names, patterns)))
    return staged, excluded


def pack(args: argparse.Namespace) -> int:
    """Load the deny-list, stage, scan, and write the zip or refuse.

    The deny-list is resolved and loaded before any file is copied. On a
    scan hit the destination zip is left untouched (or uncreated).
    """
    denylist = resolve_denylist(args.denylist)
    patterns = scan.load_denylist(denylist)
    home = os.environ.get('HOME') or os.environ.get('USERPROFILE') or ''
    names = identity_names()
    game_root = Path(args.game_dir).resolve() if args.game_dir else None
    runs = run_dirs_from_args(args)
    staged, excluded = stage_runs(
        runs, game_root, args.include_screenshots, home, names, patterns)
    for rel in excluded:
        print(f'excluded: {rel}')
    readme = build_readme(staged, excluded)
    staged.insert(0, ('README.txt', readme.encode('utf-8')))
    with tempfile.TemporaryDirectory(prefix='share-stage-') as raw:
        stage = Path(raw)
        for rel, data in staged:
            dest = stage / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
        status = scan_staged(stage, [rel for rel, _ in staged], denylist, patterns)
        if status != 0:
            return status
        write_zip(Path(args.output), staged)
    return 0


def main(argv: list[str] | None = None) -> int:
    """Entry point used by the CLI and by smzuggler-rt --share-bundle."""
    try:
        return pack(parse_args(argv))
    except BrokenPipeError:
        return 1


if __name__ == '__main__':
    sys.exit(main())
