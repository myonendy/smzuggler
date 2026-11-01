#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Turn a Map 92 profiling run into the M0 QuickJS report (spec section 6.1).

Why this exists: M0's exit criterion is a written profile report, but forge
workers have no game window. This tool builds `docs/perf/m0-map92-profile.md`
from attribution JSON, a GameLoopStats log, a frame-time CSV and a §14.4
machine-state snapshot so the owner can run it once on dev-laptop after merge
(DEC-M0-T018-OWNER-RUN). See docs/perf.md and docs/perf/m0-map92-profile-recipe.md.

Rules a reader must not break:
  * Never copy raw input text into the report. Only parsed numbers, bucket
    labels and the five machine-state fields are written, each passed through
    strip_paths(), so host names, user names and absolute paths in the inputs
    cannot leak (spec section 16).
  * Do not invent profile numbers. Empty inputs fail; the 15% JS-time gap is
    a FLAG in the report, not a nonzero exit.
  * GameLoopStats JS time is timers + jobs + rAF. Attributed JS time is the
    QuickJS + GC + shim JS + game JS self share of mean frame time
    (DEC-M0-T018b).

Exit status: 0 on success, 2 on a usage or input error.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path
import re
import sys

JS_BUCKETS = frozenset({'quickjs', 'gc', 'shim_js', 'game_js'})
GAP_LIMIT_PCT = 15.0
MACHINE_KEYS = (
    ('power_source', 'Power source'),
    ('profile', 'Profile'),
    ('battery_start_pct', 'Battery start'),
    ('battery_end_pct', 'Battery end'),
    ('epp', 'EPP'),
)
PATH_TOKEN = re.compile(r'(?:file://)?(?:/[^\s/:()\[\]]+)+/([^\s/:()\[\]]+)')
WIN_PATH = re.compile(r'[A-Za-z]:[\\/](?:[^\s|:\\/]+[\\/])+([^\s|:\\/]+)')
STATS = re.compile(
    r'perf:\s+(\d+)\s+frames avg\s+([\d.]+)\s+ms\s+\('
    r'events\s+([\d.]+),\s+timers\s+([\d.]+),\s+jobs\s+([\d.]+),\s+'
    r'rAF\s+([\d.]+),\s+swap\s+([\d.]+)\)')


def strip_paths(text):
    """Replace every absolute POSIX or Windows path in `text` with its basename.

    Why: logs, attribution `source` and machine-state extras can carry a home
    directory (spec section 16). `text` is any string; the result is a new
    string. Never fails.
    """
    cleaned = PATH_TOKEN.sub(r'\1', str(text))
    return WIN_PATH.sub(r'\1', cleaned)


def basename(path):
    """Return the final component of `path` so errors never print a home path."""
    return Path(path).name


def load_json(path):
    """Read JSON from `path`; ValueError names only the basename on failure."""
    try:
        with open(path, encoding='utf-8') as handle:
            return json.load(handle)
    except OSError as error:
        raise ValueError(f'cannot read {basename(path)}: {error.strerror}') from error
    except json.JSONDecodeError as error:
        raise ValueError(f'invalid JSON in {basename(path)}: {error.msg}') from error


def load_attribution(path):
    """Load attribute.py JSON and require a nonempty `buckets` list.

    Why: the bucket table and attributed JS time both come from this file, so
    a missing list would silently invent a 0% split. Each bucket needs `id`
    and either `self_pct` or `self_weight`.
    """
    data = load_json(path)
    buckets = data.get('buckets')
    if not isinstance(buckets, list) or not buckets:
        raise ValueError(f'{basename(path)}: missing buckets')
    for row in buckets:
        if 'id' not in row:
            raise ValueError(f'{basename(path)}: bucket without id')
        if 'self_pct' not in row and 'self_weight' not in row:
            raise ValueError(f"{basename(path)}: bucket {row['id']} has no self time")
        if 'self_pct' not in row:
            total = sum(float(item.get('self_weight') or 0) for item in buckets)
            weight = float(row.get('self_weight') or 0)
            row['self_pct'] = round(100.0 * weight / total, 2) if total else 0.0
        if 'label' not in row:
            row['label'] = str(row['id'])
    data['buckets'] = buckets
    return data


def load_machine_state(path):
    """Load the five §14.4 fields; extra keys are ignored so they cannot leak.

    Why: the report must record power source, profile, battery start/end and
    EPP, and nothing else from the snapshot. Missing keys fail loudly.
    Values are stripped of paths; numbers stay numbers.
    """
    data = load_json(path)
    if not isinstance(data, dict):
        raise ValueError(f'{basename(path)}: machine state must be a JSON object')
    out = {}
    for key, _label in MACHINE_KEYS:
        if key not in data:
            raise ValueError(f'{basename(path)}: missing {key}')
        value = data[key]
        out[key] = value
    return out


def parse_stats_log(path):
    """Parse GameLoopStats `perf:` lines into {n, total, events, timers, jobs, raf, swap}.

    Why: the 15% cross-check uses the runtime's own phase times, not the CSV.
    `n` is the frame count printed on that line. Lines without a match are
    ignored, including ones that carry host names. No match at all is an error.
    """
    try:
        text = Path(path).read_text(encoding='utf-8', errors='replace')
    except OSError as error:
        raise ValueError(f'cannot read {basename(path)}: {error.strerror}') from error
    rows = []
    for match in STATS.finditer(text):
        n, total, events, timers, jobs, raf, swap = match.groups()
        rows.append({
            'n': int(n), 'total_ms': float(total), 'events_ms': float(events),
            'timers_ms': float(timers), 'jobs_ms': float(jobs),
            'raf_ms': float(raf), 'swap_ms': float(swap),
        })
    if not rows:
        raise ValueError(f'{basename(path)}: no GameLoopStats perf lines')
    return rows


def parse_frame_csv(path):
    """Parse frame-time CSV into rows with total_ms and js_ms (milliseconds).

    Why: mean/p95 frame time and the displayed JS update time come from this
    table (DEC-M0-T018b). Comment lines starting with # are skipped so a leak
    in a comment cannot become a column. Requires `total_ms` and the JS phase
    columns `timers_ms`, `jobs_ms`, `raf_ms`.
    """
    try:
        handle = Path(path).open(encoding='utf-8', newline='')
    except OSError as error:
        raise ValueError(f'cannot read {basename(path)}: {error.strerror}') from error
    rows = []
    with handle:
        filtered = (line for line in handle if not line.lstrip().startswith('#'))
        reader = csv.DictReader(filtered)
        if not reader.fieldnames or 'total_ms' not in reader.fieldnames:
            raise ValueError(f'{basename(path)}: need a total_ms column')
        for field in ('timers_ms', 'jobs_ms', 'raf_ms'):
            if field not in reader.fieldnames:
                raise ValueError(f'{basename(path)}: need a {field} column')
        for raw in reader:
            total = float(raw['total_ms'])
            js_ms = float(raw['timers_ms']) + float(raw['jobs_ms']) + float(raw['raf_ms'])
            rows.append({'total_ms': total, 'js_ms': js_ms})
    if not rows:
        raise ValueError(f'{basename(path)}: no frame rows')
    return rows


def mean(values):
    """Arithmetic mean of a nonempty sequence of milliseconds."""
    return sum(values) / len(values)


def percentile(values, p):
    """Nearest-rank percentile of `values` (milliseconds); `p` is in 0..100.

    Why: p95 frame time is the figure Gate G1 / §14 readers compare, and
    nearest-rank needs no extra dependency. Empty input raises ValueError.
    """
    if not values:
        raise ValueError('percentile of an empty series')
    ordered = sorted(values)
    rank = max(1, math.ceil(p / 100.0 * len(ordered)))
    return ordered[min(rank, len(ordered)) - 1]


def js_ms_from_stats(rows):
    """Weighted mean of timers+jobs+rAF over GameLoopStats log rows, in ms.

    Why: that sum is the JS work GameLoopStats actually timed (DEC-M0-T018b).
    Each row's `n` is the frame count it averages. `rows` is nonempty.
    """
    weight = sum(row['n'] for row in rows)
    total = sum(row['n'] * (row['timers_ms'] + row['jobs_ms'] + row['raf_ms'])
                for row in rows)
    return total / weight if weight else 0.0


def phase_means(rows):
    """Weighted mean ms per GameLoopStats phase; used only for the phase table."""
    weight = sum(row['n'] for row in rows) or 1
    out = {}
    for key in ('events_ms', 'timers_ms', 'jobs_ms', 'raf_ms', 'swap_ms', 'total_ms'):
        out[key] = sum(row['n'] * row[key] for row in rows) / weight
    return out


def attributed_js(buckets, mean_frame_ms):
    """Return (JS self percent, JS milliseconds) from attribution buckets.

    Why: H1 and the 15% cross-check need JS time in the same unit as
    GameLoopStats. JS buckets are QuickJS, GC, shim JS and game JS. Missing
    ids count as zero. `mean_frame_ms` is milliseconds, nonnegative.
    """
    total_w = sum(float(row.get('self_weight') or 0) for row in buckets)
    js_w = sum(float(row.get('self_weight') or 0) for row in buckets
               if row.get('id') in JS_BUCKETS)
    if total_w > 0:
        pct = 100.0 * js_w / total_w
    else:
        pct = sum(float(row.get('self_pct') or 0) for row in buckets
                  if row.get('id') in JS_BUCKETS)
    return pct, mean_frame_ms * pct / 100.0


def relative_gap_pct(attributed_ms, stats_ms):
    """Return |attributed - stats| / stats as a percent of GameLoopStats JS time.

    Why: the card flags a gap of more than 15% against GameLoopStats, so that
    is the denominator. Both arguments are milliseconds, nonnegative. A zero
    stats time with nonzero attributed time is 100%.
    """
    if stats_ms == 0:
        return 0.0 if attributed_ms == 0 else 100.0
    return abs(attributed_ms - stats_ms) / stats_ms * 100.0


def summarise(attr, frames, stats):
    """Compute mean/p95/JS times and the 15% gap flag from parsed inputs.

    Why: Markdown and any later writer must share one set of numbers.
    `frames` and `stats` are nonempty; times are milliseconds.
    """
    totals = [row['total_ms'] for row in frames]
    mean_ms = mean(totals)
    attr_js_pct, attr_js_ms = attributed_js(attr['buckets'], mean_ms)
    stats_js_ms = js_ms_from_stats(stats)
    gap_pct = relative_gap_pct(attr_js_ms, stats_js_ms)
    return {
        'mean_ms': mean_ms,
        'p95_ms': percentile(totals, 95),
        'js_update_ms': mean([row['js_ms'] for row in frames]),
        'attr_js_pct': attr_js_pct,
        'attr_js_ms': attr_js_ms,
        'stats_js_ms': stats_js_ms,
        'gap_pct': gap_pct,
        'flagged': gap_pct > GAP_LIMIT_PCT,
        'phases': phase_means(stats),
    }


def format_machine_value(key, value):
    """Render one snapshot value; battery figures get a percent unit."""
    if key in ('battery_start_pct', 'battery_end_pct'):
        try:
            return f'{float(value):g} %'
        except (TypeError, ValueError):
            return strip_paths(value)
    return strip_paths(value)


def render_header():
    """SPDX header plus title; no host, user or path belongs here."""
    return '''<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Map 92 QuickJS frame profile (spec section 6.1). Generated by
tools/perf/map92_report.py from a local run; never paste a host name,
user name or absolute path into this file.
-->

# Map 92 profile (QuickJS, M0)

QuickJS Map 92 baseline for hypothesis H1 and Gate G1 planning. Measured on
dev-laptop. This file is generated; do not hand-edit numbers.
'''


def render_machine(machine):
    """§14.4 machine-state table from the five snapshot fields only."""
    lines = [
        '## Machine state (spec section 14.4)',
        '',
        '| Field | Value |',
        '| --- | --- |',
    ]
    for key, label in MACHINE_KEYS:
        lines.append(f'| {label} | {format_machine_value(key, machine[key])} |')
    lines.append('')
    return '\n'.join(lines)


def render_frames(summary):
    """Mean, p95 and JS update time in milliseconds, two decimal places."""
    return (
        '## Frame times\n'
        '\n'
        f"- Mean frame time: {summary['mean_ms']:.2f} ms\n"
        f"- p95 frame time: {summary['p95_ms']:.2f} ms\n"
        f"- JS update time: {summary['js_update_ms']:.2f} ms "
        '(GameLoopStats timers + jobs + rAF, from the frame-time CSV)\n'
        '\n'
    )


def render_phases(summary):
    """Weighted GameLoopStats phase means from the runtime log."""
    phases = summary['phases']
    lines = [
        '## GameLoopStats phases',
        '',
        '| Phase | Mean ms |',
        '| --- | ---: |',
        f"| events | {phases['events_ms']:.2f} |",
        f"| timers | {phases['timers_ms']:.2f} |",
        f"| jobs | {phases['jobs_ms']:.2f} |",
        f"| rAF | {phases['raf_ms']:.2f} |",
        f"| swap | {phases['swap_ms']:.2f} |",
        f"| total | {phases['total_ms']:.2f} |",
        '',
    ]
    return '\n'.join(lines)


def render_buckets(attr):
    """§6.1 bucket table using the attribution file's self and total percents."""
    lines = [
        '## Attribution buckets',
        '',
        'Self percentages are the spec section 6.1 split and should sum to 100.',
        '',
        '| Bucket | Self % | Total % |',
        '| --- | ---: | ---: |',
    ]
    for row in attr['buckets']:
        label = strip_paths(row.get('label') or row['id']).replace('|', '/')
        self_pct = float(row['self_pct'])
        total_pct = float(row.get('total_pct') if 'total_pct' in row else self_pct)
        lines.append(f'| {label} | {self_pct:.2f} | {total_pct:.2f} |')
    lines += ['']
    return '\n'.join(lines)


def render_cross_check(summary):
    """Attributed JS vs GameLoopStats JS, with the 15% FLAG when they diverge."""
    lines = [
        '## JS time cross-check',
        '',
        f"- Attributed JS time: {summary['attr_js_ms']:.2f} ms "
        f"({summary['attr_js_pct']:.2f} % of mean frame; QuickJS + GC + shim JS + game JS)",
        f"- GameLoopStats JS time: {summary['stats_js_ms']:.2f} ms "
        '(timers + jobs + rAF)',
        f"- Gap: {summary['gap_pct']:.2f} % of GameLoopStats JS time",
    ]
    if summary['flagged']:
        lines.append(
            '- FLAG: attributed JS time and GameLoopStats JS time differ by more than 15%')
    else:
        lines.append('- Attributed JS time and GameLoopStats JS time agree within 15%.')
    lines.append('')
    return '\n'.join(lines)


def render_conclusions(summary):
    """Gate G1 planning notes derived only from this run's numbers."""
    js_pct = summary['attr_js_pct']
    native_pct = 100.0 - js_pct
    lean = 'more' if js_pct >= native_pct else 'less'
    if summary['flagged']:
        agree = ('Do not treat the JS split as settled until attribution and '
                 'GameLoopStats agree.')
    else:
        agree = ('Attribution and GameLoopStats JS time agree closely enough to '
                 'plan Gate G1 against this split.')
    return (
        '## Conclusions for Gate G1 planning\n'
        '\n'
        'This is the QuickJS Map 92 baseline (spec section 6.1), not a V8 result. '
        'Gate G1 (M1) is **STOP** if V8 Map 92 JS time is above 12 ms mean.\n'
        '\n'
        f"- Interpreter + GC + shim JS + game JS is {js_pct:.2f} % of attributed "
        f"self time ({summary['attr_js_ms']:.2f} ms of a {summary['mean_ms']:.2f} ms "
        'mean frame).\n'
        f'- Native work (Canvas2D, renderer/GPU, audio, video, libc/other) is '
        f'{native_pct:.2f} %.\n'
        f'- Hypothesis H1 (most of the frame is interpreter time V8 would remove) '
        f'is {lean} plausible when JS buckets dominate native ones.\n'
        f"- GameLoopStats JS time is {summary['stats_js_ms']:.2f} ms.\n"
        f'- {agree}\n'
        '- This report is not a Gate G1 pass or fail; G1 is measured on V8 after M1.\n'
    )


def build_report(attr, machine, summary):
    """Assemble the Markdown report; section order is part of the test contract."""
    parts = [
        render_header(),
        render_machine(machine),
        render_frames(summary),
        render_phases(summary),
        render_buckets(attr),
        render_cross_check(summary),
        render_conclusions(summary),
    ]
    return '\n'.join(parts).rstrip() + '\n'


def parse_args(argv):
    """CLI for the four run inputs and the Markdown output path."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--attribution', required=True,
                        help='JSON from tools/perf/attribute.py')
    parser.add_argument('--stats', required=True,
                        help='runtime log containing GameLoopStats perf lines')
    parser.add_argument('--frames', required=True, help='frame-time CSV')
    parser.add_argument('--machine-state', required=True,
                        help='JSON snapshot with the §14.4 fields')
    parser.add_argument('-o', '--output', required=True,
                        help='Markdown report to write')
    return parser.parse_args(argv)


def main(argv=None):
    """Load inputs, write the report, return the process exit status."""
    args = parse_args(argv)
    try:
        attr = load_attribution(args.attribution)
        frames = parse_frame_csv(args.frames)
        stats = parse_stats_log(args.stats)
        machine = load_machine_state(args.machine_state)
        summary = summarise(attr, frames, stats)
        text = build_report(attr, machine, summary)
        Path(args.output).write_text(text, encoding='utf-8')
    except (OSError, ValueError, KeyError, csv.Error, TypeError) as error:
        print(f'map92_report: {strip_paths(str(error))}', file=sys.stderr)
        return 2
    return 0


if __name__ == '__main__':
    sys.exit(main())
