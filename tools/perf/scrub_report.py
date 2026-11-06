#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Scrub a --perf-suite report directory into a shareable archive (spec §14.8).

Why this exists: an outside tester runs the Deck suite and sends the result
back. The report must carry only the fixed §14.5/§14.7 schema fields, frame
records, summaries, markers and run metadata, never a home path, local user or
host name, and never a save file. The packager itself is
tools/share/make_bundle.py (M0-T023); this script filters the schema first and
then asks that packager's helpers to scrub, scan and write the archive. It
never uploads or posts anything. See docs/testing.md.

Rules a reader must not break:
  * Drop every field outside the fixed schema instead of searching for secrets.
  * Rewrite home prefixes to `~` before looking for local user/host names, so
    the user name inside a home path is not mistaken for a leak.
  * Refuse on a local identity hit, a save file, or a scan finding; write no
    archive in that case.
  * Print a relative file name and a rule word, never the matched text.

Exit status: 0 packed, 1 refused (identity, save or scan hit), 2 usage/setup.
"""
from __future__ import annotations

import argparse
import io
import json
import os
from pathlib import Path
import re
import sys
import tarfile
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools' / 'share'))
import make_bundle

SAVE_SUFFIXES = ('.rmmzsave', '.rpgsave')

FRAME_FIELDS = {
    'frame': ('type', 'frame', 'work_ms', 'gpu_ms',
              'present_interval_ms', 'missed_vblank', 'over_budget'),
    'summary': ('type', 'frames', 'refresh_hz', 'budget_ms', 'mean_work_ms',
                'p50_work_ms', 'p95_work_ms', 'p99_work_ms',
                'p99_9_work_ms', 'max_work_ms', 'frames_over_budget',
                'missed_vblanks', 'gpu_frames', 'mean_gpu_ms'),
    'marker': ('type', 'frame', 'kind', 'phase'),
}

REPORT_FIELDS = (
    'schema', 'profile', 'scene', 'gate', 'passed', 'warmup_frames',
    'scored_frames', 'refresh_hz', 'frames_total', 'frames_scored',
    'frames_excluded', 'frames_counted', 'criteria', 'summary', 'runs',
    'run_on', 'device_model', 'model', 'steamos_version', 'steamos_channel',
    'session', 'tdp_w', 'power_source', 'commit', 'build_id',
    'machine_state', 'notes',
)
CRITERIA_KEYS = ('p99_work_ms', 'p99_9_work_ms', 'missed_vblank_percent',
                 'max_work_ms', 'mean_work_ms')
SUMMARY_KEYS = ('mean_work_ms', 'p50_work_ms', 'p95_work_ms', 'p99_work_ms',
                'p99_9_work_ms', 'max_work_ms', 'missed_vblanks',
                'missed_vblank_percent')
MACHINE_KEYS = ('power_source', 'profile', 'battery_start_pct',
                'battery_end_pct', 'epp')
CRITERION_KEYS = ('value', 'limit', 'ok')


def home_prefixes(home: str) -> list[str]:
    """Return the slash-style HOME spellings make_bundle rewrites to `~`.

    Why: a decoded report may carry a POSIX or Windows home prefix. Applying
    the same variants keeps this pre-check in step with the packager.
    """
    return make_bundle.home_variants(home) if home else []


def replace_home(text: str, home: str) -> str:
    """Rewrite every HOME spelling in TEXT to `~`; the token is never logged."""
    for variant in home_prefixes(home):
        text = text.replace(variant, '~')
    return text


def local_identity_hit(text: str, names: list[str]) -> bool:
    """True when TEXT still contains a local user or host name.

    Why: home prefixes are removed first, so a bare name in a field such as
    `scene` or `notes` is a refusal rather than something to redact silently.
    The matched value is discarded.
    """
    lower = text.lower()
    return any(name.lower() in lower for name in names)


def is_save_path(path: Path) -> bool:
    """True for RPG Maker saves, including the project's `.rmmzsave` suffix."""
    lower = path.name.lower()
    return lower.endswith(SAVE_SUFFIXES) or lower.startswith('global')


def walk_files(root: Path) -> list[Path]:
    """List regular files below ROOT without following links, sorted by name."""
    found = []
    for dirpath, dirnames, filenames in os.walk(root, followlinks=False):
        current = Path(dirpath)
        dirnames[:] = sorted(
            name for name in dirnames if not (current / name).is_symlink())
        for name in sorted(filenames):
            found.append(current / name)
    return found


def check_tree(root: Path, home: str, names: list[str]) -> tuple[list[Path], str | None]:
    """Refuse saves and local identities anywhere in ROOT before filtering.

    Returns the files to consider and a refusal reason. The whole tree is
    checked, not only the files that survive schema filtering, because a save
    or name must block the pack even when it sits in a file the scrubber would
    otherwise drop.
    """
    files = walk_files(root)
    for path in files:
        rel = path.relative_to(root).as_posix()
        if is_save_path(path):
            return [], f'{rel}: save file'
        if local_identity_hit(rel, names):
            return [], f'{rel}: local identity'
        if path.is_symlink():
            continue
        text = replace_home(path.read_bytes().decode('latin-1'), home)
        if local_identity_hit(text, names):
            return [], f'{rel}: local identity'
    return files, None


def clean_value(value: object, home: str, names: list[str], patterns: list) -> object:
    """Keep a primitive JSON value and scrub strings with the packager rules.

    Why: a report can carry a home path in any string, so every string that
    survives schema filtering goes through the same scrubber as a shared run.
    Containers are copied; dictionaries keep only string keys.
    """
    if isinstance(value, str):
        return make_bundle.scrub_text(value, home, names, patterns)
    if isinstance(value, list):
        return [clean_value(item, home, names, patterns) for item in value]
    if isinstance(value, dict):
        return {str(key): clean_value(item, home, names, patterns)
                for key, item in value.items()}
    if isinstance(value, (bool, int, float)) or value is None:
        return value
    return None


def filter_frame_line(line: str, home: str, names: list[str], patterns: list) -> str | None:
    """Return one scrubbed frame/summary/marker record, or None to drop it.

    Why: the stream is untrusted input from another machine, so only a known
    record type and its fixed field list survive; unknown types, unknown
    fields and malformed JSON are dropped rather than searched for secrets.
    """
    try:
        record = json.loads(line)
    except json.JSONDecodeError:
        return None
    if not isinstance(record, dict):
        return None
    fields = FRAME_FIELDS.get(record.get('type'))
    if fields is None:
        return None
    kept = {key: clean_value(record[key], home, names, patterns)
            for key in fields if key in record}
    return json.dumps(kept, separators=(',', ':')) + '\n'


def filter_report(report: object, home: str, names: list[str], patterns: list) -> dict | None:
    """Keep only the fixed report schema, recursing into `runs` and criteria.

    Why: the analyser controls the field names, so an allow-list makes the
    kept surface explicit and a new free-text field cannot leak by accident.
    Unknown top-level and nested keys are dropped, not redacted.
    """
    if not isinstance(report, dict):
        return None
    kept = {}
    for key in REPORT_FIELDS:
        if key not in report:
            continue
        value = report[key]
        if key == 'criteria' and isinstance(value, dict):
            kept[key] = {
                name: {part: clean_value(item.get(part), home, names, patterns)
                       for part in CRITERION_KEYS if part in item}
                for name, item in value.items()
                if name in CRITERIA_KEYS and isinstance(item, dict)
            }
        elif key == 'summary' and isinstance(value, dict):
            kept[key] = {name: clean_value(value[name], home, names, patterns)
                         for name in SUMMARY_KEYS if name in value}
        elif key == 'machine_state' and isinstance(value, dict):
            kept[key] = {name: clean_value(value[name], home, names, patterns)
                         for name in MACHINE_KEYS if name in value}
        elif key == 'runs' and isinstance(value, list):
            runs = [filter_report(item, home, names, patterns)
                    for item in value if isinstance(item, dict)]
            kept[key] = [item for item in runs if item is not None]
        else:
            kept[key] = clean_value(value, home, names, patterns)
    return kept


def scrub_jsonl(path: Path, home: str, names: list[str], patterns: list) -> bytes:
    """Filter a frame-stats JSON Lines file line by line.

    Why: the frame stream is the measurement itself, so the fixed frame,
    summary and marker records survive while an unknown record disappears.
    The returned bytes are owned by the caller.
    """
    text = replace_home(path.read_text(encoding='utf-8'), home)
    lines = [filter_frame_line(line, home, names, patterns)
             for line in text.splitlines() if line.strip()]
    return ''.join(line for line in lines if line).encode('utf-8')


def scrub_report_json(path: Path, home: str, names: list[str], patterns: list) -> bytes:
    """Filter one perf JSON report to the fixed schema.

    Why: the JSON report is the evidence a Deck confirmation cites, so its
    measurements and gate word stay while any extra key is dropped. A report
    that is not an object raises ValueError and is not packed.
    """
    text = replace_home(path.read_text(encoding='utf-8'), home)
    report = filter_report(json.loads(text), home, names, patterns)
    if report is None:
        raise ValueError('report is not a JSON object')
    return (json.dumps(report, indent=2) + '\n').encode('utf-8')


def scrub_markdown(path: Path, home: str, names: list[str], patterns: list) -> bytes:
    """Scrub the generated Markdown summary that pairs with a report JSON.

    Why: the summary is generated from the report and a tester may open it, so
    it is kept as the human-readable pair but still passes the home and
    deny-list rewrites. Unknown Markdown files are not collected.
    """
    text = replace_home(path.read_text(encoding='utf-8'), home)
    return make_bundle.scrub_text(text, home, names, patterns).encode('utf-8')


def stage_reports(files: list[Path], root: Path, home: str, names: list[str], patterns: list) -> tuple[list[tuple[str, bytes]], list[str]]:
    """Return (`report/<rel>`, bytes) for schema files and dropped relative names.

    Why: only frame streams and generated report pairs belong in a tester
    archive, so every other file is listed as dropped by relative name instead
    of being packed. Symlinks are never followed.
    """
    staged: list[tuple[str, bytes]] = []
    dropped: list[str] = []
    for path in files:
        rel = path.relative_to(root).as_posix()
        if path.is_symlink():
            dropped.append(rel)
            continue
        try:
            if path.name.startswith('frame-stats-') and path.suffix == '.jsonl':
                data = scrub_jsonl(path, home, names, patterns)
            elif path.name.startswith('perf-report-') and path.suffix == '.json':
                data = scrub_report_json(path, home, names, patterns)
            elif path.name.startswith('perf-report-') and path.suffix == '.md':
                data = scrub_markdown(path, home, names, patterns)
            else:
                dropped.append(rel)
                continue
        except (OSError, ValueError, UnicodeError):
            dropped.append(rel)
            continue
        staged.append((f'report/{rel}', data))
    return staged, dropped


def scrubbed_note(staged: list[tuple[str, bytes]], dropped: list[str]) -> bytes:
    """Describe what was kept and dropped so the tester can review the archive."""
    lines = [
        'SCRUBBED.txt',
        '',
        'Kept (fixed report schema only):',
    ]
    for name, _data in staged:
        lines.append(f'{name}: {make_bundle.file_kind(name)}')
    lines += ['', 'Dropped (not part of the fixed report schema):']
    lines.extend(dropped or ['(none)'])
    lines += [
        '',
        'Home prefixes were written as ~.',
        'No local user or host name was found; saves are refused before packing.',
        'The archive was packed locally. Posting it is a human step.',
    ]
    return ('\n'.join(lines) + '\n').encode('utf-8')


def write_tar_gz(out: Path, staged: list[tuple[str, bytes]]) -> None:
    """Write a tar.gz with zero metadata, replacing OUT only on success."""
    out.parent.mkdir(parents=True, exist_ok=True)
    handle, tmp_name = tempfile.mkstemp(prefix='scrub-', suffix='.tar.gz',
                                       dir=out.parent)
    os.close(handle)
    tmp = Path(tmp_name)
    try:
        with tarfile.open(tmp, 'w:gz') as archive:
            for name, data in staged:
                info = tarfile.TarInfo(name)
                info.size = len(data)
                info.mtime = 0
                info.mode = 0o644
                info.uid = info.gid = 0
                info.uname = info.gname = ''
                archive.addfile(info, io.BytesIO(data))
        os.replace(tmp, out)
    except Exception:
        tmp.unlink(missing_ok=True)
        raise


def write_archive(out: Path, staged: list[tuple[str, bytes]]) -> None:
    """Pick the archive format from OUT; `.tar.gz`/`.tgz` is tar, else zip."""
    lower = out.name.lower()
    if lower.endswith(('.tar.gz', '.tgz')):
        write_tar_gz(out, staged)
    else:
        make_bundle.write_zip(out, staged)


def default_denylist() -> str | None:
    """Return an existing project deny-list path, or None on a tester machine.

    Why: an outside tester has no copy of the project's personal deny-list;
    in that case the scrubber falls back to the local identity names it already
    refuses, so the archive still passes scan.py's generic rules.
    """
    candidate = (os.environ.get('SMZUGGLER_PRIVACY_DENYLIST') or
                 make_bundle.scan.DEFAULT_DENYLIST)
    path = Path(os.path.expanduser(candidate))
    return str(path) if path.is_file() else None


def identity_patterns(names: list[str]) -> list[re.Pattern]:
    """Compile local identity names as literal deny-list patterns.

    Used only when no project list exists; it keeps scan.py's fail-closed
    behaviour across the same names the tree check refuses. The list is never
    printed or packed.
    """
    values = names or ['__smzuggler_no_identity__']
    return [re.compile(re.escape(name), re.I) for name in values]


def deny_source(denylist: str | None, names: list[str]) -> tuple[str | None, list[re.Pattern], str | None]:
    """Choose the deny-list path, its patterns and any fallback file text.

    An explicit `--denylist` stays mandatory: a missing explicit list is still
    a setup error. Only the absent default on a tester machine falls back.
    """
    if denylist:
        path = make_bundle.resolve_denylist(denylist)
        return path, make_bundle.scan.load_denylist(path), None
    if os.environ.get('PRIVACY_DENYLIST'):
        return None, make_bundle.scan.load_denylist(None), None
    path = default_denylist()
    if path:
        return path, make_bundle.scan.load_denylist(path), None
    values = names or ['__smzuggler_no_identity__']
    return None, identity_patterns(values), '\n'.join(values) + '\n'


def pack_report(root: Path, out: Path, denylist: str | None) -> int:
    """Filter, scan and write one report archive; return the process status."""
    home = os.environ.get('HOME') or os.environ.get('USERPROFILE') or ''
    names = make_bundle.identity_names()
    files, refusal = check_tree(root, home, names)
    if refusal:
        print(f'scrub-report: {refusal}; refusing to pack', file=sys.stderr)
        return 1
    resolved_denylist, patterns, fallback_text = deny_source(denylist, names)
    reports, dropped = stage_reports(files, root, home, names, patterns)
    note = scrubbed_note(reports, dropped)
    readme = make_bundle.build_readme(reports, dropped).encode('utf-8')
    staged = [('README.txt', readme), ('SCRUBBED.txt', note), *reports]
    with tempfile.TemporaryDirectory(prefix='scrub-stage-') as raw:
        base = Path(raw)
        scan_denylist = resolved_denylist
        if fallback_text is not None:
            fallback = base / 'deny.txt'
            fallback.write_text(fallback_text, encoding='utf-8')
            scan_denylist = str(fallback)
        stage = base / 'stage'
        for rel, data in staged:
            dest = stage / rel
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
        status = make_bundle.scan_staged(
            stage, [rel for rel, _data in staged], scan_denylist, patterns)
        if status != 0:
            return status
        write_archive(out, staged)
    return 0


def parse_args(argv: list[str] | None) -> argparse.Namespace:
    """CLI: one report directory, an archive path and an optional deny-list."""
    parser = argparse.ArgumentParser(
        description='Scrub a smzuggler Deck report directory into a local archive.')
    parser.add_argument('report_dir', help='directory written by --perf-suite')
    parser.add_argument('--out', default='deck-report.zip',
                        help='archive to write (.tar.gz/.tgz or .zip)')
    parser.add_argument('--denylist',
                        help='deny-list file (scan.py default otherwise)')
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """Entry point used by the CLI and by smzuggler-rt --scrub-report."""
    args = parse_args(argv)
    root = Path(args.report_dir)
    if not root.is_dir():
        print(f'scrub-report: not a directory: {root.as_posix()}', file=sys.stderr)
        return 2
    try:
        return pack_report(root.resolve(), Path(args.out), args.denylist)
    except SystemExit:
        raise
    except (OSError, ValueError) as exc:
        print(f'scrub-report: {exc}', file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
