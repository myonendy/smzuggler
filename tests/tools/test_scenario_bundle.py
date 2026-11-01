# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Prove an evaluator run directory packs through the real share packager.

The scenario entry point's output layout must be one tools/share/make_bundle.py
accepts (M0-T023). This builds a synthetic run with fake data only, never a
game or a real capture, and drives the real packager by subprocess through
tools.evaluator.smzuggler.bundle. Nothing is posted anywhere.
"""
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.evaluator import smzuggler

CANARY = 'synthcanary'


def png_chunk(kind, data):
    """Return one PNG chunk with its CRC, for building a tiny placeholder shot."""
    body = kind + data
    return struct.pack('>I', len(data)) + body + struct.pack('>I', zlib.crc32(body))


def placeholder_png():
    """Return a valid 1x1 RGB PNG; stands in for a capture, contains no game pixels."""
    header = png_chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 8, 2, 0, 0, 0))
    pixels = png_chunk(b'IDAT', zlib.compress(b'\x00\x10\x20\x30'))
    return b'\x89PNG\r\n\x1a\n' + header + pixels + png_chunk(b'IEND', b'')


class ScenarioBundleTests(unittest.TestCase):
    """Run the real packager on a fabricated run directory in a temp tree."""

    def setUp(self):
        """Create a synthetic run in the scenario's layout and an isolated env."""
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        base = Path(self.tmp.name)
        self.game = base / 'game-copy'
        self.game.mkdir()
        self.run = base / 'state/smzuggler-rt/evaluator/run-synthetic'
        self.run.mkdir(parents=True)
        self.deny = base / 'deny'
        self.deny.write_text(CANARY + '\n')
        results = [{'step': 0, 'status': 'ok', 'start_frame': 0, 'end_frame': 40},
                   {'step': 1, 'status': 'error', 'start_frame': 40, 'end_frame': 41},
                   {'step': 2, 'status': 'not-run'},
                   {'step': 'window-close', 'status': 'not-run'}]
        (self.run / 'summary.json').write_text(json.dumps(results))
        (self.run / 'runtime.log').write_text(
            'perf: 1 frames avg 16.2 ms (events 0.1, timers 0.2, jobs 0.3, rAF 10.0, swap 5.6)\n')
        smzuggler.timing_report(self.run)
        (self.run / 'steps.csv').write_text('step,start_frame,end_frame,wall_ms,status\n0,0,40,650.0,ok\n')
        (self.run / 'shots').mkdir()
        (self.run / 'shots/title.png').write_bytes(placeholder_png())
        (self.run / 'input/save').mkdir(parents=True)
        (self.run / 'input/save/file1.rmmzsave').write_text('fake save')
        env = {'HOME': str(base / 'home'), 'USER': 'fakeaccount',
               'LOGNAME': 'fakeaccount', 'PRIVACY_DENYLIST': ''}
        patcher = patch.dict(os.environ, env)
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_real_packager_writes_bundle_with_step_summary(self):
        """The bundle exists, lists every step's outcome and passes the scanner."""
        status = smzuggler.bundle(self.game, self.run, denylist=self.deny)
        self.assertEqual(status, 0)
        archive_path = self.run / 'bundle.zip'
        self.assertTrue(archive_path.is_file())
        with zipfile.ZipFile(archive_path) as archive:
            names = archive.namelist()
            readme = archive.read('README.txt').decode()
            extract = Path(self.tmp.name) / 'extract'
            archive.extractall(extract)
        for expected in ('runtime.log', 'frame-times.csv', 'perf-report-evaluator.json',
                         'perf-report-steps.json', 'title.png'):
            self.assertTrue(any(n.endswith(expected) for n in names), expected)
        self.assertFalse(any('rmmzsave' in n or '/input/' in n for n in names))
        self.assertIn('Steps (pass/fail):\nstep 0: pass\nstep 1: fail\nstep 2: not run\n'
                      'window-close: not run\n', readme)
        self.assertIn('Collector commit:', readme)
        scan = subprocess.run(
            [sys.executable, str(ROOT / 'tools/privacy/scan.py'), '--denylist', str(self.deny),
             '--paths', *map(str, (p for p in extract.rglob('*') if p.is_file()))],
            capture_output=True, text=True)
        self.assertEqual(scan.returncode, 0, scan.stdout + scan.stderr)

    def test_planted_term_never_reaches_bundle(self):
        """The packager's deny-list is live here: a planted log term is scrubbed."""
        with (self.run / 'runtime.log').open('a') as stream:
            stream.write(CANARY + '\n')
        status = smzuggler.bundle(self.game, self.run, denylist=self.deny)
        self.assertEqual(status, 0)
        with zipfile.ZipFile(self.run / 'bundle.zip') as archive:
            log = [n for n in archive.namelist() if n.endswith('runtime.log')]
            self.assertEqual(len(log), 1)
            for name in archive.namelist():
                self.assertNotIn(CANARY.encode(), archive.read(name).lower())

    def test_step_lines_reject_free_text(self):
        """Unscanned README text is limited to indices and fixed status words."""
        with self.assertRaises(ValueError):
            smzuggler.step_lines([{'step': 'Some caption', 'status': 'ok'}])
        with self.assertRaises(ValueError):
            smzuggler.step_lines([{'step': 0, 'status': 'Some caption'}])


if __name__ == '__main__':
    unittest.main()
