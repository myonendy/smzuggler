# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Exercise the offline compatibility inventory gate; see ARCHITECTURE.md.

Temporary repositories keep these CLI tests independent of the runtime build.
"""
import pathlib
import subprocess
import sys
import tempfile
import unittest

CHECKER = pathlib.Path(__file__).resolve().parents[2] / 'tools/doclint/compat_check.py'


class CompatCheckTests(unittest.TestCase):
    """Pin bidirectional equality and file contracts at the CLI boundary."""

    def run_check(self, rows=None, dump=None, args=(), shim=True, test=True):
        """Run with owned temporary files; return captured status and diagnostics."""
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            (root / 'docs').mkdir()
            if shim:
                (root / 'a.js').touch()
            if test:
                (root / 'test.py').touch()
            (root / 'docs/compat-surface.md').write_text(
                '| name | status | shim file | test | notes |\n'
                '| --- | --- | --- | --- | --- |\n' +
                (rows if rows is not None else '| A | real | a.js | test.py | native |\n'))
            (root / 'surface.tsv').write_text(
                dump if dump is not None else 'A\tfunction\ta.js\n')
            return subprocess.run([sys.executable, str(CHECKER), 'surface.tsv', *args],
                                  cwd=root, text=True, capture_output=True)

    def test_equal(self):
        """Equal inventories with valid evidence must pass."""
        self.assertEqual(self.run_check().returncode, 0)

    def test_extra_dump(self):
        """An undocumented runtime member must fail."""
        self.assertIn('B', self.run_check(dump='A\tfunction\ta.js\nB\tobject\ta.js\n').stderr)
        self.assertNotEqual(self.run_check(dump='B\tobject\ta.js\n').returncode, 0)

    def test_extra_table(self):
        """A stale documented member must fail."""
        self.assertNotEqual(self.run_check(dump='').returncode, 0)

    def test_missing_shim(self):
        """Rows must credit a real implementation file."""
        self.assertNotEqual(self.run_check(shim=False).returncode, 0)

    def test_missing_test(self):
        """Real and partial claims require existing test evidence."""
        for status in ('real', 'partial'):
            self.assertNotEqual(self.run_check(
                rows=f'| A | {status} | a.js | test.py | native |\n', test=False).returncode, 0)

    def test_filter(self):
        """Filtering excludes both other-shim rows and other-shim dump members."""
        rows = '| A | real | a.js | test.py | native |\n| B | stub | b.js | - | gap |\n'
        dump = 'A\tfunction\ta.js\nC\tobject\tb.js\n'
        self.assertEqual(self.run_check(rows, dump, ('--shim', 'a.js')).returncode, 0)
        self.assertNotEqual(self.run_check(rows, dump.replace('A\t', 'D\t'),
                                            ('--shim', 'a.js')).returncode, 0)

    def test_stub_and_missing_still_participate(self):
        """Status labels must never exempt a stale row from exact equality."""
        for status in ('stub', 'missing'):
            rows = f'| A | {status} | a.js | - | known gap |\n'
            self.assertEqual(self.run_check(rows=rows, test=False).returncode, 0)
            self.assertNotEqual(self.run_check(rows=rows, dump='').returncode, 0)

    def test_literal_backtick_name(self):
        """Legacy RegExp members contain a backtick that must not be stripped."""
        result = self.run_check(rows='| RegExp.$` | stub | a.js | - | legacy |\n',
                                dump='RegExp.$`\taccessor\ta.js\n')
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_attribution_mismatch(self):
        """An API credited to the wrong shim is a mismatch, even if names match."""
        result = self.run_check(dump='A\tfunction\tb.js\n')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('missing from dump', result.stderr)
        self.assertIn('missing from table', result.stderr)


if __name__ == '__main__':
    unittest.main()
