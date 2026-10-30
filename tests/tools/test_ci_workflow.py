# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Guard the game-free CI contract described in docs/testing.md.
# Parse YAML rather than matching formatting; build must precede both suites.

from pathlib import Path
import re
import unittest

import yaml

ROOT = Path(__file__).resolve().parents[2]


class CIWorkflowTests(unittest.TestCase):
    """Keep all three desktop jobs useful without a local game installation."""

    def test_workflow_contract(self):
        """Reject missing platforms, mutable actions, game paths or omitted suites."""
        text = (ROOT / '.github/workflows/ci.yml').read_text(encoding='utf-8')
        workflow = yaml.safe_load(text)
        self.assertEqual(set(workflow['on']), {'push', 'pull_request'})
        self.assertEqual(set(workflow['jobs']), {'linux', 'windows', 'macos'})
        for name, job in workflow['jobs'].items():
            with self.subTest(platform=name):
                self.assertIn({'linux': 'ubuntu', 'windows': 'windows',
                               'macos': 'macos'}[name], job['runs-on'])
                steps = job['steps']
                scripts = [step.get('run', '') for step in steps]
                configure = next(i for i, s in enumerate(scripts) if 'cmake -S' in s)
                build = next(i for i, s in enumerate(scripts) if 'cmake --build' in s)
                ctest = next(i for i, s in enumerate(scripts) if 'ctest --test-dir' in s)
                python = next(i for i, s in enumerate(scripts) if 'unittest discover' in s)
                self.assertIn('-DRMMZ_BUILD_TESTS=ON', scripts[configure])
                self.assertLess(configure, build)
                self.assertLess(build, ctest)
                self.assertLess(ctest, python)
                for directory in ('tools/privacy', 'tools/commitmsg', 'tests/tools'):
                    self.assertIn(directory, scripts[python])
                for step in steps:
                    if 'uses' in step:
                        self.assertRegex(step['uses'], r'^[\w.-]+/[\w./-]+@[0-9a-f]{40}$')
                    self.assertFalse(step.get('continue-on-error', False))
                self.assertFalse(job.get('continue-on-error', False))
        self.assertIsNone(re.search(r'SMZUGGLER_GAME_DIR|RMMZ_TEST_GAME_DIR', text))


if __name__ == '__main__':
    unittest.main()
