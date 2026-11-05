# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Run tools/perf/map92_report.py on synthetic Map 92 inputs (no game).

Why: M0-T018 ships the report tool and recipe, not a real profile. These tests
pin the §14.4 machine-state block, bucket percentages, mean/p95/JS update
figures, the 15% GameLoopStats gap flag, privacy stripping, and that every
command the recipe names exists in the tree. Outputs go to temp dirs.
"""
from __future__ import annotations

import csv
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / 'tools/perf/map92_report.py'
SCAN = ROOT / 'tools/privacy/scan.py'
RECIPE = ROOT / 'docs/perf/m0-map92-profile-recipe.md'
COMMITTED_REPORT = ROOT / 'docs/perf/m0-map92-profile.md'
RECIPE_TOOL = re.compile(
    r'(?:^|[\s`"\'])((?:tools|src)/[A-Za-z0-9_./-]+)', re.M)
_SPEC = importlib.util.spec_from_file_location('map92_report', TOOL)
MAP = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(MAP)
# The report sums every id in JS_BUCKETS; the interpreter share is the one id
# that is not GC, shim JS or game JS. Derive it so the test does not pin a name
# that M1-T027 may rename.
INTERPRETER = next(bucket_id for bucket_id in sorted(MAP.JS_BUCKETS)
                   if bucket_id not in ('gc', 'shim_js', 'game_js'))

# Ten scored frames used by the matching tests. Mean total 11.00 ms, nearest-rank
# p95 20.00 ms, mean JS update (timers+jobs+rAF) 7.50 ms.
FRAME_TOTALS = [10.0] * 9 + [20.0]
FRAME_JS = [7.0] * 9 + [12.0]
MEAN_TOTAL = 11.00
P95_TOTAL = 20.00
MEAN_JS = 7.50


def leaky_posix(user):
    """Build a POSIX home path at runtime so this file stays path-free."""
    return os.sep + os.sep.join(('home', user, 'games'))


def leaky_windows(user):
    """Build a Windows home path at runtime so this file stays path-free."""
    return 'C:' + '\\' + '\\'.join(('Users', user, 'run'))


def run_tool(*args, env=None):
    """Run the report CLI as a subprocess so the tests cover the real contract."""
    merged = dict(os.environ, PRIVACY_DENYLIST='')
    if env:
        merged.update(env)
    return subprocess.run([sys.executable, str(TOOL), *args], capture_output=True,
                          text=True, check=False, env=merged)


def scan_paths(*paths, denylist):
    """Run tools/privacy/scan.py --paths on generated files with a temp list."""
    return subprocess.run(
        [sys.executable, str(SCAN), '--denylist', str(denylist), '--paths',
         *map(str, paths)],
        capture_output=True, text=True, check=False,
        env=dict(os.environ, PRIVACY_DENYLIST=''))


def attribution(js_pct=70.0, leak=None):
    """Return attribute.py-shaped JSON with JS self% `js_pct` of total weight.

    Remaining weight goes to native buckets so self percentages sum to 100.
    `leak` is an optional string stuffed into `source` to prove it is stripped.
    """
    js_weight = int(js_pct * 100)
    native = 10000 - js_weight
    weights = {
        INTERPRETER: js_weight * 40 // 70, 'gc': js_weight * 10 // 70,
        'shim_js': js_weight * 5 // 70, 'game_js': js_weight * 15 // 70,
        'canvas2d': native * 10 // 30, 'renderer': native * 10 // 30,
        'audio': native * 2 // 30, 'video': 0,
    }
    weights['other'] = 10000 - sum(weights.values())
    labels = {
        INTERPRETER: 'JS interpreter', 'gc': 'GC', 'shim_js': 'shim JS',
        'game_js': 'game JS', 'canvas2d': 'Canvas2D',
        'renderer': 'renderer/GPU submit', 'audio': 'audio', 'video': 'video',
        'other': 'libc/other',
    }
    buckets = []
    for bid, label in labels.items():
        weight = weights[bid]
        buckets.append({
            'id': bid, 'label': label, 'self_weight': weight,
            'self_pct': round(100.0 * weight / 10000, 2),
            'total_pct': round(100.0 * weight / 10000, 2),
            'self_samples': 1, 'total_samples': 1, 'total_weight': weight,
        })
    return {'source': leak or 'run.perf.txt', 'buckets': buckets,
            'diagnostics': {'samples': 10, 'weight': 10000}, 'top_leaves': []}


def write_frames(path, leak=None):
    """Write the ten-frame CSV; an optional leak string goes in a comment line."""
    lines = []
    if leak:
        lines.append(f'# {leak}')
    handle = path.open('w', encoding='utf-8', newline='')
    with handle:
        if lines:
            handle.write('\n'.join(lines) + '\n')
        writer = csv.writer(handle)
        writer.writerow(['frame', 'total_ms', 'events_ms', 'timers_ms',
                         'jobs_ms', 'raf_ms', 'swap_ms'])
        for index, (total, js) in enumerate(zip(FRAME_TOTALS, FRAME_JS), 1):
            raf = js - 2.0
            writer.writerow([index, total, 1.0, 1.0, 1.0, raf, total - js - 1.0])


def write_stats(path, leak=None):
    """Write GameLoopStats log lines that match the CSV, plus an optional leak."""
    lines = []
    if leak:
        lines.append(f'[2026-11-01 12:00:00] [INFO] boot from {leak}')
    for total, js in zip(FRAME_TOTALS, FRAME_JS):
        raf = js - 2.0
        swap = total - js - 1.0
        lines.append(
            f'[2026-11-01 12:00:00] [INFO] perf: 1 frames avg {total:.1f} ms '
            f'(events 1.0, timers 1.0, jobs 1.0, rAF {raf:.1f}, swap {swap:.1f})')
    path.write_text('\n'.join(lines) + '\n', encoding='utf-8')


def write_machine(path, extra=None):
    """Write the §14.4 snapshot; `extra` keys are ignored by the tool on purpose."""
    data = {
        'power_source': 'battery', 'profile': 'power-saver',
        'battery_start_pct': 82, 'battery_end_pct': 79, 'epp': 'power',
    }
    if extra:
        data.update(extra)
    path.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')


def generate(tmp, js_pct=70.0, leak=None, extra_machine=None):
    """Write one synthetic run into `tmp` and return (output path, CLI result)."""
    attr = tmp / 'attr.json'
    frames = tmp / 'frames.csv'
    stats = tmp / 'runtime.log'
    machine = tmp / 'machine.json'
    out = tmp / 'report.md'
    attr.write_text(json.dumps(attribution(js_pct, leak)) + '\n', encoding='utf-8')
    write_frames(frames, leak)
    write_stats(stats, leak)
    write_machine(machine, extra_machine)
    result = run_tool('--attribution', str(attr), '--stats', str(stats),
                      '--frames', str(frames), '--machine-state', str(machine),
                      '-o', str(out))
    return out, result


class Map92ReportTests(unittest.TestCase):
    """Pin the Map 92 report tool on synthetic fixture inputs."""

    def test_report_fields_match_fixture(self):
        """Machine-state, bucket %, mean/p95 and JS update match the fixture."""
        with tempfile.TemporaryDirectory() as tmp:
            out, result = generate(Path(tmp))
            self.assertEqual(result.returncode, 0, result.stderr)
            text = out.read_text(encoding='utf-8')
        lowered = text.lower()
        for field in ('power source', 'profile', 'battery start', 'battery end',
                      'epp'):
            self.assertIn(field, lowered)
        self.assertIn('battery', lowered)
        self.assertIn('power-saver', text)
        self.assertIn('82', text)
        self.assertIn('79', text)
        self.assertRegex(text, r'(?i)\bepp\b.*\bpower\b')
        section = text.split('## Attribution buckets', 1)[1].split('\n## ', 1)[0]
        percents = [float(m) for m in re.findall(
            r'^\| [^|]+ \| ([0-9.]+) \|', section, re.M)]
        self.assertGreaterEqual(len(percents), 9)
        self.assertAlmostEqual(sum(percents[:9]), 100.0, delta=0.5)
        self.assertIn(f'{MEAN_TOTAL:.2f} ms', text)
        self.assertIn(f'{P95_TOTAL:.2f} ms', text)
        self.assertRegex(text, rf'JS update time:\s*{MEAN_JS:.2f} ms')
        self.assertIn('Conclusions', text)

    def test_js_gap_within_15_percent(self):
        """A ~3% JS-time gap is recorded without the FLAG line."""
        with tempfile.TemporaryDirectory() as tmp:
            out, result = generate(Path(tmp), js_pct=70.0)
            self.assertEqual(result.returncode, 0, result.stderr)
            text = out.read_text(encoding='utf-8')
        self.assertNotIn('FLAG:', text)
        self.assertIn('within 15%', text)
        self.assertIn('7.50 ms', text)

    def test_js_gap_flagged_over_15_percent(self):
        """A >15% JS-time gap is flagged in the report; the tool still exits 0."""
        with tempfile.TemporaryDirectory() as tmp:
            out, result = generate(Path(tmp), js_pct=40.0)
            self.assertEqual(result.returncode, 0, result.stderr)
            text = out.read_text(encoding='utf-8')
        self.assertIn(
            'FLAG: attributed JS time and GameLoopStats JS time differ by more than 15%',
            text)
        self.assertIn('7.50 ms', text)

    def test_privacy_scan_strips_host_user_and_paths(self):
        """Host name, user name and absolute paths in inputs never reach the report."""
        user, host = 'fixtureuser', 'fixturehost'
        leak = ' '.join((leaky_posix(user), leaky_windows(user), user, host))
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            deny = tmp_path / 'deny.txt'
            deny.write_text(f'{user}\n{host}\n', encoding='utf-8')
            out, result = generate(
                tmp_path, leak=leak,
                extra_machine={'hostname': host, 'user': user, 'log_dir': leak})
            self.assertEqual(result.returncode, 0, result.stderr)
            text = out.read_text(encoding='utf-8')
            for token in (user, host, leaky_posix(user), leaky_windows(user)):
                self.assertNotIn(token, text)
            self.assertNotIn(token_home_prefix(), text)
            scanned = scan_paths(out, denylist=deny)
        self.assertEqual(scanned.returncode, 0, scanned.stdout + scanned.stderr)

    def test_recipe_commands_exist(self):
        """Every tools/ or src/ path the recipe names is present in the tree."""
        self.assertTrue(RECIPE.is_file())
        text = RECIPE.read_text(encoding='utf-8')
        named = recipe_paths(text)
        self.assertIn('tools/perf/attribute.py', named)
        self.assertIn('tools/perf/map92_report.py', named)
        self.assertIn('tools/share/make_bundle.py', named)
        missing = [path for path in named if not (ROOT / path).exists()]
        self.assertEqual(missing, [])
        self.assertIn('power-saver', text)
        self.assertIn('10 s', text)
        self.assertIn('60 s', text)
        self.assertIn('SMZUGGLER_GAME_DIR', text)
        self.assertFalse(COMMITTED_REPORT.exists())

    def test_missing_input_fails_loudly(self):
        """A missing input is exit 2 with a basename, not a silent empty report."""
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            result = run_tool('--attribution', str(tmp_path / 'missing.json'),
                              '--stats', str(tmp_path / 'missing.log'),
                              '--frames', str(tmp_path / 'missing.csv'),
                              '--machine-state', str(tmp_path / 'missing.json'),
                              '-o', str(tmp_path / 'out.md'))
        self.assertEqual(result.returncode, 2)
        self.assertIn('missing.json', result.stderr)
        self.assertNotRegex(result.stderr, r'(?:/home/|/Users/)')


def token_home_prefix():
    """Return the `/home/` prefix as joined parts so this source stays path-free."""
    return os.sep + 'home' + os.sep


def recipe_paths(text):
    """Return tools/ and src/ paths named in the recipe, including code spans."""
    found = []
    for match in RECIPE_TOOL.finditer(text):
        path = match.group(1).rstrip('.,;:`\'")')
        if path not in found:
            found.append(path)
    return found


if __name__ == '__main__':
    unittest.main()
