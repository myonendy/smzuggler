#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Attribute `perf script` call-graph samples to the spec section 6.1 buckets.

Why this exists: the M0 experiment of spec section 6.1 asks how much of a
QuickJS frame is interpreter time that V8 would remove and how much is native
work (Canvas2D, renderer, GC, audio, video) that it would not. perf reports
symbols and DSOs; this tool folds them into those buckets with the rules in
tools/perf/buckets.toml and writes JSON and a Markdown table with self and
total percentages. It reads text only, so it never needs root.
See docs/perf.md for recording, usage and the bucket rules.

Rules a reader must not break:
  * Every string that reaches the output goes through strip_paths(): DSOs and
    paths inside symbols become basenames, so reports never carry a home
    directory or install path. Reports are still run through
    `tools/privacy/scan.py --paths` before they are shared.
  * Self time goes to the innermost frame a non-folded rule claims; frames no
    rule claims, kernel frames and `[unknown]` frames pass it to their caller.
    A stack nobody claims goes to the fallback bucket.
  * Total time counts a sample once per bucket however many frames match.
  * Weights are the sample periods when perf printed them, otherwise 1.

Exit status: 0 on success, 2 on a usage, input or rules error.
"""
from __future__ import annotations

import argparse
import fnmatch
import json
import re
import sys
import tomllib
from pathlib import Path

DEFAULT_RULES = Path(__file__).resolve().with_name('buckets.toml')
PATH_TOKEN = re.compile(r'(?:file://)?(?:/[^\s/:()\[\]]+)+/([^\s/:()\[\]]+)')
PERIOD = re.compile(r'\d+\.\d+:\s+(\d+)\s+\S+:')
FRAME = re.compile(r'^\s+([0-9a-fA-F]+)\s+(.*)$')
OFFSET = re.compile(r'\+0x[0-9a-fA-F]+$')
KERNEL_ADDRESS = 0xffff800000000000
KERNEL_DSOS = {'[kernel.kallsyms]', '[vdso]', '[vsyscall]'}
TOP_LEAVES = 20


def strip_paths(text):
    """Replace every absolute path in `text` with its basename.

    Why: perf prints DSO paths and V8 JIT symbols carry script URLs, both of
    which can reveal a home directory or install location (spec section 16).
    `text` is any string; the result is a new string. Never fails.
    """
    return PATH_TOKEN.sub(r'\1', text)


def parse_frame(line):
    """Split one indented call-chain line into (address, symbol, dso).

    Why: the DSO is the last parenthesised group, but C++ symbols may contain
    " (" themselves, so the line is split from the right. The symbol loses its
    +0x offset and the DSO becomes a basename. The symbol keeps any path it
    carries (a JIT script URL tells shim from game JS); writers strip it.
    Returns None when the line is not a frame. A missing DSO is "[unknown]".
    """
    match = FRAME.match(line.rstrip())
    if not match:
        return None
    address, rest = int(match.group(1), 16), match.group(2).strip()
    symbol, dso = rest, '[unknown]'
    if rest.endswith(')'):
        body = rest[:-1]
        deleted = body.endswith(' (deleted)')
        if deleted:
            body = body[:-len(' (deleted)')]
        head, sep, tail = body.rpartition(' (')
        if sep:
            symbol, dso = head.strip(), tail.strip()
    symbol = OFFSET.sub('', symbol) or '[unknown]'
    dso = Path(dso).name if dso.startswith('/') else strip_paths(dso)
    return address, symbol, dso or '[unknown]'


def parse_samples(lines):
    """Group `perf script` output into samples of {'weight', 'frames'}.

    Why: perf prints a header line per sample followed by its call chain,
    innermost frame first, and a blank line. `lines` is any iterable of str;
    `#` comment lines are ignored. Frames before any header raise ValueError,
    which the caller reports with the line number.
    """
    samples, current = [], None
    for number, line in enumerate(lines, 1):
        if not line.strip():
            current = None
            continue
        if line.startswith('#'):
            continue
        if line[0].isspace():
            frame = parse_frame(line)
            if current is None or frame is None:
                raise ValueError(f'line {number}: call-chain line outside a sample')
            current['frames'].append(frame)
            continue
        period = PERIOD.search(line)
        current = {'weight': int(period.group(1)) if period else 1, 'frames': []}
        samples.append(current)
    return samples


def load_rules(path):
    """Read and validate buckets.toml; return (buckets, rules, fallback).

    Why: a typo in a rule would silently move time between buckets, so
    unknown bucket ids and bad regexes fail loudly with ValueError naming the
    rule. `buckets` is a list of (id, label) in report order; each rule is a
    dict with 'dso' (list), 'symbol' (compiled regex or None), 'bucket'
    ('' for none) and 'fold'.
    """
    with open(path, 'rb') as handle:
        data = tomllib.load(handle)
    buckets = [(b['id'], b['label']) for b in data.get('bucket', [])]
    ids = {bucket_id for bucket_id, _ in buckets}
    fallback = data.get('fallback', '')
    if fallback not in ids:
        raise ValueError(f'fallback bucket {fallback!r} is not declared')
    rules = []
    for index, raw in enumerate(data.get('rule', []), 1):
        bucket = raw.get('bucket', '')
        if bucket and bucket not in ids:
            raise ValueError(f'rule {index}: unknown bucket {bucket!r}')
        if not raw.get('dso') and not raw.get('symbol'):
            raise ValueError(f'rule {index}: needs dso or symbol')
        try:
            symbol = re.compile(raw['symbol']) if raw.get('symbol') else None
        except re.error as error:
            raise ValueError(f'rule {index}: bad symbol regex: {error}') from None
        rules.append({'dso': list(raw.get('dso', [])), 'symbol': symbol,
                      'bucket': bucket, 'fold': bool(raw.get('fold', False))})
    return buckets, rules, fallback


def dso_matches(patterns, dso):
    """Return True when `dso` equals or fnmatch-matches one of `patterns`.

    Why: names such as "[kernel.kallsyms]" are fnmatch character classes, so
    exact equality is tried first. An empty pattern list matches any DSO.
    """
    if not patterns:
        return True
    return any(dso == p or fnmatch.fnmatchcase(dso, p) for p in patterns)


def classify(frame, rules):
    """Return the first rule matching `frame` (address, symbol, dso), or None.

    Why: first match wins, which lets specific rules shadow broad ones. Frames
    at kernel addresses count as "[kernel.kallsyms]" even when perf could not
    name the DSO, so kernel time is never mistaken for an unknown library.
    """
    address, symbol, dso = frame
    if address >= KERNEL_ADDRESS and dso == '[unknown]':
        dso = '[kernel.kallsyms]'
    for rule in rules:
        if not dso_matches(rule['dso'], dso):
            continue
        if rule['symbol'] is None or rule['symbol'].search(symbol):
            return rule
    return None


def is_kernel(frame):
    """Return True when a frame lies in the kernel by DSO or address."""
    return frame[2] in KERNEL_DSOS or frame[0] >= KERNEL_ADDRESS


def attribute_sample(frames, rules, fallback):
    """Return (self bucket, set of total buckets, claimed) for one call chain.

    Why: this is the attribution policy of the module docstring in one place.
    `frames` runs innermost first; an empty chain goes to `fallback`.
    `claimed` is False when no non-folded rule matched any frame.
    """
    self_bucket, totals = None, set()
    for frame in frames:
        rule = classify(frame, rules)
        if rule is None or not rule['bucket']:
            continue
        totals.add(rule['bucket'])
        if self_bucket is None and not rule['fold']:
            self_bucket = rule['bucket']
    claimed = self_bucket is not None
    if not claimed:
        self_bucket = fallback
    totals.add(self_bucket)
    return self_bucket, totals, claimed


def percent(part, whole):
    """Return `part` as a percentage of `whole`, rounded to 2 places; 0 if empty."""
    return round(100.0 * part / whole, 2) if whole else 0.0


def summarise(samples, buckets, rules, fallback):
    """Attribute every sample and return the JSON-ready report dict.

    Why: one structure feeds both the JSON and the Markdown writers so they can
    never disagree. Counts are sample counts; weights are summed periods.
    Diagnostics count kernel-leaf samples, `[unknown]` frames and stacks no
    non-folded rule claimed, which tell the reader how far to trust the split;
    `top_leaves` lists the heaviest innermost frames so rules can be tuned.
    """
    rows = {bid: {'self_samples': 0, 'self_weight': 0, 'total_samples': 0,
                  'total_weight': 0} for bid, _ in buckets}
    diag = {'samples': len(samples), 'weight': 0, 'kernel_samples': 0,
            'unknown_frames': 0, 'unclaimed_samples': 0}
    leaves = {}
    for sample in samples:
        weight, frames = sample['weight'], sample['frames']
        self_bucket, totals, claimed = attribute_sample(frames, rules, fallback)
        diag['weight'] += weight
        diag['kernel_samples'] += bool(frames) and is_kernel(frames[0])
        diag['unknown_frames'] += sum(f[1] == '[unknown]' for f in frames)
        diag['unclaimed_samples'] += not claimed
        if frames:
            leaf = (strip_paths(frames[0][1]), frames[0][2], self_bucket)
            leaves[leaf] = leaves.get(leaf, 0) + weight
        rows[self_bucket]['self_samples'] += 1
        rows[self_bucket]['self_weight'] += weight
        for bucket in totals:
            rows[bucket]['total_samples'] += 1
            rows[bucket]['total_weight'] += weight
    report = []
    for bid, label in buckets:
        row = dict(id=bid, label=label, **rows[bid])
        row['self_pct'] = percent(row['self_weight'], diag['weight'])
        row['total_pct'] = percent(row['total_weight'], diag['weight'])
        report.append(row)
    ranked = sorted(leaves.items(), key=lambda item: (-item[1], item[0]))[:TOP_LEAVES]
    top = [{'symbol': s, 'dso': d, 'bucket': b, 'weight': w,
            'pct': percent(w, diag['weight'])} for (s, d, b), w in ranked]
    return {'buckets': report, 'diagnostics': diag, 'top_leaves': top}


def markdown(report, source):
    """Render the report as a Markdown table plus a diagnostics line.

    Why: the table is what goes into docs/perf.md results and review threads.
    `source` is the input name, path-stripped here as well.
    """
    diag = report['diagnostics']
    lines = [f'# perf attribution: {strip_paths(source)}', '',
             '| Bucket | Self % | Total % | Self samples | Total samples |',
             '| --- | ---: | ---: | ---: | ---: |']
    for row in report['buckets']:
        lines.append(f"| {row['label']} | {row['self_pct']:.2f} | "
                     f"{row['total_pct']:.2f} | {row['self_samples']} | "
                     f"{row['total_samples']} |")
    lines += ['', f"Samples: {diag['samples']}; weight: {diag['weight']}; "
              f"kernel-leaf samples: {diag['kernel_samples']}; "
              f"`[unknown]` frames: {diag['unknown_frames']}; "
              f"unclaimed samples: {diag['unclaimed_samples']}.", '',
              '| Innermost frame | DSO | Self bucket | Self % |',
              '| --- | --- | --- | ---: |']
    for leaf in report['top_leaves']:
        symbol = leaf['symbol'].replace('|', '\\|')
        lines.append(f"| `{symbol}` | {leaf['dso']} | {leaf['bucket']} | "
                     f"{leaf['pct']:.2f} |")
    lines.append('')
    return '\n'.join(lines)


def write_text(path, text):
    """Write `text` to `path`, or to stdout when `path` is "-"."""
    if path == '-':
        sys.stdout.write(text)
    else:
        Path(path).write_text(text, encoding='utf-8')


def main(argv=None):
    """Parse arguments, attribute the input and write the reports.

    Why: the command-line entry point of docs/perf.md. With neither --json nor
    --markdown, the Markdown table goes to stdout. Returns the exit status.
    """
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('input', nargs='?', default='-',
                    help='perf script output (default: stdin)')
    ap.add_argument('--rules', default=str(DEFAULT_RULES), help='bucket rules TOML')
    ap.add_argument('--json', help='write the JSON report here ("-" for stdout)')
    ap.add_argument('--markdown', help='write the Markdown table here ("-" for stdout)')
    args = ap.parse_args(argv)
    try:
        buckets, rules, fallback = load_rules(args.rules)
        if args.input == '-':
            samples = parse_samples(sys.stdin)
        else:
            with open(args.input, encoding='utf-8', errors='replace') as handle:
                samples = parse_samples(handle)
    except (OSError, ValueError, tomllib.TOMLDecodeError) as error:
        print(f'attribute: {strip_paths(str(error))}', file=sys.stderr)
        return 2
    report = summarise(samples, buckets, rules, fallback)
    report['source'] = Path(args.input).name if args.input != '-' else 'stdin'
    if args.json:
        write_text(args.json, json.dumps(report, indent=2, sort_keys=True) + '\n')
    if args.markdown or not args.json:
        write_text(args.markdown or '-', markdown(report, report['source']))
    return 0


if __name__ == '__main__':
    sys.exit(main())
