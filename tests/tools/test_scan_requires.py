# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Run tools/compat/scan_requires.py over synthetic plugins, never a game tree.

Fixtures live in a temporary directory owned by each test. Output is plugin
basenames and counts only; see docs/shims.md.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[2] / 'tools/compat/scan_requires.py'


class ScanRequiresTests(unittest.TestCase):
    """Pin env, path and count contracts at the CLI boundary."""

    def run_scan(self, extra_env=None):
        """Run the scanner; return the finished process. Env is copied."""
        merged = dict(os.environ)
        merged.pop('SMZUGGLER_GAME_DIR', None)
        if extra_env:
            merged.update(extra_env)
        return subprocess.run([sys.executable, str(TOOL)], env=merged,
                              capture_output=True, text=True, timeout=15)

    def write_plugins(self, root, files):
        """Create js/plugins/*.js under ROOT from a name-to-source map."""
        plugins = root / 'js' / 'plugins'
        plugins.mkdir(parents=True)
        for name, source in files.items():
            (plugins / name).write_text(source, encoding='utf-8')
        return plugins

    def test_counts_require_and_process(self):
        """Each plugin line is a basename plus require and process counts."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.write_plugins(root, {
                'alpha.js': "var fs = require('fs');\nprocess.cwd();\n"
                            "process.env.HOME;\nrequire('path');\n",
                'beta.js': "require('nw.gui');\n",
                'gamma.js': "var p = process.platform;\n",
                'none.js': "var x = 1;\n",
            })
            result = self.run_scan({'SMZUGGLER_GAME_DIR': str(root)})
            self.assertEqual(result.returncode, 0, result.stderr)
            lines = [line for line in result.stdout.splitlines() if line]
            self.assertEqual(lines, [
                'alpha.js\trequire=2\tprocess=2',
                'beta.js\trequire=1\tprocess=0',
                'gamma.js\trequire=0\tprocess=1',
                'none.js\trequire=0\tprocess=0',
            ])
            self.assertNotIn(str(root), result.stdout + result.stderr)

    def test_ignores_comments(self):
        """Comment text must not count as a require or process use."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.write_plugins(root, {
                'comment.js': "// require('nope')\n/* process.exit() */\n"
                              "var ok = 1;\n",
            })
            result = self.run_scan({'SMZUGGLER_GAME_DIR': str(root)})
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout.strip(),
                             'comment.js\trequire=0\tprocess=0')

    def test_missing_env(self):
        """Unset SMZUGGLER_GAME_DIR is a setup error with a path-free message."""
        result = self.run_scan()
        self.assertEqual(result.returncode, 2)
        self.assertIn('SMZUGGLER_GAME_DIR', result.stderr)
        self.assertNotRegex(result.stderr, r'/home/|/Users/|C:\\\\Users')

    def test_missing_plugins_dir(self):
        """A game copy without js/plugins fails closed, without printing paths."""
        with tempfile.TemporaryDirectory() as directory:
            result = self.run_scan({'SMZUGGLER_GAME_DIR': directory})
            self.assertEqual(result.returncode, 2, result.stderr)
            self.assertIn('js/plugins', result.stderr)
            self.assertNotIn(directory, result.stdout + result.stderr)

    def test_refuses_steam_path(self):
        """Steam/Proton path components are refused without echoing the path."""
        with tempfile.TemporaryDirectory() as directory:
            game = Path(directory) / 'steamapps' / 'common' / 'game'
            game.mkdir(parents=True)
            (game / 'js' / 'plugins').mkdir(parents=True)
            result = self.run_scan({'SMZUGGLER_GAME_DIR': str(game)})
            self.assertEqual(result.returncode, 2, result.stderr)
            self.assertIn('Steam', result.stderr)
            self.assertNotIn(str(game), result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
