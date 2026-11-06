# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Exercise --scrub-report against fabricated reports, no game or save data.

Why: an outside tester sends a Deck report back, so the archive must keep the
fixed report schema, never a home path, local user/host name or save file, and
must pass tools/privacy/scan.py. Every identity here is fabricated in a
temporary directory. Nothing is uploaded.
"""

import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'tools/perf/scrub_report.py'


class ScrubReportTests(unittest.TestCase):
    """Run the real scrubber with isolated HOME, user and host names."""

    def setUp(self):
        """Own a report tree, a deny-list and an archive path per test."""
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.base = Path(self.tmp.name)
        self.report = self.base / 'report'
        self.report.mkdir()
        self.deny = self.base / 'deny'
        self.deny.write_text('secretcanary\n')
        self.out = self.base / 'deck-report.tar.gz'
        self.home = self.base / 'home'
        self.env = dict(
            os.environ,
            HOME=str(self.home),
            USER='fakeaccount',
            LOGNAME='fakeaccount',
            HOSTNAME='bundlehost',
            PRIVACY_DENYLIST='',
            SMZUGGLER_PRIVACY_DENYLIST='',
        )

    def invoke(self):
        """Run the scrubber and capture diagnostics for leak checks."""
        return subprocess.run(
            [sys.executable, str(SCRIPT), str(self.report),
             '--out', str(self.out), '--denylist', str(self.deny)],
            env=self.env, capture_output=True, text=True)

    def write_schema_report(self):
        """Write one frame stream and one JSON/Markdown report pair."""
        frame = {
            'type': 'frame', 'frame': 1, 'work_ms': 4.5, 'gpu_ms': None,
            'present_interval_ms': 16.7, 'missed_vblank': False,
            'over_budget': False, 'note': f'{self.home}/perf/private',
        }
        summary = {
            'type': 'summary', 'frames': 1, 'refresh_hz': 60.0,
            'budget_ms': 16.666, 'mean_work_ms': 4.5, 'p50_work_ms': 4.5,
            'p95_work_ms': 4.5, 'p99_work_ms': 4.5, 'p99_9_work_ms': 4.5,
            'max_work_ms': 4.5, 'frames_over_budget': 0, 'missed_vblanks': 0,
            'gpu_frames': 0, 'mean_gpu_ms': None,
        }
        marker = {'type': 'marker', 'frame': 1, 'kind': 'scene',
                  'phase': 'point', 'extra': 'dropped'}
        lines = [frame, summary, marker,
                 {'type': 'unknown', 'value': f'{self.home}/perf/'}]
        (self.report / 'frame-stats-steam-deck-S1-run1.jsonl').write_text(
            '\n'.join(json.dumps(item) for item in lines) + '\n')
        report = {
            'schema': 'smzuggler-perf-report/1', 'profile': 'steam-deck',
            'scene': 'S1', 'gate': 'confirm-pass', 'passed': True,
            'warmup_frames': 600, 'scored_frames': 7200, 'refresh_hz': 60.0,
            'frames_total': 7800, 'frames_scored': 7200,
            'frames_excluded': 0, 'frames_counted': 7200,
            'criteria': {
                'p99_work_ms': {'value': 8.0, 'limit': 16.7, 'ok': True},
                'mean_work_ms': {'value': 4.0, 'limit': None, 'ok': True},
            },
            'summary': {'mean_work_ms': 4.0, 'p50_work_ms': 4.0,
                        'p95_work_ms': 4.0, 'p99_work_ms': 8.0},
            'machine_state': {'power_source': 'battery',
                              'profile': 'power-saver'},
            'notes': f'{self.home}/perf/',
            'hostname': 'not-the-local-host',
        }
        (self.report / 'perf-report-steam-deck-S1.json').write_text(
            json.dumps(report))
        (self.report / 'perf-report-steam-deck-S1.md').write_text(
            f'# Perf report: S1 on steam-deck\n\nHome: {self.home}/perf/\n')

    def extract(self):
        """Extract the archive and return its root directory."""
        dest = self.base / 'extract'
        with tarfile.open(self.out, 'r:gz') as archive:
            archive.extractall(dest)
        return dest

    def test_scrub_keeps_schema_and_passes_privacy(self):
        """Keep frame/report fields, write SCRUBBED.txt and expose no identity."""
        self.write_schema_report()
        (self.report / 'notes.txt').write_text(
            f'not part of the schema: {self.home}/perf/\n')
        result = self.invoke()
        self.assertEqual(result.returncode, 0, result.stderr)
        dest = self.extract()
        names = sorted(path.relative_to(dest).as_posix()
                       for path in dest.rglob('*') if path.is_file())
        self.assertIn('SCRUBBED.txt', names)
        self.assertIn('README.txt', names)
        self.assertIn('report/frame-stats-steam-deck-S1-run1.jsonl', names)
        self.assertIn('report/perf-report-steam-deck-S1.json', names)
        self.assertNotIn('report/notes.txt', names)

        packed_json = (dest / 'report/perf-report-steam-deck-S1.json').read_text()
        self.assertNotIn('hostname', packed_json)
        self.assertNotIn(str(self.home), packed_json)
        self.assertIn('~/perf/', packed_json)
        packed_frames = (dest / 'report/frame-stats-steam-deck-S1-run1.jsonl').read_text()
        self.assertNotIn(str(self.home), packed_frames)
        self.assertNotIn('"note"', packed_frames)
        self.assertNotIn('"extra"', packed_frames)
        self.assertNotIn('unknown', packed_frames)

        text = '\n'.join(path.read_text(errors='replace')
                         for path in dest.rglob('*') if path.is_file())
        for identity in (self.env['HOME'], self.env['USER'],
                         self.env['HOSTNAME'], 'secretcanary'):
            self.assertNotIn(identity, text)
        self.assertIn('Home prefixes were written as ~.', text)

        files = [str(path) for path in dest.rglob('*') if path.is_file()]
        scan = subprocess.run(
            [sys.executable, str(ROOT / 'tools/privacy/scan.py'),
             '--denylist', str(self.deny), '--paths', *files],
            env=self.env, capture_output=True, text=True)
        self.assertEqual(scan.returncode, 0, scan.stdout + scan.stderr)

    def test_packs_without_a_project_denylist(self):
        """A tester machine with no deny-list still packs under generic rules."""
        self.write_schema_report()
        result = subprocess.run(
            [sys.executable, str(SCRIPT), str(self.report),
             '--out', str(self.out)],
            env=self.env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(self.out.is_file())
        dest = self.extract()
        files = [str(path) for path in dest.rglob('*') if path.is_file()]
        scan = subprocess.run(
            [sys.executable, str(ROOT / 'tools/privacy/scan.py'),
             '--denylist', str(self.deny), '--paths', *files],
            env=self.env, capture_output=True, text=True)
        self.assertEqual(scan.returncode, 0, scan.stdout + scan.stderr)

    def test_default_deck_identity_preserves_schema(self):
        """Default SteamOS identities must not corrupt fixed profile tokens."""
        self.env.update(USER='deck', LOGNAME='deck', HOSTNAME='steamdeck')
        self.write_schema_report()
        for with_denylist in (True, False):
            with self.subTest(with_denylist=with_denylist):
                args = [sys.executable, str(SCRIPT), str(self.report),
                        '--out', str(self.out)]
                if with_denylist:
                    args += ['--denylist', str(self.deny)]
                result = subprocess.run(args, env=self.env,
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                dest = self.extract()
                report = json.loads((dest / 'report' /
                    'perf-report-steam-deck-S1.json').read_text())
                self.assertEqual(report['profile'], 'steam-deck')
                self.assertEqual(report['gate'], 'confirm-pass')
                text = (dest / 'report' /
                        'perf-report-steam-deck-S1.md').read_text()
                self.assertIn('steam-deck', text)

    def test_proxy_and_identity_substrings_survive(self):
        """Profile enums and larger words must survive local token checks."""
        self.env.update(USER='deck', LOGNAME='deck', HOSTNAME='steamdeck')
        self.write_schema_report()
        for path in self.report.iterdir():
            text = path.read_text().replace('steam-deck', 'deck-proxy')
            path.write_text(text)
            path.rename(path.with_name(path.name.replace('steam-deck',
                                                       'deck-proxy')))
        path = self.report / 'perf-report-deck-proxy-S1.json'
        report = json.loads(path.read_text())
        report.update(profile='deck-proxy', gate='proxy', passed=False,
                      notes='steamdecking decking')
        path.write_text(json.dumps(report))
        result = self.invoke()
        self.assertEqual(result.returncode, 0, result.stderr)
        dest = self.extract()
        packed = json.loads((dest / 'report' / path.name).read_text())
        self.assertEqual(packed['profile'], 'deck-proxy')
        self.assertEqual(packed['gate'], 'proxy')
        self.assertFalse(packed['passed'])
        self.assertEqual(packed['notes'], 'steamdecking decking')

    def test_bare_deck_identity_is_refused(self):
        """A schema vocabulary exemption must not hide a bare user in notes."""
        self.env.update(USER='deck', LOGNAME='deck', HOSTNAME='steamdeck')
        self.write_schema_report()
        path = self.report / 'perf-report-steam-deck-S1.json'
        report = json.loads(path.read_text())
        report['notes'] = 'collected by deck'
        path.write_text(json.dumps(report))
        result = self.invoke()
        self.assertEqual(result.returncode, 1)
        self.assertIn('local identity', result.stderr)
        self.assertFalse(self.out.exists())

    def test_save_file_is_refused(self):
        """A tree holding a project save is never packed, even when nested."""
        self.write_schema_report()
        saves = self.report / 'save'
        saves.mkdir()
        (saves / 'file1.rmmzsave').write_bytes(b'\x00save')
        result = self.invoke()
        self.assertEqual(result.returncode, 1)
        self.assertIn('save file', result.stderr)
        self.assertFalse(self.out.exists())

    def test_local_user_name_is_refused(self):
        """A bare local user name in a kept field blocks the pack."""
        self.write_schema_report()
        report_path = self.report / 'perf-report-steam-deck-S1.json'
        report = json.loads(report_path.read_text())
        report['notes'] = 'collected by fakeaccount'
        report_path.write_text(json.dumps(report))
        result = self.invoke()
        self.assertEqual(result.returncode, 1)
        self.assertIn('local identity', result.stderr)
        self.assertNotIn('fakeaccount', result.stdout + result.stderr)
        self.assertFalse(self.out.exists())

    def test_local_host_name_is_refused(self):
        """A local host name anywhere in the tree blocks the pack."""
        self.write_schema_report()
        (self.report / 'unrelated.txt').write_text('host bundlehost was here\n')
        result = self.invoke()
        self.assertEqual(result.returncode, 1)
        self.assertIn('local identity', result.stderr)
        self.assertNotIn('bundlehost', result.stdout + result.stderr)
        self.assertFalse(self.out.exists())


if __name__ == '__main__':
    unittest.main()
