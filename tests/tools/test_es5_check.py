# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# ES5 guard regressions; see docs/style.md. Fixtures stay in memory.
import importlib.util
import pathlib
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/format'))
SPEC = importlib.util.spec_from_file_location('es5_check', ROOT / 'tools/format/es5_check.py')
CHECK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECK)


class ES5Tests(unittest.TestCase):
    """Prevent syntax upgrades in shared shims without requiring Node."""

    def test_forbidden(self):
        """Each forbidden construct must produce a diagnostic independently."""
        samples = ['let x = 1;', 'const x = 1;', 'var f = x => x;',
                   'class Thing {}', 'var x = `text`;', 'var {x} = y;',
                   'var [x] = y;', 'function f(x = 1) {}',
                   'function f(...x) {}', 'f(...x);', 'for (x of xs) {}',
                   '({x} = y);', '([x] = y);']
        for sample in samples:
            with self.subTest(sample=sample):
                self.assertTrue(list(CHECK.check(sample)))

    def test_lookalikes(self):
        """Strings, comments, regexes and property names are ordinary ES5."""
        source = '''var o = {class: '=>', let: 1, const: 2};
        o.class = "`"; // => let const class
        /* function f(x = 1) {} */ var re = /class|let|=>/;
        function f(x) { return x / 2; }'''
        self.assertEqual(list(CHECK.check(source)), [])

    def test_upstream_shims(self):
        """Check the actual upstream snapshot, independent of changed-line selection."""
        names = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only',
                                         'upstream-main', 'src/shims'], cwd=ROOT).decode().splitlines()
        self.assertTrue(names)
        failures = []
        for name in names:
            if name.endswith('.js'):
                source = subprocess.check_output(['git', 'show', 'upstream-main:' + name], cwd=ROOT).decode()
                failures.extend((name, line, reason) for line, reason in CHECK.check(source))
        self.assertEqual(failures, [])


    def test_cli_selection(self):
        """Changed tracked shims fail; unrelated paths require explicit selection."""
        with tempfile.TemporaryDirectory() as directory:
            repo = pathlib.Path(directory)
            subprocess.run(['git', 'init', '-q', directory], check=True)
            (repo / 'src/shims').mkdir(parents=True)
            shim = repo / 'src/shims/example.js'
            shim.write_text('var value = 1;\n')
            other = repo / 'other.js'
            other.write_text('var other = 1;\n')
            subprocess.run(['git', '-C', directory, 'add', '.'], check=True)
            subprocess.run(['git', '-C', directory, '-c', 'user.name=Fixture',
                            '-c', 'user.email=fixture@example.invalid', 'commit',
                            '-qm', 'Initial fixture'], check=True)
            command = ['python3', str(ROOT / 'tools/format/es5_check.py'), '--base', 'HEAD']
            other.write_text('const other = 1;\n')
            result = subprocess.run(command, cwd=repo, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            result = subprocess.run(command + ['--paths', '*.js'], cwd=repo,
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertIn('other.js:1:', result.stdout)
            shim.write_text('var value = 1;\nlet next = 2;\n')
            result = subprocess.run(command, cwd=repo, capture_output=True, text=True)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertIn('src/shims/example.js:2:', result.stdout)
