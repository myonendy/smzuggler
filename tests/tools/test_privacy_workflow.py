# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Verify the CI wiring without external YAML dependencies or network access.
# This guards the privacy boundary described in docs/privacy.md: missing
# secrets must fail, commit ranges must be complete, and reports stay local.

import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
WORKFLOW = ROOT / '.github/workflows/privacy.yml'


def script_for(text, name):
    """Return a named CI step's shell script for local, isolated execution."""
    section = text.split(f'      - name: {name}\n', 1)[1]
    block = section.split('        run: |\n', 1)[1]
    lines = []
    for line in block.splitlines():
        if line and not line.startswith('          '):
            break
        lines.append(line[10:])
    return '\n'.join(lines) + '\n'


class PrivacyWorkflowTests(unittest.TestCase):
    """Exercise the workflow contract without running GitHub Actions."""

    def test_events_and_fail_closed_configuration(self):
        """Require both events, full history, and unconditional failing checks."""
        text = WORKFLOW.read_text()
        self.assertRegex(text, r'on:\s*\n  push:\s*\n  pull_request:')
        self.assertIn('  privacy-scan:', text)
        self.assertIn('  commit-subjects:', text)
        self.assertEqual(text.count('fetch-depth: 0'), 2)
        self.assertNotIn('continue-on-error', text)
        self.assertNotRegex(text, r'(?m)^\s*if:')
        self.assertIn('PRIVACY_DENYLIST: ${{ secrets.PRIVACY_DENYLIST }}', text)
        self.assertNotIn('echo', text)
        self.assertNotIn('set -x', text)

    def run_step(self, name, event, secret='blocked-example-token'):
        """Run real shell wiring with recording tool stubs in a temporary checkout."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            checkout = root / 'checkout'
            checkout.mkdir()
            runner = root / 'runner'
            runner.mkdir()
            for tool in ('privacy/scan.py', 'commitmsg/check_subject.py'):
                path = checkout / 'tools' / tool
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text('import os, sys\n'
                                'from pathlib import Path\n'
                                'with open(os.environ["CALLS"], "a") as out:\n'
                                '    out.write(repr(sys.argv[1:]) + "\\n")\n'
                                'if "--denylist" in sys.argv:\n'
                                '    p = Path(sys.argv[sys.argv.index("--denylist") + 1])\n'
                                '    assert "PRIVACY_DENYLIST" not in os.environ\n'
                                '    assert p.parent == Path(os.environ["RUNNER_TEMP"])\n'
                                '    assert not p.stat().st_mode & 0o077\n'
                                '    if not p.read_text().strip(): sys.exit(2)\n')
            reports = checkout / 'reports'
            reports.mkdir()
            (reports / 'summary with spaces.md').write_text('profile: dev-laptop\n')
            calls = root / 'calls'
            env = dict(os.environ, RUNNER_TEMP=str(runner), CALLS=str(calls),
                       PRIVACY_DENYLIST=secret, EVENT_NAME=event,
                       BASE='base-sha', HEAD='head-sha', BEFORE='before-sha',
                       AFTER='after-sha')
            result = subprocess.run(['bash', '-euo', 'pipefail', '-c',
                                     script_for(WORKFLOW.read_text(), name)],
                                    cwd=checkout, env=env, capture_output=True, text=True)
            self.assertEqual(list(runner.iterdir()), [])
            return result, calls.read_text() if calls.exists() else ''

    def test_privacy_ranges_reports_and_secret(self):
        """Check tree, PR commits, report paths and secret confinement at runtime."""
        for event in ('push', 'pull_request'):
            with self.subTest(event=event):
                result, calls = self.run_step('Scan privacy', event)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("'--tree', 'HEAD'", calls)
                self.assertIn("'--denylist'", calls)
                self.assertIn("'--paths'", calls)
                self.assertIn('summary with spaces.md', calls)
                if event == 'pull_request':
                    self.assertIn("'--range', 'base-sha..head-sha'", calls)
                self.assertNotIn('blocked-example-token', result.stdout + result.stderr)

    def test_missing_secret_fails(self):
        """An unavailable repository secret must reach the scanner and exit 2."""
        result, calls = self.run_step('Scan privacy', 'pull_request', secret='')
        self.assertEqual(result.returncode, 2)
        self.assertIn("'--denylist'", calls)

    def test_commit_subject_ranges(self):
        """Check subjects over actual event endpoints rather than a merge commit."""
        for event, expected in (('push', 'before-sha..after-sha'),
                                ('pull_request', 'base-sha..head-sha')):
            with self.subTest(event=event):
                result, calls = self.run_step('Check commit subjects', event)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn(repr(['--range', expected]), calls)


if __name__ == '__main__':
    unittest.main()
