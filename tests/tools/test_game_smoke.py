# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Exercise smoke orchestration without game material; see docs/testing.md.

Fixtures own their temporary files. The bwrap double checks the mount contract;
these tests do not claim to exercise kernel namespace enforcement.
"""
from pathlib import Path
import os
import subprocess
import shutil
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[2] / 'tools/smoke/game_smoke.sh'


class GameSmoke(unittest.TestCase):
    """Check fail-closed setup, hashing, control input and runtime status."""

    def run_smoke(self, body, game_name='game', configured=True, native=False):
        """Run isolated shell fixtures; return captured status without paths leaking."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            game = root / game_name
            game.mkdir(parents=True)
            (game / 'data').write_text('original')
            runtime = root / 'runtime'
            runtime.write_text('#!/bin/sh\n' + body)
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
                       SMZUGGLER_RUNTIME=str(runtime))
            env.pop('SMZUGGLER_GAME_DIR', None)
            if configured:
                env['SMZUGGLER_GAME_DIR'] = str(game)
            result = subprocess.run(['bash', str(SCRIPT), '7'], env=env,
                                    capture_output=True, text=True, timeout=15)
            self.assertNotIn(str(root), result.stdout + result.stderr)
            if native:
                self.assertFalse((root / 'escape').exists())
                self.assertEqual((game / 'data').read_text(), 'original')
            return result

    def test_good_runtime(self):
        """Require control mode, frame count, a live stdin and scratch XDG writes."""
        result = self.run_smoke('''test "$1" = --game || exit 8
 test "$3" = --control || exit 9
 test "$4" = --frames && test "$5" = 7 || exit 10
 test "$6" = --shims && test -d "$7" || exit 12
 read command
 test "$command" = '1 eval null' || exit 11
 echo log > "$XDG_STATE_HOME/log"
''')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('game dir unchanged', result.stdout)

    def test_game_mutations(self):
        """Detect additions, modifications and removals even if the child succeeds."""
        for command in ('echo bad > "$2/new"', 'echo bad > "$2/data"',
                        'rm "$2/data"'):
            with self.subTest(command=command):
                self.assertEqual(self.run_smoke(command).returncode, 1)

    def test_runtime_failure(self):
        """Propagate a failing runtime independently of tree equality."""
        self.assertEqual(self.run_smoke('exit 17').returncode, 1)

    def test_unsafe_configuration(self):
        """Reject missing game configuration and Steam/Proton path components."""
        self.assertEqual(self.run_smoke('exit 0', configured=False).returncode, 2)
        for name in ('Steam/game', 'steamapps/common/game', 'compatdata/game',
                     'Proton/game', 'SteamLibrary/game'):
            self.assertEqual(self.run_smoke('exit 0', game_name=name).returncode, 2)


    def test_native_containment_or_closed_failure(self):
        """Verify denied external writes, or safe failure on restricted hosts.

        Even where namespaces are unavailable the child must never run outside
        containment. The fixture checks both sentinels after every outcome.
        """
        result = self.run_smoke('''echo bad > "$2/../escape"
echo bad > "$2/data"
echo ok > "$XDG_STATE_HOME/log"
''', native=True)
        if shutil.which('bwrap'):
            self.assertIn(result.returncode, (0, 1))
            self.assertIn('game dir unchanged', result.stdout)
        else:
            self.assertEqual(result.returncode, 2)
