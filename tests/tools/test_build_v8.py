# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Check the V8 pin and GN args of tools/v8/build_v8.sh without building V8.

See docs/build-v8.md. The script runs only in --print-args mode with a temporary
SMZ_DEPS_DIR and HOME, so nothing is fetched and the real cache is never touched.
"""

from pathlib import Path
import os
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'tools/v8/build_v8.sh'
PIN = '13.6.233.8'
REQUIRED_ARGS = {
    'is_debug': 'false',
    'v8_monolithic': 'true',
    'is_component_build': 'false',
    'use_custom_libcxx': 'false',
    'v8_use_external_startup_data': 'false',
    'v8_enable_i18n_support': 'true',
    'v8_enable_pointer_compression': 'true',
}
# A path that starts at the filesystem root (or a Windows drive) rather than
# under a variable such as $SMZ_DEPS_DIR. URLs ("https://") do not match.
ABSOLUTE = re.compile(r'(?:^|[\s"\'=(])(?:/[A-Za-z.~]|[A-Za-z]:\\)')


def run_script(*args, deps, home):
    """Run the script with an isolated environment; return the CompletedProcess."""
    env = {'PATH': os.environ.get('PATH', ''), 'HOME': home,
           'SMZ_DEPS_DIR': deps}
    return subprocess.run(['bash', str(SCRIPT), *args], env=env, cwd=home,
                          capture_output=True, text=True, timeout=60)


def parse_args_gn(text):
    """Map each `key = value` line of args.gn text to its value string."""
    pairs = {}
    for line in text.splitlines():
        match = re.fullmatch(r'\s*(\w+)\s*=\s*(.+?)\s*', line)
        if match:
            pairs[match.group(1)] = match.group(2)
    return pairs


class BuildV8PrintArgs(unittest.TestCase):
    """--print-args reports the pin and the §4.1 args and writes nothing."""

    def setUp(self):
        """Give each test its own empty deps and home directories."""
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.deps = Path(temp.name, 'deps')
        self.home = Path(temp.name, 'home')
        self.deps.mkdir()
        self.home.mkdir()

    def print_args(self):
        """Return stdout of a successful --print-args run."""
        result = run_script('--print-args', deps=str(self.deps), home=str(self.home))
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout

    def test_pinned_version(self):
        """The printed header and the script's pin both name the proposed version."""
        self.assertIn(f'# smzuggler V8 pin: {PIN}\n', self.print_args())
        source = SCRIPT.read_text(encoding='utf-8')
        self.assertRegex(source, rf'(?m)^V8_VERSION={re.escape(PIN)}$')

    def test_required_gn_args(self):
        """Every required arg is present with the decided value."""
        args = parse_args_gn(self.print_args())
        for key, value in REQUIRED_ARGS.items():
            with self.subTest(arg=key):
                self.assertEqual(args.get(key), value)
        self.assertIn(args.get('target_cpu'), ('"x64"', '"arm64"'))

    def test_print_mode_writes_nothing(self):
        """Print mode leaves the deps dir and home empty: no fetch, no caches."""
        self.print_args()
        self.assertEqual(list(self.deps.iterdir()), [])
        self.assertEqual(list(self.home.iterdir()), [])

    def test_rejects_unknown_option(self):
        """A typo fails loudly instead of starting a multi-hour build."""
        result = run_script('--print-arg', deps=str(self.deps), home=str(self.home))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('unknown option', result.stderr)
        self.assertEqual(list(self.deps.iterdir()), [])


class BuildV8Paths(unittest.TestCase):
    """The script names no absolute path, so writes can only go under its vars."""

    def test_no_absolute_paths(self):
        """Apart from the shebang, every path is built from a variable."""
        lines = SCRIPT.read_text(encoding='utf-8').splitlines()
        self.assertTrue(lines[0].startswith('#!'))
        offenders = [f'{number}: {line}' for number, line in enumerate(lines[1:], 2)
                     if ABSOLUTE.search(line)]
        self.assertEqual(offenders, [])

    def test_pattern_catches_absolute_paths(self):
        """Guard the guard: the pattern flags root paths but not URLs or vars."""
        self.assertTrue(ABSOLUTE.search('cp x /opt/v8'))
        self.assertTrue(ABSOLUTE.search('out="/tmp/v8"'))
        self.assertFalse(ABSOLUTE.search('url=https://chromium.googlesource.com/v8'))
        self.assertFalse(ABSOLUTE.search('cp x "$deps/v8/lib"'))

    def test_deps_dir_writes_are_rooted(self):
        """Every mkdir/cp/mv/rm/clone target is under $deps, $src, $out or a staging dir."""
        source = SCRIPT.read_text(encoding='utf-8')
        roots = ('"$deps', '"$src', '"$out', '"$stage', '"$prefix', '"$tools',
                 '"$CIPD_CACHE_DIR', '"$VPYTHON_VIRTUALENV_ROOT')
        for match in re.finditer(r'(?m)^\s*(?:mkdir -p|rm -rf|mv|cp(?: -R)?) (.+)$',
                                 source):
            target = match.group(1).split()[-1]
            with self.subTest(command=match.group(0).strip()):
                self.assertTrue(target.startswith(roots), target)


if __name__ == '__main__':
    unittest.main()
