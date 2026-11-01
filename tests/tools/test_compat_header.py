# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Protect literal DOM names from compatibility-table header filtering."""
import importlib.util
from pathlib import Path
import tempfile
import unittest


class CompatHeaderTests(unittest.TestCase):
    """Header recognition must preserve an API named name."""

    def test_name_member_survives_header(self):
        """Only the complete header is metadata, including with code spans."""
        source = Path(__file__).resolve().parents[2] / 'tools/doclint/compat_check.py'
        spec = importlib.util.spec_from_file_location('compat_check', source)
        checker = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(checker)
        with tempfile.TemporaryDirectory() as directory:
            table = Path(directory) / 'surface.md'
            table.write_text(
                '| `name` | `status` | `shim file` | `test` | `notes` |\n'
                '| --- | --- | --- | --- | --- |\n'
                '| `name` | stub | dom_shim.js | - | Empty context name. |\n')
            self.assertEqual(list(checker.table_rows(table)), [
                ['name', 'stub', 'dom_shim.js', '-', 'Empty context name.']])


if __name__ == '__main__':
    unittest.main()
