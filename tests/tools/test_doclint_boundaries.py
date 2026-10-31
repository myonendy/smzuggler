# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Check declaration boundaries so compact source cannot hide documentation debt.

These subprocess fixtures exercise the standalone tool without runtime resources;
see docs/doclint-baseline.md for its intentionally limited parser contract.
"""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[2] / 'tools/doclint/doclint.py'
HEADER = '''/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Supply standalone parser fixtures without runtime dependencies.
 */
'''


class DeclarationBoundaryTests(unittest.TestCase):
    """Require each definition to have its own preceding documentation."""

    def test_compact_definitions(self):
        """A comment on the first definition cannot cover its same-line neighbor."""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'compact.c'
            path.write_text(HEADER + '/** Supply the first value. */\n'
                            'int first(void) { return 1; } int second(void) { return 2; }\n')
            result = subprocess.run([sys.executable, str(TOOL), '--all', str(path)],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
            self.assertEqual(result.stdout.count('function.doc'), 1, result.stdout)


if __name__ == '__main__':
    unittest.main()
