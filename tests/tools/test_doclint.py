# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Exercise doclint's source and diff contracts using disposable repositories."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[2] / 'tools/doclint/doclint.py'
HEADER = '''/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Provide a small fixture for documentation checks.
 */
'''


class DoclintTests(unittest.TestCase):
    """Keep parsing coverage independent of the repository's legacy findings."""

    def run_sample(self, source, suffix='.c'):
        """Lint one temporary source and return diagnostics without retaining files."""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / ('sample' + suffix)
            path.write_text(source)
            return subprocess.run([sys.executable, str(TOOL), '--all', directory],
                                  capture_output=True, text=True)

    def test_headers(self):
        """Distinguish absent headers, license metadata, and purpose text."""
        for source, rule in [('int x;\n', 'header.missing'),
                             (HEADER.replace('SPDX-License-Identifier:', 'License:'),
                              'header.spdx'),
                             ('/* SPDX-License-Identifier: GPL-3.0-or-later\n'
                              ' * Copyright (C) 2026 Contributors\n */\n',
                              'header.purpose'),
                             (HEADER.replace('Copyright', 'Authors'),
                              'header.copyright')]:
            with self.subTest(rule=rule):
                result = self.run_sample(source)
                self.assertEqual(result.returncode, 1)
                self.assertIn(':1: ' + rule, result.stdout)

    def test_function_forms(self):
        """Require comments on static, multiline, C, and JavaScript definitions."""
        samples = [('int work(void) { return 1; }', '.c'),
                   ('static void work(void) {}', '.c'),
                   ('const char *work(void) { return 0; }', '.c'),
                   ('int\nwork(\n int value\n)\n{ return value; }', '.cpp'),
                   ('int work(void);', '.h'),
                   ('function work(value) { return value; }', '.js'),
                   ('Thing.prototype.work = function(value) {};', '.js')]
        for source, suffix in samples:
            with self.subTest(source=source):
                result = self.run_sample(HEADER + source, suffix)
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertIn('function.doc', result.stdout)
                result = self.run_sample(HEADER + '/** Explain the contract. */\n' + source,
                                         suffix)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_source_noise(self):
        """Ignore fake definitions in comments, strings, control flow and calls."""
        source = HEADER + '/** Keep a documented entry point. */\n' + r'''
void work(void) {
    const char *text = "int fake(void) {}";
    /* int fake_again(void) {} */
    if (ready()) { consume(); }
    return consume();
}
'''
        result = self.run_sample(source)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_js_methods(self):
        """Check class shorthand methods and arrows as well as ES5 assignments."""
        source = HEADER + 'class Thing {\n work(value) { return value; }\n}\n'
        result = self.run_sample(source, '.js')
        self.assertEqual(result.returncode, 1)
        self.assertIn('function.doc', result.stdout)
        source = source.replace(' work(value)', ' /** Preserve the value. */\n work(value)')
        result = self.run_sample(source, '.js')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        result = self.run_sample(HEADER + 'const work = (value) => { return value; };', '.js')
        self.assertEqual(result.returncode, 1)
        self.assertIn('function.doc', result.stdout)

    def test_changed_lines(self):
        """Only touched signatures or bodies require comments in changed mode."""
        with tempfile.TemporaryDirectory() as directory:
            def git(*args):
                """Run local Git fixture commands and fail with captured context."""
                return subprocess.run(['git', '-C', directory, *args], check=True,
                                      capture_output=True, text=True)
            git('init', '-q')
            path = Path(directory) / 'sample.c'
            path.write_text(HEADER + 'int old(void) { return 1; }\n'
                            '/** Existing contract. */\nint fresh(void) { return 2; }\n')
            git('add', '.')
            git('-c', 'user.name=Fixture', '-c', 'user.email=fixture@example.invalid',
                'commit', '-qm', 'Initial fixture')
            path.write_text(path.read_text().replace('return 2', 'return 3'))
            command = [sys.executable, str(TOOL), '--changed', 'HEAD']
            result = subprocess.run(command, cwd=directory, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            path.write_text(path.read_text().replace('return 1', 'return 4'))
            result = subprocess.run(command, cwd=directory, capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertEqual(result.stdout.count('function.doc'), 1)
            path.write_text(HEADER + 'int old(void) { return 1; }\n')
            result = subprocess.run(command, cwd=directory, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
