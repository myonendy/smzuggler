# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Verify the spec section 12 header gate in isolated git repositories.

Each test owns a temporary repository whose first commit stands in for upstream;
nothing here reads the real tree or any game material. See tools/doclint/headers.py.
"""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[2] / 'tools/doclint/headers.py'
UPSTREAM = ('/*\n * Copyright (c) 2026 General Arcade (Pte. Ltd.)\n'
            ' * SPDX-License-Identifier: GPL-3.0-or-later OR '
            'LicenseRef-GeneralArcade-Commercial\n */\n')
SMZ_LINE = 'Copyright (C) 2026 The smzuggler contributors'
NEW = f'/* SPDX-License-Identifier: GPL-3.0-or-later\n * {SMZ_LINE}\n */\n'
MARK = ' * Modified 2026-10-31: fix the frame pacing.\n'


class HeadersTests(unittest.TestCase):
    """Exercise each header rule through the public command line."""

    def setUp(self):
        """Commit an upstream-style file and a smzuggler file as the base revision."""
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.git('init', '-q')
        self.git('config', 'user.name', 'Test Agent')
        self.git('config', 'user.email', 'test@example.invalid')
        self.write('src/up.c', UPSTREAM + 'int up;\n')
        self.write('src/ours.c', NEW + 'int ours;\n')
        self.git('add', '.')
        self.git('commit', '-qm', 'Initial fixture')

    def git(self, *args):
        """Run fixture git operations synchronously; failures raise with context."""
        return subprocess.run(['git', *args], cwd=self.root, check=True,
                              capture_output=True, text=True)

    def write(self, path, text):
        """Write fixture text under the owned root, creating parent directories."""
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)

    def gate(self, *extra):
        """Run the tool against the fixture's HEAD and return the finished process."""
        return subprocess.run([sys.executable, str(TOOL), '--base', 'HEAD', *extra],
                              cwd=self.root, capture_output=True, text=True)

    def assert_fails(self, path, needle, *extra):
        """Require exit 1 with a diagnostic naming PATH and containing NEEDLE."""
        result = self.gate(*extra)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn(path, result.stdout)
        self.assertIn(needle, result.stdout)

    def assert_passes(self, *extra):
        """Require a clean exit with no diagnostics."""
        result = self.gate(*extra)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout, '')

    def test_new_file_good(self):
        """Accept an added file with our SPDX and copyright lines, tracked or not."""
        self.write('src/added.c', NEW + 'int added;\n')
        self.write('tools/run.sh', '#!/bin/sh\n# SPDX-License-Identifier: '
                   f'GPL-3.0-or-later\n# {SMZ_LINE}\necho ok\n')
        self.git('add', 'tools/run.sh')
        self.assert_passes()

    def test_new_file_missing_lines(self):
        """Reject an added file without the copyright line or without SPDX."""
        self.write('src/nocopy.c', '/* SPDX-License-Identifier: GPL-3.0-or-later */\n')
        self.write('src/nospdx.py', f'# {SMZ_LINE}\n')
        result = self.gate()
        self.assertEqual(result.returncode, 1)
        self.assertIn(f'src/nocopy.c: missing "{SMZ_LINE}"', result.stdout)
        self.assertIn('src/nospdx.py: needs exactly one "SPDX-License-Identifier',
                      result.stdout)

    def test_new_file_commercial_licenseref(self):
        """Reject the GeneralArcade commercial LicenseRef on a file we add."""
        self.write('src/added.c', UPSTREAM.replace(' */\n', f' * {SMZ_LINE}\n */\n'))
        self.assert_fails('src/added.c', 'must not carry '
                          'LicenseRef-GeneralArcade-Commercial')

    def test_changed_upstream_with_mark(self):
        """Accept a changed upstream file that keeps its lines and adds ours."""
        self.write('src/up.c', UPSTREAM.replace(' */\n', f' * {SMZ_LINE}\n{MARK} */\n')
                   + 'int up = 1;\n')
        self.assert_passes()

    def test_changed_upstream_without_mark(self):
        """Reject a changed upstream file lacking our line or a dated note."""
        self.write('src/up.c', UPSTREAM + 'int up = 1;\n')
        result = self.gate()
        self.assertEqual(result.returncode, 1)
        self.assertIn(f'src/up.c: missing "{SMZ_LINE}"', result.stdout)
        self.assertIn('src/up.c: needs a new "Modified', result.stdout)

    def test_moved_upstream_file_keeps_upstream_header(self):
        """Accept a moved upstream file that keeps its lines and adds ours and a note."""
        (self.root / 'src/up.c').unlink()
        self.write('src/moved.c', UPSTREAM.replace(' */\n', f' * {SMZ_LINE}\n{MARK} */\n')
                   + 'int up;\n')
        self.git('add', '-A')
        self.assert_passes()

    def test_moved_upstream_file_without_mark_fails(self):
        """Reject a moved upstream file that lacks our line and a dated note."""
        self.git('mv', 'src/up.c', 'src/moved.c')
        self.assert_fails('src/moved.c', 'needs a new "Modified')

    def test_undated_mark_is_not_a_mark(self):
        """Reject a Modified note whose date is missing or impossible."""
        bad = ' * Modified 2026-13-45: fix the frame pacing.\n * Modified: no date\n'
        self.write('src/up.c', UPSTREAM.replace(' */\n', f' * {SMZ_LINE}\n{bad} */\n'))
        self.assert_fails('src/up.c', 'needs a new "Modified')

    def test_second_change_needs_new_mark(self):
        """Require a fresh note when an already-marked upstream file changes again."""
        marked = UPSTREAM.replace(' */\n', f' * {SMZ_LINE}\n{MARK} */\n')
        self.write('src/up.c', marked)
        self.git('commit', '-qam', 'First smzuggler change')
        self.write('src/up.c', marked + 'int again;\n')
        self.assert_fails('src/up.c', 'needs a new "Modified')

    def test_removed_upstream_copyright_fails(self):
        """Reject a changed upstream file whose original copyright line is gone."""
        stripped = UPSTREAM.replace(' * Copyright (c) 2026 General Arcade (Pte. Ltd.)\n', '')
        self.write('src/up.c', stripped.replace(' */\n', f' * {SMZ_LINE}\n{MARK} */\n'))
        self.assert_fails('src/up.c', 'upstream line removed or altered: '
                          '"Copyright (c) 2026 General Arcade (Pte. Ltd.)"')

    def test_altered_upstream_spdx_fails(self):
        """Reject replacing upstream's SPDX expression with ours."""
        swapped = UPSTREAM.replace(' OR LicenseRef-GeneralArcade-Commercial', '')
        self.write('src/up.c', swapped.replace(' */\n', f' * {SMZ_LINE}\n{MARK} */\n'))
        self.assert_fails('src/up.c', 'upstream line removed or altered: '
                          '"SPDX-License-Identifier: GPL-3.0-or-later OR')

    def test_changed_smzuggler_file_needs_no_mark(self):
        """Treat a file whose base header is smzuggler-only like a new file."""
        self.write('src/ours.c', NEW + 'int ours = 2;\n')
        self.assert_passes()

    def test_skipped_types_and_deletions(self):
        """Ignore comment-less files and deleted upstream files."""
        self.write('data/map.json', '{"no": "header"}\n')
        (self.root / 'img.png').write_bytes(b'\x89PNG\r\n\x1a\n\x00\xff')
        (self.root / 'src/up.c').unlink()
        self.assert_passes()

    def test_verbatim_licence_texts_skipped_but_licence_prose_checked(self):
        """Skip LICENSE and LICENSES/*.txt only; README prose and lookalikes fail."""
        self.write('LICENSE', 'GNU GENERAL PUBLIC LICENSE\n')
        self.write('LICENSES/GPL-3.0.txt', 'GNU GENERAL PUBLIC LICENSE\n')
        self.assert_passes()
        for path in ('LICENSES/README.md', 'docs/LICENSE', 'LICENSES/sub/x.txt'):
            self.write(path, 'project-written notes\n')
            self.assert_fails(path, 'SPDX-License-Identifier')

    def test_prose_and_configuration_require_headers(self):
        """Prevent prose and configuration from bypassing the CLI header rules."""
        for path in ('docs/notes.md', 'config.toml', 'config.yaml', '.gitignore'):
            self.write(path, 'missing licensing lines\n')
            self.assert_fails(path, 'SPDX-License-Identifier')

    def test_prose_and_configuration_headers_pass(self):
        """Accept native HTML and hash comments without including their delimiters."""
        self.write('docs/notes.md', '<!-- SPDX-License-Identifier: '
                   f'GPL-3.0-or-later -->\n<!-- {SMZ_LINE} -->\n# Notes\n')
        for path in ('config.toml', 'config.yaml', '.gitignore'):
            self.write(path, f'# SPDX-License-Identifier: GPL-3.0-or-later\n'
                       f'# {SMZ_LINE}\n')
        self.assert_passes()

    def test_undecodable_source_fails(self):
        """Reject invalid UTF-8 even when a source header otherwise looks valid."""
        for path in ('src/added.c', 'src/up.c', 'tools/run.py'):
            target = self.root / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(NEW.encode() + b'\xff')
            self.assert_fails(path, 'invalid UTF-8')

    def test_upstream_mode_rejects_smzuggler_line(self):
        """In --upstream mode, our copyright line or note must not appear."""
        self.write('src/up.c', UPSTREAM.replace(' */\n', f' * {SMZ_LINE}\n */\n'))
        self.write('src/offer.c', UPSTREAM.replace(' */\n', f' * {SMZ_LINE}\n */\n'))
        result = self.gate('--upstream')
        self.assertEqual(result.returncode, 1)
        for path in ('src/up.c', 'src/offer.c'):
            self.assertIn(f'{path}: upstream offer must not carry "{SMZ_LINE}"',
                          result.stdout)

    def test_upstream_mode_accepts_untouched_header(self):
        """In --upstream mode, accept upstream's header on changed and added files."""
        self.write('src/up.c', UPSTREAM + 'int up = 3;\n')
        self.write('src/offer.c', UPSTREAM + 'int offer;\n')
        self.assert_passes('--upstream')

    def test_upstream_mode_rejects_unrelated_copyright(self):
        """An added offer must name upstream's canonical copyright holder."""
        unrelated = UPSTREAM.replace('General Arcade (Pte. Ltd.)', 'Unrelated Project')
        self.write('src/offer.c', unrelated + 'int offer;\n')
        self.assert_fails('src/offer.c', 'canonical upstream copyright/SPDX', '--upstream')

    def test_upstream_mode_rejects_substituted_spdx(self):
        """An added offer must retain upstream's complete licence expression."""
        substituted = UPSTREAM.replace('GPL-3.0-or-later OR '
                                       'LicenseRef-GeneralArcade-Commercial', 'MIT')
        self.write('src/offer.c', substituted + 'int offer;\n')
        self.assert_fails('src/offer.c', 'canonical upstream copyright/SPDX', '--upstream')

    def test_upstream_mode_rejects_header_edits(self):
        """In --upstream mode, reject a changed upstream header or a note."""
        self.write('src/up.c', UPSTREAM.replace('(Pte. Ltd.)', 'Ltd') + MARK)
        result = self.gate('--upstream')
        self.assertEqual(result.returncode, 1)
        self.assertIn('src/up.c: upstream copyright/SPDX lines must stay untouched',
                      result.stdout)
        self.assertIn('src/up.c: upstream offer must not add', result.stdout)

    def test_bad_base_is_usage_error(self):
        """Fail with status 2 and context when the base revision does not exist."""
        result = subprocess.run([sys.executable, str(TOOL), '--base', 'no-such-rev'],
                                cwd=self.root, capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("no-such-rev", result.stderr)


if __name__ == '__main__':
    unittest.main()
