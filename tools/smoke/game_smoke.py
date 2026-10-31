#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Contain a local game smoke run; see docs/testing.md and spec section 5.6.

The driver owns scratch storage and never relays child diagnostics. All runtime
filesystem mounts are read-only except scratch; no unsafe fallback is allowed.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def tree(root):
    """Return owned relative SHA-256 records on the driver thread; IO errors fail.

    Reject symlinks and special files rather than hash data outside the game.
    Include directory entries so empty-directory mutations are also detected.
    """
    result = {}
    pending = [root]
    while pending:
        for path in sorted(pending.pop().iterdir()):
            relative = path.relative_to(root).as_posix()
            if path.is_symlink():
                raise ValueError('game tree contains a symlink')
            if path.is_dir():
                result[relative + '/'] = 'directory'
                pending.append(path)
            elif path.is_file():
                digest = hashlib.sha256()
                with path.open('rb') as stream:
                    for block in iter(lambda: stream.read(1024 * 1024), b''):
                        digest.update(block)
                result[relative] = digest.hexdigest()
            else:
                raise ValueError('game tree contains a special file')
    return result


def configuration(args):
    """Validate caller arguments before launching; return owned paths and frames.

    Called only on the driver thread; invalid/unavailable setup raises ValueError.
    Frame counts are positive signed 32-bit integers, defaulting to 120.
    """
    raw = os.environ.get('SMZUGGLER_GAME_DIR')
    if not raw:
        raise ValueError('SMZUGGLER_GAME_DIR must name a local test copy')
    game = Path(raw).resolve(strict=True)
    forbidden = {'steam', '.steam', 'steamapps', 'compatdata', 'pfx'}
    if any(p.lower() in forbidden or p.lower().startswith('steam')
           or 'proton' in p.lower()
           for p in (*Path(raw).parts, *game.parts)):
        raise ValueError('Steam/Proton paths are refused; use a test copy')
    if not game.is_dir():
        raise ValueError('game directory is unavailable')
    frames = int(args[0]) if len(args) == 1 else 120
    if len(args) > 1 or not 1 <= frames <= 2147483647:
        raise ValueError('usage: game_smoke.sh [positive frame count]')
    runtime = shutil.which(os.environ.get('SMZUGGLER_RUNTIME', 'smzuggler-rt'))
    bwrap = shutil.which('bwrap')
    if not runtime or not bwrap:
        raise ValueError('runtime and bwrap are required for write containment')
    return game, str(Path(runtime).resolve()), bwrap, frames


def execute(game, runtime, bwrap, frames, scratch):
    """Run once with owned scratch and a live control pipe; return child status.

    No child streams escape scratch. A 120-second timeout fails the smoke run.
    The caller owns cleanup; this function retains no resources after return.
    """
    env = {key: os.environ[key] for key in
           ('PATH', 'DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY', 'LANG')
           if key in os.environ}
    authority = Path.home() / '.Xauthority'
    if 'XAUTHORITY' not in env and authority.is_file():
        env['XAUTHORITY'] = str(authority)
    for key in ('HOME', 'TMPDIR', 'XDG_DATA_HOME', 'XDG_STATE_HOME',
                'XDG_CACHE_HOME', 'XDG_CONFIG_HOME', 'XDG_RUNTIME_DIR'):
        target = scratch / key.lower()
        target.mkdir(mode=0o700)
        env[key] = str(target)
    command = [bwrap, '--ro-bind', '/', '/', '--bind', str(scratch), str(scratch),
               '--ro-bind', str(game), str(game), '--unshare-pid', '--unshare-net',
               '--die-with-parent', '--cap-drop', 'ALL', '--chdir', str(scratch),
               '--', runtime, '--game', str(game), '--control', '--frames', str(frames),
               '--shims', str(Path(__file__).resolve().parents[2] / 'src/shims')]
    with (scratch / 'runtime.log').open('wb') as log:
        with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=log,
                              stderr=log, env=env) as child:
            try:
                child.stdin.write(b'1 eval null\n')
                child.stdin.flush()
            except BrokenPipeError:
                pass
            try:
                return child.wait(timeout=120)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait()
                return 124


def main(args):
    """Return CLI status: 2 for setup, 1 for failed smoke, 0 for unchanged success.

    Only relative tree entries and fixed diagnostics are printed on this thread.
    Temporary manifests/logs are owned and removed by the driver on every exit.
    """
    try:
        game, runtime, bwrap, frames = configuration(args)
        before = tree(game)
    except (ValueError, OSError):
        print('smoke: invalid setup; set SMZUGGLER_GAME_DIR to a test copy; '
              'Steam/Proton paths refused; runtime and bwrap required', file=sys.stderr)
        return 2
    try:
        with tempfile.TemporaryDirectory(prefix='smz-smoke-') as directory:
            scratch = Path(directory)
            if scratch.is_relative_to(game):
                raise ValueError('scratch must be outside game')
            (scratch / 'before.json').write_text(json.dumps(before, sort_keys=True))
            status = execute(game, runtime, bwrap, frames, scratch)
            after = tree(game)
            (scratch / 'after.json').write_text(json.dumps(after, sort_keys=True))
            for name in sorted(before.keys() | after.keys()):
                print('before', before.get(name, 'absent'), json.dumps(name))
                print('after ', after.get(name, 'absent'), json.dumps(name))
            print(f'frames: {frames}; exit code: {status}')
            if before != after:
                print('game dir changed')
                return 1
            print('game dir unchanged')
            return 0 if status == 0 else 1
    except (OSError, ValueError):
        print('smoke: containment or tree verification failed', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
