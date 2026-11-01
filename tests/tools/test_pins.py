# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Guard offline reproducibility by rejecting moving FetchContent revisions."""

from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
FIXED_REVISION = re.compile(
    r"(?:[0-9a-fA-F]{40}|(?:v|release-|vulkan-sdk-)?[0-9]+"
    r"(?:\.[0-9]+)+(?:[-.][0-9A-Za-z]+)*|[0-9]{3,}[a-z])"
)


def git_tags(source):
    """Resolve literal set variables so indirect tags receive the same checks.

    This intentionally handles only the simple declarations used by this file;
    unresolved expressions remain visible and fail the revision assertion.
    """
    source = re.sub(r"#[^\n]*", "", source)
    variables = dict(re.findall(
        r'\bset\s*\(\s*(\w+)\s+"?([^\s"()]+)"?\s*\)',
        source, re.IGNORECASE
    ))
    declarations = re.findall(
        r'\bFetchContent_Declare\s*\((.*?)\)', source,
        re.IGNORECASE | re.DOTALL
    )
    tags = []
    for declaration in declarations:
        for tag in re.findall(r'\bGIT_TAG\s+"?([^\s"()]+)', declaration):
            variable = re.fullmatch(r"\$\{(\w+)\}", tag)
            tags.append(variables.get(variable[1], tag) if variable else tag)
    return tags


class DependencyPins(unittest.TestCase):
    """Check every declared Git revision, including optional dependencies."""

    def test_fetchcontent_revisions_are_fixed(self):
        tags = git_tags((ROOT / 'third_party/CMakeLists.txt').read_text())
        self.assertTrue(tags, 'No FetchContent Git revisions found')
        for tag in tags:
            with self.subTest(tag=tag):
                self.assertRegex(tag, '^' + FIXED_REVISION.pattern + '$')

    def test_branch_names_and_short_hashes_are_rejected(self):
        for tag in ('main', 'master', 'develop', 'feature/v1.0', 'e82fd32'):
            with self.subTest(tag=tag):
                self.assertIsNone(FIXED_REVISION.fullmatch(tag))

    def test_full_hashes_and_version_tags_are_accepted(self):
        for tag in ('abcdef0123456789' * 2 + 'abcdef01', 'release-3.4.16',
                    'v0.9.0', '16.6.0', 'vulkan-sdk-1.4.357.0', '170e'):
            with self.subTest(tag=tag):
                self.assertIsNotNone(FIXED_REVISION.fullmatch(tag))

    def test_indirect_tags_are_resolved(self):
        self.assertEqual(git_tags('set(REV "main")\n'
                                 'FetchContent_Declare(example GIT_TAG ${REV})'),
                         ['main'])
