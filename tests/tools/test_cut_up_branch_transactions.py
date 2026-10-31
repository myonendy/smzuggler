# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Check atomic upstream extraction using local repositories only.

See docs/UPSTREAM.md. Fixtures own temporary files on the test thread;
no remotes or game material are used, even on failed extraction.
"""
import subprocess
import unittest

import test_cut_up_branch as fixtures


class ExtractionTransactions(unittest.TestCase):
    """Protect ordered replay and all-or-nothing branch publication."""

    def setUp(self):
        """Borrow the existing local fixture; release its files after each test."""
        self.fixture = fixtures.CutBranchTests()
        self.fixture.setUp()
        self.addCleanup(self.fixture.doCleanups)

    def candidate(self, content):
        """Commit fixture text synchronously and return its owned SHA string."""
        path = self.fixture.repo / 'src/shims/dom_shim.js'
        path.write_text(content, encoding='utf-8')
        self.fixture.git('add', '.')
        self.fixture.git('commit', '-qm', 'Candidate')
        return self.fixture.git('rev-parse', 'HEAD')

    def extract(self, *commits):
        """Replay ordered SHAs locally; return captured diagnostics on any exit."""
        return subprocess.run([str(fixtures.SCRIPT), 'U1', 'sequence', *commits],
                              cwd=self.fixture.repo, capture_output=True, text=True)

    def test_ordered_commits(self):
        """Each supplied commit survives in order without fork-only ancestry."""
        first = self.candidate('var base = 2;\n')
        second = self.candidate('var base = 3;\n')
        result = self.extract(first, second)
        self.assertEqual(result.returncode, 0, result.stderr)
        git = self.fixture.git
        self.assertEqual(git('rev-parse', 'up/U1-sequence~2'), self.fixture.base)
        self.assertEqual(git('show', 'up/U1-sequence:src/shims/dom_shim.js'),
                         'var base = 3;')
        self.assertIn(first, git('log', '-1', '--format=%B', 'up/U1-sequence^'))
        self.assertIn(second, git('log', '-1', '--format=%B', 'up/U1-sequence'))
        self.assertEqual(git('rev-parse', 'HEAD'), second)

    def test_intermediate_header_is_refused(self):
        """Removing a forbidden header later cannot hide it in PR history."""
        first = self.candidate('// Copyright (C) 2026 The smzuggler contributors\n')
        second = self.candidate('var base = 3;\n')
        result = self.extract(first, second)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('copyright', result.stderr)
        self.assertEqual(self.fixture.git('branch', '--list', 'up/*'), '')
        self.assertEqual(self.fixture.git('rev-parse', 'HEAD'), second)

    def test_existing_branch_is_preserved(self):
        """A destination collision must never overwrite the owner's topic tip."""
        tip = self.candidate('var base = 2;\n')
        self.fixture.git('branch', 'up/U1-sequence', self.fixture.base)
        result = self.extract(tip)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('branch already exists', result.stderr)
        self.assertEqual(self.fixture.git('rev-parse', 'up/U1-sequence'),
                         self.fixture.base)
