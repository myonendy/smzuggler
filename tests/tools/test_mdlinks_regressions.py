# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Guard Markdown example and rendered-heading handling in the offline docs gate.

Uses the checker subprocess as documented in docs/style.md; temporary inputs
are owned by each test and never require network access.
"""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[2] / 'tools/doclint/mdlinks.py'


class MarkdownRegressionTests(unittest.TestCase):
    """Keep parser edge cases observable through the command-line contract."""

    def check_markdown(self, body):
        """Check caller-owned Markdown in a disposable tree; return exit and output."""
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / 'README.md'
            source.write_text(body, encoding='utf-8')
            result = subprocess.run(
                [sys.executable, str(TOOL), '--root', directory, str(source)],
                capture_output=True, text=True)
            return result.returncode, result.stdout

    def test_indented_links_are_examples(self):
        """Four spaces and tabs introduce code whose links must be ignored."""
        self.assertEqual(self.check_markdown(
            '    [example](missing.md)\n\n\t[example](missing.md)\n'), (0, ''))

    def test_indented_headings_do_not_define_anchors(self):
        """Code examples cannot satisfy fragment links, including Setext headings."""
        for example in ('    # Example\n', '\t# Example\n',
                        '    Example\n    -------\n',
                        '    Example\n-------\n',
                        '    <a id="example"></a>\n'):
            with self.subTest(example=example):
                body = '[link](#example)\n\n' + example
                self.assertEqual(self.check_markdown(body),
                                 (1, 'README.md:1: missing anchor: #example\n'))

    def test_multiline_destination_retains_opening_line(self):
        """Report missing multiline destinations on the opening link's line."""
        self.assertEqual(self.check_markdown(
            '# Title\n\n[broken](\nmissing.md\n)\n'),
            (1, 'README.md:3: missing file: missing.md\n'))
        self.assertEqual(self.check_markdown('[good](\nREADME.md\n)\n'), (0, ''))
        self.assertEqual(self.check_markdown(
            '[broken\nlabel](\n    missing.md\n)\n'),
            (1, 'README.md:1: missing file: missing.md\n'))

    def test_indentation_does_not_interrupt_paragraphs(self):
        """Indented paragraph continuations still contain actionable links."""
        self.assertEqual(self.check_markdown(
            'Paragraph\n    [broken](missing.md)\n'),
            (1, 'README.md:2: missing file: missing.md\n'))

    def test_heading_entities_render_before_slugging(self):
        """Decode named and numeric entities except inside literal code spans."""
        self.assertEqual(self.check_markdown(
            '# Fish &amp; Chips\n\n[a](#fish--chips)\n'
            '# Fish &#38; Chips\n\n[b](#fish--chips-1)\n'
            '# `Fish &amp; Chips`\n\n[c](#fish-amp-chips)\n'), (0, ''))
        self.assertEqual(self.check_markdown(
            '# Fish &amp; Chips\n\n[wrong](#fish-amp-chips)\n'),
            (1, 'README.md:3: missing anchor: #fish-amp-chips\n'))


if __name__ == '__main__':
    unittest.main()
