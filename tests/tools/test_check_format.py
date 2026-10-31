# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Regression coverage for changed-line formatting; see docs/style.md.
# Temporary repositories own all fixtures; no runtime resources are used.
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


class FormatTests(unittest.TestCase):
    """Exercise Git and clang-format together without changing the checkout."""

    def test_changed_lines(self):
        """Only changed lines may produce diagnostics, even beside old bad style."""
        with tempfile.TemporaryDirectory() as directory:
            repo = pathlib.Path(directory)
            subprocess.run(['git', 'init', '-q', directory], check=True)
            (repo / '.clang-format').write_bytes((ROOT / '.clang-format').read_bytes())
            source = repo / 'sample.c'
            source.write_text('int first = 1;\n\nint second = 2;\n')
            subprocess.run(['git', '-C', directory, 'add', '.'], check=True)
            subprocess.run(['git', '-C', directory, '-c', 'user.name=Fixture',
                            '-c', 'user.email=fixture@example.invalid', 'commit',
                            '-qm', 'Initial fixture'], check=True)
            command = ['python3', str(ROOT / 'tools/format/check_format.py'),
                       '--base', 'HEAD']
            result = subprocess.run(command, cwd=repo, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            source.write_text('int first = 1;\n\nint second=2;\n')
            subprocess.run(['git', '-C', directory, 'add', '.'], check=True)
            subprocess.run(['git', '-C', directory, '-c', 'user.name=Fixture',
                            '-c', 'user.email=fixture@example.invalid', 'commit',
                            '-qm', 'Malformed fixture'], check=True)
            result = subprocess.run(command[:-1] + ['HEAD~1'], cwd=repo,
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertIn('sample.c:3:', result.stdout)
            self.assertNotIn('sample.c:1:', result.stdout)

    def test_untouched_bad_line(self):
        """A formatter expansion must not accuse legacy lines outside the diff."""
        with tempfile.TemporaryDirectory() as directory:
            repo = pathlib.Path(directory)
            subprocess.run(['git', 'init', '-q', directory], check=True)
            (repo / '.clang-format').write_bytes((ROOT / '.clang-format').read_bytes())
            source = repo / 'sample.c'
            source.write_text('int old=1;\n\nint value = 2;\n')
            subprocess.run(['git', '-C', directory, 'add', '.'], check=True)
            subprocess.run(['git', '-C', directory, '-c', 'user.name=Fixture',
                            '-c', 'user.email=fixture@example.invalid', 'commit',
                            '-qm', 'Initial fixture'], check=True)
            source.write_text('int old=1;\n\nint value = 3;\n')
            result = subprocess.run(['python3', str(ROOT / 'tools/format/check_format.py'),
                                     '--base', 'HEAD'], cwd=repo, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            source.write_text('int old=1;\n\nint value = 2;\n')
            subprocess.run(['git', '-C', directory, 'mv', 'sample.c', 'renamed.c'], check=True)
            result = subprocess.run(['python3', str(ROOT / 'tools/format/check_format.py'),
                                     '--base', 'HEAD'], cwd=repo, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
