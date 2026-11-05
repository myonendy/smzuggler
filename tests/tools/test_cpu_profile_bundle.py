# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""The share packager must include and scrub `--cpu-prof` profiles.

Why: profiles written under the state directory are JavaScript samples that
can carry host paths in script URLs. A shared bundle that silently dropped
them would lose the evidence a report cites, and one that packed them raw
would leak the collector's directory layout. This test drives the real CLI
over a fabricated `.cpuprofile` and checks inclusion, JSON validity, the
README description and the privacy scrub. No game files are used.
"""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'tools/share/make_bundle.py'


class CpuProfileBundleTests(unittest.TestCase):
    """Pack one profile in isolated directories with fabricated private terms."""

    def setUp(self):
        """Own temporary inputs and the mandatory deny-list."""
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.base = Path(self.tmp.name)
        self.run_dir = self.base / 'run'
        self.run_dir.mkdir()
        self.deny = self.base / 'deny'
        self.deny.write_text('secretcanary\n')
        self.out = self.base / 'bundle.zip'
        self.env = dict(
            os.environ,
            HOME=str(self.base / 'home'),
            USER='fakeaccount',
            LOGNAME='fakeaccount',
            PRIVACY_DENYLIST='',
        )

    def invoke(self):
        """Run the packager; diagnostics must never echo matched content."""
        return subprocess.run(
            [sys.executable, str(SCRIPT), str(self.run_dir), '-o',
             str(self.out), '--denylist', str(self.deny)],
            env=self.env, capture_output=True, text=True)

    def test_profile_is_collected_and_scrubbed(self):
        """Pack a profiles/*.cpuprofile, keep it valid JSON and scrub host paths."""
        profile = {
            'nodes': [{
                'id': 1,
                'callFrame': {
                    'functionName': 'game_hot_path',
                    'url': 'file:///home/fakeaccount/game/js/main.js',
                },
            }],
            'samples': [1],
            'timeDeltas': [12],
            'note': 'secretcanary',
        }
        nested = self.run_dir / 'profiles'
        nested.mkdir()
        (nested / 'cpu-1-2.cpuprofile').write_text(json.dumps(profile))
        result = self.invoke()
        self.assertEqual(result.returncode, 0, result.stderr)

        archive_name = 'run-1/profiles/cpu-1-2.cpuprofile'
        with zipfile.ZipFile(self.out) as archive:
            self.assertIn(archive_name, archive.namelist())
            readme = archive.read('README.txt').decode()
            self.assertIn(f'{archive_name}: CPU profile', readme)
            packed = archive.read(archive_name).decode()
            for carried in ('fakeaccount', 'secretcanary'):
                self.assertNotIn(carried, packed)
            parsed = json.loads(packed)
            self.assertEqual(parsed['samples'], [1])
            self.assertEqual(parsed['timeDeltas'], [12])
            self.assertEqual(parsed['nodes'][0]['callFrame']['functionName'],
                             'game_hot_path')
            dest = self.base / 'extract'
            archive.extractall(dest)

        scan = subprocess.run(
            [sys.executable, str(ROOT / 'tools/privacy/scan.py'),
             '--denylist', str(self.deny), '--paths',
             *map(str, dest.rglob('*.*'))],
            env=self.env, capture_output=True)
        self.assertEqual(scan.returncode, 0, scan.stderr)


if __name__ == '__main__':
    unittest.main()
