# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Exercise upstream isolation without remotes; see docs/UPSTREAM.md.

All repositories belong to temporary directories and never use game material.
"""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'tools/upstream/cut_up_branch.sh'


class CutBranchTests(unittest.TestCase):
    """Check that only reviewed upstream paths leave the fork."""

    def git(self, *args):
        """Run local Git synchronously; return stdout or raise on failure."""
        return subprocess.check_output(['git', '-C', str(self.repo), *args], text=True).strip()

    def setUp(self):
        """Create independent fork and upstream tips with no network configuration."""
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.repo = pathlib.Path(self.tmp.name)
        self.git('init', '-q', '-b', 'main')
        self.git('config', 'user.name', 'Project contributors')
        self.git('config', 'user.email', 'project@example.invalid')
        (self.repo / 'src/shims').mkdir(parents=True)
        (self.repo / 'src/shims/dom_shim.js').write_text('var base = 1;\n')
        self.git('add', '.')
        self.git('commit', '-qm', 'Base')
        self.base = self.git('rev-parse', 'HEAD')
        self.git('branch', 'upstream-main')
        (self.repo / 'fork-only').write_text('fork\n')
        self.git('add', '.')
        self.git('commit', '-qm', 'Fork only')

    def cut(self, paths):
        """Commit supplied relative contents and invoke the helper; preserve dirty state."""
        for path, content in paths.items():
            (self.repo / path).write_text(content)
        self.git('add', '.')
        self.git('commit', '-qm', 'Candidate')
        tip = self.git('rev-parse', 'HEAD')
        (self.repo / 'dirty').write_text('keep\n')
        before = self.git('status', '--porcelain')
        result = subprocess.run([str(SCRIPT), 'U1', 'dom', tip], cwd=self.repo,
                                text=True, capture_output=True)
        self.assertEqual(self.git('rev-parse', 'HEAD'), tip)
        self.assertEqual(self.git('branch', '--show-current'), 'main')
        self.assertEqual(self.git('status', '--porcelain'), before)
        self.assertEqual(len(self.git('worktree', 'list', '--porcelain').split('worktree ')), 2)
        return result

    def test_allowed_branch(self):
        """The extracted commit starts at upstream and excludes fork history."""
        result = self.cut({'src/shims/dom_shim.js': 'var base = 2;\n'})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.git('rev-parse', 'up/U1-dom^'), self.base)
        self.assertEqual(self.git('diff', '--name-only', self.base, 'up/U1-dom'),
                         'src/shims/dom_shim.js')
        self.assertIn('cherry picked from commit', self.git('log', '-1', '--format=%B', 'up/U1-dom'))
        self.assertEqual(self.git('remote'), '')
        self.assertNotIn('git push', SCRIPT.read_text())

    def test_unlisted_path(self):
        """Mixed commits must be refused rather than silently losing changes."""
        result = self.cut({'src/shims/dom_shim.js': 'var base = 2;\n', 'extra': 'no\n'})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('unlisted path', result.stderr)
        self.assertEqual(self.git('branch', '--list', 'up/*'), '')

    def test_fork_header(self):
        """Assigned patches must never carry the fork copyright line."""
        result = self.cut({'src/shims/dom_shim.js':
                           '// Copyright (C) 2026 The smzuggler contributors\n'})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('copyright', result.stderr)
        self.assertEqual(self.git('branch', '--list', 'up/*'), '')

    def test_conflict_cleanup(self):
        """A replay conflict leaves neither a topic branch nor a linked checkout."""
        self.git('checkout', '-q', 'upstream-main')
        (self.repo / 'src/shims/dom_shim.js').write_text('var upstream = 3;\n')
        self.git('add', '.')
        self.git('commit', '-qm', 'Upstream change')
        self.git('checkout', '-q', 'main')
        result = self.cut({'src/shims/dom_shim.js': 'var candidate = 4;\n'})
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(self.git('branch', '--list', 'up/*'), '')
