#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Contain a local game smoke run; see docs/testing.md and spec section 5.6.

The driver owns scratch storage and never relays child diagnostics. All runtime
filesystem mounts are read-only except scratch; no unsafe fallback is allowed.
"""
import errno
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

# Startup (window, GL context, script load) needs a fixed allowance; frames
# then get four times their 60 fps duration to cover slow power-saver clocks.
BASE_TIMEOUT_SECONDS = 60
TIMEOUT_MARGIN = 4

# Exact Steam directory names; Proton is matched as a substring because its
# install directories are named like "Proton 8.0" or "Proton - Experimental".
STEAM_PARTS = {'steam', '.steam', 'steamapps', 'steamlibrary', 'compatdata', 'pfx'}


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


def steam_path(parts):
    """Return True when any path component names a Steam or Proton location.

    Checked on both the given and the resolved path so a symlinked test copy
    cannot hide a Steam library behind a harmless-looking name.
    """
    return any(p.lower() in STEAM_PARTS or 'proton' in p.lower() for p in parts)


def configuration(args):
    """Validate caller arguments before launching; return owned paths and frames.

    Called only on the driver thread; invalid/unavailable setup raises ValueError
    with a path-free message. Frames are positive signed 32-bit ints, default 120.
    """
    raw = os.environ.get('SMZUGGLER_GAME_DIR')
    if not raw:
        raise ValueError('SMZUGGLER_GAME_DIR must name a local test copy')
    game = Path(raw).resolve(strict=True)
    if steam_path((*Path(raw).parts, *game.parts)):
        raise ValueError('Steam/Proton paths are refused; use a test copy')
    if not game.is_dir():
        raise ValueError('SMZUGGLER_GAME_DIR is not a directory')
    usage = 'usage: game_smoke.sh [positive frame count]'
    if len(args) > 1 or (args and not args[0].isdigit()):
        raise ValueError(usage)
    frames = int(args[0]) if args else 120
    if not 1 <= frames <= 2147483647:
        raise ValueError(usage)
    runtime = shutil.which(os.environ.get('SMZUGGLER_RUNTIME', 'smzuggler-rt'))
    if not runtime:
        raise ValueError('runtime not found; put smzuggler-rt on PATH '
                         'or set SMZUGGLER_RUNTIME')
    bwrap = shutil.which('bwrap')
    if not bwrap:
        raise ValueError('bwrap (bubblewrap) is required for write containment')
    return game, str(Path(runtime).resolve()), bwrap, frames


def session_env(scratch):
    """Build the child environment: host display sockets, scratch XDG/HOME dirs.

    XDG_RUNTIME_DIR moves to scratch, so Wayland and PulseAudio sockets are
    passed as absolute paths into the host runtime dir (readable on the
    read-only bind; connecting to a socket is not a filesystem write).
    Creates the scratch subdirectories; returns a new dict the caller owns.
    """
    env = {key: os.environ[key] for key in
           ('PATH', 'DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY', 'LANG',
            'PULSE_SERVER') if key in os.environ}
    host_runtime = os.environ.get('XDG_RUNTIME_DIR')
    wayland = env.get('WAYLAND_DISPLAY')
    if wayland and not os.path.isabs(wayland):
        if host_runtime:
            env['WAYLAND_DISPLAY'] = os.path.join(host_runtime, wayland)
        else:
            del env['WAYLAND_DISPLAY']
    pulse = os.path.join(host_runtime or '', 'pulse', 'native')
    if 'PULSE_SERVER' not in env and host_runtime and os.path.exists(pulse):
        env['PULSE_SERVER'] = 'unix:' + pulse
    authority = Path.home() / '.Xauthority'
    if 'XAUTHORITY' not in env and authority.is_file():
        env['XAUTHORITY'] = str(authority)
    for key in ('HOME', 'TMPDIR', 'XDG_DATA_HOME', 'XDG_STATE_HOME',
                'XDG_CACHE_HOME', 'XDG_CONFIG_HOME', 'XDG_RUNTIME_DIR'):
        target = scratch / key.lower()
        target.mkdir(mode=0o700)
        env[key] = str(target)
    return env


def execute(game, runtime, bwrap, frames, scratch):
    """Run once with owned scratch and a live control pipe; return child status.

    No child streams escape scratch (they go to scratch/runtime.log). The wall
    timeout scales with frames; expiry kills the child and returns 124.
    The caller owns cleanup; this function retains no resources after return.
    """
    env = session_env(scratch)
    command = [bwrap, '--ro-bind', '/', '/', '--bind', str(scratch), str(scratch),
               '--ro-bind', str(game), str(game), '--unshare-pid', '--unshare-net',
               '--die-with-parent', '--cap-drop', 'ALL', '--chdir', str(scratch),
               '--', runtime, '--game', str(game), '--control', '--frames', str(frames),
               '--shims', str(Path(__file__).resolve().parents[2] / 'src/shims')]
    timeout = BASE_TIMEOUT_SECONDS + TIMEOUT_MARGIN * frames / 60
    with (scratch / 'runtime.log').open('wb') as log:
        with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=log,
                              stderr=log, env=env) as child:
            try:
                child.stdin.write(b'1 eval null\n')
                child.stdin.flush()
            except BrokenPipeError:
                pass
            try:
                return child.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                child.kill()
                child.wait()
                return 124


def os_reason(error):
    """Name an OSError by type and errno only; its text may contain paths."""
    code = errno.errorcode.get(error.errno, 'no errno')
    return f'{type(error).__name__} {code}'


def report(before, after, frames, status):
    """Print both relative listings and the verdict; return the CLI status.

    Returns 1 if the tree changed or the runtime failed, otherwise 0.
    """
    for name in sorted(before.keys() | after.keys()):
        print('before', before.get(name, 'absent'), json.dumps(name))
        print('after ', after.get(name, 'absent'), json.dumps(name))
    print(f'frames: {frames}; exit code: {status}')
    if before != after:
        print('game dir changed')
        return 1
    print('game dir unchanged')
    return 0 if status == 0 else 1


def main(args):
    """Return CLI status: 2 for setup, 1 for failed smoke, 0 for unchanged success.

    Only relative tree entries and path-free diagnostics are printed on this
    thread. Manifests and diagnostics stay inside scratch and are removed on
    exit, including failures; the caller's XDG directories are never written.
    """
    try:
        game, runtime, bwrap, frames = configuration(args)
        before = tree(game)
    except ValueError as error:
        print(f'smoke: {error}', file=sys.stderr)
        return 2
    except OSError as error:
        print(f'smoke: cannot read SMZUGGLER_GAME_DIR ({os_reason(error)})',
              file=sys.stderr)
        return 2
    try:
        with tempfile.TemporaryDirectory(prefix='smz-smoke-') as directory:
            scratch = Path(directory)
            if scratch.is_relative_to(game):
                raise ValueError('scratch directory must be outside the game dir')
            (scratch / 'before.json').write_text(json.dumps(before, sort_keys=True))
            status = execute(game, runtime, bwrap, frames, scratch)
            after = tree(game)
            (scratch / 'after.json').write_text(json.dumps(after, sort_keys=True))
            return report(before, after, frames, status)
    except ValueError as error:
        print(f'smoke: {error}', file=sys.stderr)
        return 1
    except OSError as error:
        print(f'smoke: smoke run failed ({os_reason(error)})', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
