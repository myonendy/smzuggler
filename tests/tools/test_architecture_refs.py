# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Keep the architecture navigation trustworthy without running game code.

See ARCHITECTURE.md. Reads repository files on the test thread; fixtures stay
in temporary directories. References use backticked repository paths, optionally
followed by a colon and a function name. Runtime paths belong in plain prose.
"""

from pathlib import Path
import re
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
REFERENCE = re.compile(
    r"`((?:src/|tools/|tests/|docs/)[^`\s:]*|[A-Z][A-Z_.]+\.md)"
    r"(?::([A-Za-z_]\w*))?`"
)


def reference_errors(document, root):
    """Return broken local references; borrow text and root, retain no resources.

    This read-only check runs on the caller's thread; filesystem errors propagate
    so inaccessible sources cannot silently count as valid navigation.
    """
    errors = []
    for match in REFERENCE.finditer(document):
        path, function = match.groups()
        target = root / path
        if not target.exists():
            errors.append(f"missing path: {path}")
        elif function:
            source = target.read_text(encoding='utf-8')
            definition = rf"\b{re.escape(function)}\s*\([^;{{}}]*\)\s*\{{"
            if not re.search(definition, source):
                errors.append(f"missing function definition: {path}:{function}")
    return errors


class ArchitectureReferences(unittest.TestCase):
    """Validate navigation and directory coverage as the source tree grows."""

    def test_references(self):
        """Fail with actionable paths when the architecture names stale code."""
        document = (ROOT / 'ARCHITECTURE.md').read_text(encoding='utf-8')
        self.assertTrue(list(REFERENCE.finditer(document)))
        self.assertEqual(reference_errors(document, ROOT), [])

    def test_directory_map(self):
        """Require every source/tool directory, excluding generated caches."""
        document = (ROOT / 'ARCHITECTURE.md').read_text(encoding='utf-8')
        for base in ('src', 'tools'):
            directories = [ROOT / base] + [
                path for path in (ROOT / base).rglob('*')
                if path.is_dir() and '__pycache__' not in path.parts
            ]
            for directory in directories:
                relative = directory.relative_to(ROOT).as_posix() + '/'
                with self.subTest(directory=relative):
                    self.assertRegex(document, rf'(?m)^\| `{re.escape(relative)}` \|',
                                     msg=f'missing directory row: {relative}')

    def test_checker_rejects_stale_references(self):
        """Exercise missing files/functions and reject calls as definitions."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'src').mkdir()
            (root / 'src/example.c').write_text(
                'void present(void) { absent(); }\n', encoding='utf-8')
            self.assertEqual(reference_errors('`src/example.c:present`', root), [])
            self.assertEqual(len(reference_errors('`src/missing.c`', root)), 1)
            self.assertEqual(len(reference_errors('`src/example.c:absent`', root)), 1)


if __name__ == '__main__':
    unittest.main()
