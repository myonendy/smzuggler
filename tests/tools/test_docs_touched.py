# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Verify docs gate contracts in isolated repositories, without runtime resources.

See docs/docs-map.toml; fixtures own their files and never access game material.
"""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[2] / 'tools/doclint/docs_touched.py'


class DocsTouchedTests(unittest.TestCase):
    """Exercise git changes and CI exceptions through the public command."""

    def setUp(self):
        """Create an owned temporary git history for each gate invocation."""
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.git('init', '-q')
        self.git('config', 'user.name', 'Test Agent')
        self.git('config', 'user.email', 'test@example.invalid')
        self.write('docs/docs-map.toml', '[[areas]]\npath = "src/shims/**"\n'
                   'docs = ["docs/shims.md", "docs/compat-surface.md"]\n')
        self.write('src/shims/sample.js', 'old\n')
        self.git('add', '.')
        self.git('commit', '-qm', 'Initial fixture')

    def git(self, *args):
        """Run fixture git operations synchronously; failures raise with context."""
        return subprocess.run(['git', *args], cwd=self.root, check=True,
                              capture_output=True, text=True)

    def write(self, path, text):
        """Write fixture text under the owned root, creating parent directories."""
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)

    def gate(self, **env):
        """Return CLI diagnostics with explicit CI environment, avoiding host labels."""
        clean = dict(os.environ)
        clean.pop('NO_DOC_CHANGE', None)
        clean.pop('PR_BODY', None)
        clean.update(env)
        return subprocess.run([sys.executable, str(TOOL), '--base', 'HEAD'],
                              cwd=self.root, env=clean, capture_output=True, text=True)

    def test_mapped_change(self):
        """Require any listed document and name missing, not-yet-created docs."""
        self.write('src/shims/sample.js', 'new\n')
        result = self.gate()
        self.assertEqual(result.returncode, 1)
        for expected in ('src/shims/sample.js', 'docs/shims.md',
                         'docs/compat-surface.md', 'to be created'):
            self.assertIn(expected, result.stdout)
        self.write('docs/shims.md', 'documentation\n')
        self.assertEqual(self.gate().returncode, 0)

    def test_unmapped_file(self):
        """Unrelated root files impose no subsystem documentation obligation."""
        self.write('notes.txt', 'notes\n')
        self.assertEqual(self.gate().returncode, 0)

    def test_label_justification(self):
        """CI label requires a reason when a PR body is supplied."""
        self.write('src/shims/sample.js', 'new\n')
        for body in ('', 'No-doc-change justification:   \n',
                     'No-doc-change justification:\nUnrelated prose\n',
                     'No-doc-change justification: <!-- explain -->\n'):
            self.assertEqual(self.gate(NO_DOC_CHANGE='1', PR_BODY=body).returncode, 1)
        self.assertEqual(self.gate(NO_DOC_CHANGE='1', PR_BODY=
                         'No-doc-change justification: Internal cleanup.\n').returncode, 0)
        self.assertEqual(self.gate(NO_DOC_CHANGE='1').returncode, 0)

    def test_new_directory(self):
        """New source modules require architecture and a map entry."""
        self.write('src/new_module/sample.c', 'source\n')
        result = self.gate()
        self.assertEqual(result.returncode, 1)
        self.assertIn('unmapped directory: src/new_module', result.stdout)
        self.assertIn('ARCHITECTURE.md', result.stdout)
        self.write('ARCHITECTURE.md', 'module description\n')
        self.assertIn('unmapped directory', self.gate().stdout)

    def test_unmapped_tools_directory(self):
        """Tool directories need explicit coverage even when CI exempts prose."""
        self.write('tools/new_tool/check.py', 'pass\n')
        result = self.gate(NO_DOC_CHANGE='1')
        self.assertEqual(result.returncode, 1)
        self.assertIn('unmapped directory: tools/new_tool', result.stdout)

    def test_invalid_base(self):
        """An invalid git revision fails loudly rather than yielding a clean gate."""
        result = subprocess.run([sys.executable, str(TOOL), '--base', 'absent-ref'],
                                cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn('cannot check', result.stderr)

    def test_committed_and_staged_changes(self):
        """Compare base through the working tree, including committed changes."""
        self.write('src/shims/sample.js', 'new\n')
        self.git('add', '.')
        self.git('commit', '-qm', 'Change source')
        result = subprocess.run([sys.executable, str(TOOL), '--base', 'HEAD~1'],
                                cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn('src/shims/sample.js', result.stdout)


if __name__ == '__main__':
    unittest.main()
