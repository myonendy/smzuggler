# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Exercise ES5 expression contexts that resemble modern syntax.
# See docs/style.md; fixtures are borrowed strings with no runtime resources.
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'tools/format'))
from es5_check import check


class RegexContextTests(unittest.TestCase):
    """Keep ES5 regex expressions distinct from forbidden syntax tokens."""

    def test_expression_regexes(self):
        """Logical operators and unary keywords may precede ES5 regex literals."""
        for source in ('var x = value && /class|=>/.test(text);',
                       'var x = value || /let|const/.test(text);',
                       'function f() { throw /class|=>/; }',
                       'var x = typeof /class|=>/;',
                       'var x = void /let|const/;'):
            with self.subTest(source=source):
                self.assertEqual(list(check(source)), [])

    def test_modern_rhs(self):
        """Logical operators must still expose modern syntax on their right."""
        self.assertTrue(list(check('var x = value && (x => x);')))
