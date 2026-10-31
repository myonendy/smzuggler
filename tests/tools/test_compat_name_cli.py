# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Keep the window.name inventory contract observable through the checker CLI."""
import pathlib
import subprocess
import sys
import tempfile
import unittest

CHECKER = pathlib.Path(__file__).resolve().parents[2] / 'tools/doclint/compat_check.py'


class CompatNameCliTests(unittest.TestCase):
    """A literal name row must participate in both directions of dump matching."""

    def test_name_matches_and_stale_name_fails(self):
        """Header filtering must neither discard name nor exempt it from equality."""
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            (root / 'docs').mkdir()
            (root / 'dom_shim.js').touch()
            (root / 'docs/compat-surface.md').write_text(
                '| name | status | shim file | test | notes |\n'
                '| --- | --- | --- | --- | --- |\n'
                '| `name` | stub | dom_shim.js | - | Empty browsing context name. |\n')
            dump = root / 'surface.tsv'
            dump.write_text('name\tstring\tdom_shim.js\n')
            command = [sys.executable, str(CHECKER), 'surface.tsv',
                       '--shim', 'dom_shim.js']
            matched = subprocess.run(command, cwd=root, capture_output=True, text=True)
            self.assertEqual(matched.returncode, 0, matched.stderr)
            self.assertEqual(matched.stdout.strip(), 'compat-check: 1 rows match dump')
            dump.write_text('')
            stale = subprocess.run(command, cwd=root, capture_output=True, text=True)
            self.assertNotEqual(stale.returncode, 0)
            self.assertIn('table member missing from dump: name (dom_shim.js)', stale.stderr)


if __name__ == '__main__':
    unittest.main()
