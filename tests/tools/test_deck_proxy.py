# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Check the optional Deck-proxy script and recipe stay clearly labelled.

Why: spec §14.9 makes the proxy optional and never a pass, and every cap it
changes must be restorable. These tests read the shipped text and run the
script's non-root paths only, so they never modify host frequency settings.
No game or personal data is involved.
"""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'tools/perf/deck-proxy.sh'
RECIPE = ROOT / 'tools/perf/deck-proxy.md'


class DeckProxyTests(unittest.TestCase):
    """Verify syntax, help/restore behaviour and the proxy labels."""

    def run_script(self, *args, state=None):
        """Run the script with an isolated state directory and no root writes."""
        env = dict(os.environ)
        if state is not None:
            env['DECK_PROXY_STATE_DIR'] = str(state)
        return subprocess.run(
            ['sh', str(SCRIPT), *args], env=env, capture_output=True, text=True)

    def test_script_exists_and_parses(self):
        """The script is executable and valid POSIX shell."""
        self.assertTrue(SCRIPT.is_file())
        self.assertTrue(os.access(SCRIPT, os.X_OK))
        result = subprocess.run(['sh', '-n', str(SCRIPT)], capture_output=True,
                                text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_help_names_the_proxy_and_restore(self):
        """Help explains the caps, the undo path and that it is not a pass."""
        result = self.run_script('--help')
        self.assertEqual(result.returncode, 0, result.stderr)
        for expected in ('--restore', 'taskset', 'profile: deck-proxy',
                         'gate: proxy', 'not a pass'):
            self.assertIn(expected, result.stdout.lower())

    def test_restore_is_safe_without_state(self):
        """A restore with nothing recorded succeeds and writes no output file."""
        with tempfile.TemporaryDirectory() as raw:
            result = self.run_script('--restore', state=Path(raw) / 'missing')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('nothing to restore', result.stdout)

    def test_recipe_labels_and_caps(self):
        """The recipe states the optional status, caps and calibration rule."""
        text = RECIPE.read_text(encoding='utf-8')
        for expected in ('optional', 'not a pass', 'profile: deck-proxy',
                         'gate: proxy', 'taskset -c 0-7', '2.8 GHz',
                         '1250 MHz', '1280x800', '--restore', 'Calibration'):
            self.assertIn(expected, text)
        self.assertIn('steam-deck', text)


if __name__ == '__main__':
    unittest.main()
