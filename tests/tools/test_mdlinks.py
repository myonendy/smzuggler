# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Exercise mdlinks' link, anchor and slug rules on disposable Markdown trees."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[2] / 'tools/doclint/mdlinks.py'
TARGET = '''# Guide

## Build Steps

## Build Steps

## `jsb_call()` & Friends!

<a id="custom-spot"></a>

Setext Title
------------
'''


class MdlinksTests(unittest.TestCase):
    """Check one README against a sibling guide in a fresh temporary root."""

    def run_readme(self, body):
        """Write README.md next to docs/guide.md, run mdlinks, return the result."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'docs').mkdir()
            (root / 'docs/guide.md').write_text(TARGET)
            (root / 'README.md').write_text(body)
            return subprocess.run([sys.executable, str(TOOL), '--root', directory,
                                   str(root / 'README.md')],
                                  capture_output=True, text=True)

    def assert_clean(self, body):
        """Require exit 0 and no findings for `body`."""
        result = self.run_readme(body)
        self.assertEqual((result.returncode, result.stdout), (0, ''))

    def assert_broken(self, body, line, message):
        """Require exit 1 with exactly one README.md:line finding."""
        result = self.run_readme(body)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertEqual(result.stdout.splitlines(),
                         ['README.md:%d: %s' % (line, message)])

    def test_good_file_link(self):
        """Accept existing files, directories and /-rooted paths."""
        self.assert_clean('See [guide](docs/guide.md), [dir](docs/) and '
                          '[root](/docs/guide.md "title").\n')

    def test_missing_file(self):
        """Report a missing file with its line number."""
        self.assert_broken('# Top\n\n[gone](docs/gone.md)\n', 3,
                           'missing file: docs/gone.md')

    def test_good_anchor(self):
        """Accept heading slugs in other files and in the same file."""
        self.assert_clean('# My Title\n\n[a](docs/guide.md#build-steps) '
                          '[b](#my-title) [c](docs/guide.md#jsb_call--friends) '
                          '[d](docs/guide.md#custom-spot) '
                          '[e](docs/guide.md#setext-title)\n')

    def test_missing_anchor(self):
        """Report anchors with no heading, in other files and the same file."""
        self.assert_broken('[a](docs/guide.md#nope)\n', 1,
                           'missing anchor: docs/guide.md#nope')
        self.assert_broken('# Here\n[b](#there)\n', 2, 'missing anchor: #there')

    def test_duplicate_headings(self):
        """Give the second identical heading the -1 suffix, and no -2."""
        self.assert_clean('[a](docs/guide.md#build-steps-1)\n')
        self.assert_broken('[a](docs/guide.md#build-steps-2)\n', 1,
                           'missing anchor: docs/guide.md#build-steps-2')

    def test_code_is_ignored(self):
        """Skip links in fenced blocks and inline code spans."""
        self.assert_clean('```md\n[x](missing.md)\n# Fake\n```\n'
                          '~~~\n[y](missing.md)\n~~~\n'
                          'Inline `[z](missing.md)` too.\n')
        self.assert_broken('```\n# Fake\n```\n[a](#fake)\n', 4,
                           'missing anchor: #fake')

    def test_external_links_are_ignored(self):
        """Never resolve URLs with a scheme or a //host prefix."""
        self.assert_clean('[a](https://example.invalid/x.md#nope) '
                          '[b](ftp://example.invalid/file) '
                          '[c](//example.invalid/y)\n\n'
                          '[ref]: https://example.invalid/z\n')

    def test_reference_and_html_links(self):
        """Check reference definitions and HTML href attributes as well."""
        self.assert_broken('[r]: docs/none.md\n', 1, 'missing file: docs/none.md')
        self.assert_clean('Claim.[^1]\n\n[^1]: A footnote, not a link.\n')
        self.assert_broken('<a href="docs/guide.md#zzz">x</a>\n', 1,
                           'missing anchor: docs/guide.md#zzz')


if __name__ == '__main__':
    unittest.main()
