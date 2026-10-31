# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Guard contribution contracts in the contributor documentation layer.

See CONTRIBUTING.md. Read-only tests run on the test thread; git failures and
missing documents fail loudly. Never load game material or private identities.
"""

from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
CHECKLIST = (
    'Why the change is needed',
    'Docs updated',
    'Compat row updated',
    'Tests added',
    'Upstream-eligible?',
    'Privacy scan clean (§16)',
)


class ContributionContracts(unittest.TestCase):
    """Prevent omissions from the spec checklist and upstream preservation."""

    def test_checklist(self):
        """Require each §15.7 item as an actionable unchecked template entry."""
        template = (ROOT / '.github/pull_request_template.md').read_text()
        for item in CHECKLIST:
            with self.subTest(item=item):
                self.assertIn('- [ ] ' + item, template)
        self.assertIn('no-doc-change', template)
        self.assertIn('Justification:', template)
        self.assertIn('descriptive commit subjects', template)

    def test_upstream_bytes(self):
        """Keep U-PR instructions byte-exact after the fork note comment block."""
        upstream = subprocess.check_output(
            ['git', 'show', 'f789f46:CONTRIBUTING.md'], cwd=ROOT)
        preserved = (ROOT / 'docs/upstream-CONTRIBUTING.md').read_bytes()
        self.assertTrue(preserved.startswith(b'<!--\n'))
        note, body = preserved.split(b'\n-->\n', 1)
        self.assertIn(b'\nSPDX-License-Identifier: GPL-3.0-or-later\n', note)
        self.assertIn(b'\nCopyright (C) 2026 The smzuggler contributors\n', note)
        self.assertEqual(body, upstream)

    def test_commit_rules_preserved(self):
        """Compare the commit section to its pre-card revision without rewriting it."""
        previous = subprocess.check_output(
            ['git', 'show', '256e7f55fc11ed28bcc1118cc1e90ac51a13151e:CONTRIBUTING.md'], cwd=ROOT).decode()
        heading = '## Commit messages (smzuggler)'
        current = (ROOT / 'CONTRIBUTING.md').read_text()
        self.assertEqual(current[current.index(heading):],
                         previous[previous.index(heading):])

    def test_links(self):
        """Exercise the real offline link checker for all contribution documents."""
        result = subprocess.run(
            ['python3', 'tools/doclint/mdlinks.py', 'CONTRIBUTING.md',
             'docs/upstream-CONTRIBUTING.md', '.github/pull_request_template.md'],
            cwd=ROOT, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
