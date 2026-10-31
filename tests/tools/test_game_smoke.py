# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Exercise smoke orchestration without game material; see docs/testing.md.

Fixtures own their temporary files. The bwrap double checks the mount contract;
only the native test, where bubblewrap works, exercises kernel enforcement.
"""
from pathlib import Path
import os
import subprocess
import shutil
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / 'tools/smoke/game_smoke.sh'
KEPT_LOG = Path('smzuggler-rt/smoke/runtime.log')


class GameSmoke(unittest.TestCase):
    """Check fail-closed setup, hashing, control input and runtime status."""

    def run_smoke(self, body, game_name='game', configured=True, native=False,
                  kept_log=None):
        """Run isolated shell fixtures; return captured status without paths leaking.

        `@RUN@` in the stub body becomes the host XDG_RUNTIME_DIR. When
        `kept_log` is a bool, assert whether the failure log was kept.
        """
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            game = root / game_name
            game.mkdir(parents=True)
            (game / 'data').write_text('original')
            (root / 'run').mkdir()
            runtime = root / 'runtime'
            runtime.write_text('#!/bin/sh\n' + body.replace('@RUN@', str(root / 'run')))
            runtime.chmod(0o755)
            bwrap = root / 'bwrap'
            bwrap.write_text('''#!/usr/bin/env python3
import os, sys
args = sys.argv[1:]
assert args[:3] == ['--ro-bind', '/', '/']
assert '--cap-drop' in args and '--unshare-net' in args
assert args.count('--bind') == 1
assert args.count('--ro-bind') == 2
command = args[args.index('--') + 1:]
os.execv(command[0], command)
''')
            bwrap.chmod(0o755)
            if native:
                bwrap.unlink()
            env = dict(os.environ, PATH=str(root) + os.pathsep + os.environ['PATH'],
                       SMZUGGLER_RUNTIME=str(runtime), WAYLAND_DISPLAY='wayland-test',
                       XDG_RUNTIME_DIR=str(root / 'run'),
                       XDG_STATE_HOME=str(root / 'state'))
            env.pop('SMZUGGLER_GAME_DIR', None)
            env.pop('PULSE_SERVER', None)
            if configured:
                env['SMZUGGLER_GAME_DIR'] = str(game)
            result = subprocess.run(['bash', str(SCRIPT), '7'], env=env,
                                    capture_output=True, text=True, timeout=15)
            self.assertNotIn(str(root), result.stdout + result.stderr)
            if native:
                self.assertFalse((root / 'escape').exists())
                self.assertEqual((game / 'data').read_text(), 'original')
            if kept_log is not None:
                self.assertEqual((root / 'state' / KEPT_LOG).is_file(), kept_log)
            return result

    def test_good_runtime(self):
        """Require control mode, frames, live stdin, scratch XDG and Wayland path."""
        result = self.run_smoke('''test "$1" = --game || exit 8
 test "$3" = --control || exit 9
 test "$4" = --frames && test "$5" = 7 || exit 10
 test "$6" = --shims && test -d "$7" || exit 12
 test "$WAYLAND_DISPLAY" = "@RUN@/wayland-test" || exit 13
 test "$XDG_RUNTIME_DIR" != "@RUN@" || exit 14
 read command
 test "$command" = '1 eval null' || exit 11
 echo log > "$XDG_STATE_HOME/log"
''', kept_log=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('frames: 7; exit code: 0', result.stdout)
        self.assertIn('game dir unchanged', result.stdout)

    def test_game_mutations(self):
        """Detect additions, modifications and removals even if the child succeeds."""
        for command in ('echo bad > "$2/new"', 'echo bad > "$2/data"',
                        'rm "$2/data"'):
            with self.subTest(command=command):
                result = self.run_smoke(command, kept_log=True)
                self.assertEqual(result.returncode, 1)
                self.assertIn('game dir changed', result.stdout)

    def test_runtime_failure(self):
        """Propagate a failing runtime and keep its log at a relative location."""
        result = self.run_smoke('echo diagnostics; exit 17', kept_log=True)
        self.assertEqual(result.returncode, 1)
        self.assertIn('exit code: 17', result.stdout)
        self.assertIn('$XDG_STATE_HOME/' + KEPT_LOG.as_posix(), result.stdout)

    def test_unsafe_configuration(self):
        """Reject missing game configuration and Steam/Proton path components."""
        result = self.run_smoke('exit 0', configured=False)
        self.assertEqual(result.returncode, 2)
        self.assertIn('SMZUGGLER_GAME_DIR', result.stderr)
        for name in ('Steam/game', 'steamapps/common/game', 'compatdata/game',
                     'Proton 8.0/game', 'SteamLibrary/game'):
            with self.subTest(name=name):
                result = self.run_smoke('exit 0', game_name=name)
                self.assertEqual(result.returncode, 2)
                self.assertIn('Steam/Proton', result.stderr)

    def test_steam_match_is_not_prefix(self):
        """Accept ordinary names that merely start with 'steam'."""
        result = self.run_smoke('exit 0', game_name='steamed/game')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_invalid_frames(self):
        """Report the usage line for a non-numeric frame count."""
        with tempfile.TemporaryDirectory() as directory:
            env = dict(os.environ, SMZUGGLER_GAME_DIR=directory)
            result = subprocess.run(['bash', str(SCRIPT), 'many'], env=env,
                                    capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 2)
        self.assertIn('usage: game_smoke.sh', result.stderr)

    def test_native_containment_or_closed_failure(self):
        """Verify denied external writes, or safe failure on restricted hosts.

        The stub exits 0 only if both forbidden writes failed and the scratch
        write succeeded, so a passing run proves the stub ran contained; the
        driver cannot produce that status without running it.
        """
        result = self.run_smoke('''echo bad > "$2/../escape" 2>/dev/null && exit 3
echo bad > "$2/data" 2>/dev/null && exit 4
echo ok > "$XDG_STATE_HOME/log" || exit 5
exit 0
''', native=True)
        if shutil.which('bwrap'):
            self.assertIn('game dir unchanged', result.stdout)
            if result.returncode == 0:
                self.assertIn('exit code: 0', result.stdout)
            else:
                self.assertEqual(result.returncode, 1)
                self.assertNotIn('exit code: 3', result.stdout)
                self.assertNotIn('exit code: 4', result.stdout)
                self.assertNotIn('exit code: 5', result.stdout)
        else:
            self.assertEqual(result.returncode, 2)
