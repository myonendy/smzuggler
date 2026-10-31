# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Exercise sharing without game material or real identities; see docs/sharing.md.

Why: the packager is the last step before a human posts a run, so scrubbing,
exclusions, metadata stripping and fail-closed scanning are checked here against
fabricated terms in temporary directories. No game files are used.
"""

import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'tools/share/make_bundle.py'


def chunk(kind, data):
    """Build a valid PNG chunk with a CRC so stripping preserves encoded pixels."""
    return struct.pack('>I', len(data)) + kind + data + struct.pack(
        '>I', zlib.crc32(kind + data))


def jpeg_image():
    """Encode a neutral 1x1 grayscale JPEG using minimal valid Huffman tables."""

    def segment(marker, data):
        """Wrap a JPEG segment with its big-endian length, including length bytes."""
        return bytes([255, marker]) + struct.pack('>H', len(data) + 2) + data

    tables = bytes([0, 1]) + bytes(15) + bytes([0])
    tables += bytes([16, 1]) + bytes(15) + bytes([0])
    return (
        b'\xff\xd8'
        + segment(219, bytes([0]) + bytes([1]) * 64)
        + segment(192, bytes([8, 0, 1, 0, 1, 1, 1, 17, 0]))
        + segment(196, tables)
        + segment(218, bytes([1, 1, 0, 0, 63, 0]))
        + b'?\xff\xd9'
    )


class BundleTests(unittest.TestCase):
    """Run the actual CLI in isolated directories, with fabricated private terms."""

    def setUp(self):
        """Own temporary inputs and ensure automatic cleanup after every test."""
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.base = Path(self.tmp.name)
        self.run = self.base / 'run'
        self.run.mkdir()
        self.deny = self.base / 'deny'
        self.deny.write_text('secretcanary\n')
        self.out = self.base / 'bundle.zip'
        self.env = dict(
            os.environ,
            HOME=str(self.base / 'home'),
            USER='fakeaccount',
            LOGNAME='fakeaccount',
            PRIVACY_DENYLIST='',
        )

    def invoke(self, *args):
        """Capture CLI diagnostics to check they never expose matched content."""
        return subprocess.run(
            [sys.executable, str(SCRIPT), str(self.run), '-o', str(self.out),
             '--denylist', str(self.deny), *args],
            env=self.env, capture_output=True, text=True)

    def test_scrub_and_manifest(self):
        """Scrub local identifiers and retain report provenance and machine state."""
        import socket
        originals = [self.env['HOME'], 'fakeaccount', socket.gethostname(),
                     'secretcanary']
        (self.run / 'run.log').write_text(' '.join(originals))
        (self.run / 'perf-report-test.json').write_text(json.dumps({
            'commit': 'abc123', 'version': 'test-build',
            'machine_state': {'profile': 'power-saver'},
        }))
        result = self.invoke()
        self.assertEqual(result.returncode, 0, result.stderr)
        with zipfile.ZipFile(self.out) as archive:
            text = '\n'.join(archive.read(n).decode() for n in archive.namelist())
            for original in originals:
                self.assertNotIn(original, text)
            for expected in ('~', '[redacted]', 'abc123', 'test-build',
                             'power-saver', 'run.log'):
                self.assertIn(expected, text)
            readme = archive.read('README.txt').decode()
            self.assertIn('Collector commit:', readme)
            self.assertIn('run-1/run.log: runtime log', readme)
            self.assertIn('run-1/perf-report-test.json: performance report', readme)
            dest = self.base / 'extract'
            archive.extractall(dest)
        scan = subprocess.run(
            [sys.executable, str(ROOT / 'tools/privacy/scan.py'),
             '--denylist', str(self.deny), '--paths',
             *map(str, dest.rglob('*.*'))],
            capture_output=True)
        self.assertEqual(scan.returncode, 0, scan.stderr)

    def test_images_and_exclusions(self):
        """Remove metadata byte-for-byte without altering PNG IDAT or JPEG scan data."""
        pixels = zlib.compress(b'\x00\xff\x00\x00')
        clean = b'\x89PNG\r\n\x1a\n' + chunk(
            b'IHDR', struct.pack('>IIBBBBB', 1, 1, 8, 2, 0, 0, 0))
        tail = chunk(b'IDAT', pixels) + chunk(b'IEND', b'')
        (self.run / 'shot.png').write_bytes(
            clean + chunk(b'tEXt', b'label\x00secretcanary') + tail)
        jpeg = jpeg_image()
        (self.run / 'shot.jpg').write_bytes(
            jpeg[:2] + b'\xff\xe1\x00\x08Exif\x00\x00' + jpeg[2:])
        (self.run / 'slot.rmmzsave').write_bytes(b'save')
        (self.run / 'audio').mkdir()
        (self.run / 'audio/fake.ogg').write_bytes(b'asset')
        game = self.run / 'game'
        game.mkdir()
        (game / 'private.log').write_text('game material')
        result = self.invoke('--include-screenshots', '--game-dir', str(game))
        self.assertEqual(result.returncode, 0, result.stderr)
        with zipfile.ZipFile(self.out) as archive:
            self.assertEqual(archive.read('run-1/shot.png'), clean + tail)
            self.assertEqual(archive.read('run-1/shot.jpg'), jpeg)
            self.assertNotIn('run-1/slot.rmmzsave', archive.namelist())
            self.assertIn('slot.rmmzsave', archive.read('README.txt').decode())
        for name in ('slot.rmmzsave', 'audio/fake.ogg', 'game/private.log'):
            self.assertIn(name, result.stdout)

    def test_binary_refusal(self):
        """Even binary payloads the upstream scanner ignores must fail closed."""
        (self.run / 'frames.jsonl').write_bytes(b'\x00secretcanary')
        result = self.invoke()
        self.assertEqual(result.returncode, 1)
        self.assertFalse(self.out.exists())
        self.assertNotIn('secretcanary', result.stdout + result.stderr)

    def test_missing_denylist(self):
        """A missing mandatory list is a setup error, never a silent pass."""
        self.deny.unlink()
        self.assertEqual(self.invoke().returncode, 2)
        self.assertFalse(self.out.exists())

    def test_default_exclusions_and_atomic_failure(self):
        """Do not opt into screenshots, follow links or replace an output on refusal."""
        (self.run / 'shot.png').write_bytes(b'opaque image')
        external = self.base / 'external.log'
        external.write_text('secretcanary')
        (self.run / 'linked.log').symlink_to(external)
        outside = self.base / 'outside'
        outside.mkdir()
        (outside / 'secret.log').write_text('secretcanary')
        (self.run / 'linked-dir').symlink_to(outside)
        (self.run / 'run.log').write_text('clean log')
        result = self.invoke()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('linked-dir', result.stdout)
        with zipfile.ZipFile(self.out) as archive:
            self.assertEqual(set(archive.namelist()),
                             {'README.txt', 'run-1/run.log'})
        previous = self.out.read_bytes()
        (self.run / 'frames.csv').write_bytes(b'\x00secretcanary')
        self.assertEqual(self.invoke().returncode, 1)
        self.assertEqual(self.out.read_bytes(), previous)

    def test_regex_and_relative_denylist(self):
        """Match case-insensitive regex entries and resolve the list before staging."""
        self.deny.write_text('re:canary[0-9]+\n')
        (self.run / 'runtime.log').write_text('CANARY492')
        relative = os.path.relpath(self.deny)
        result = self.invoke('--denylist', relative)
        self.assertEqual(result.returncode, 0, result.stderr)
        with zipfile.ZipFile(self.out) as archive:
            self.assertEqual(archive.read('run-1/runtime.log'), b'[redacted]')

    def test_last_run_selects_newest_state_dir(self):
        """`--last-run` packs the newest XDG state run, not an older sibling."""
        state = self.base / 'state' / 'smzuggler-rt' / 'runs'
        older = state / 'old'
        newer = state / 'new'
        older.mkdir(parents=True)
        newer.mkdir()
        (older / 'old.log').write_text('older run')
        (newer / 'new.log').write_text('newer run')
        os.utime(older, (1, 1))
        os.utime(newer, (2, 2))
        env = dict(self.env, XDG_STATE_HOME=str(self.base / 'state'))
        result = subprocess.run(
            [sys.executable, str(SCRIPT), '--last-run', '-o', str(self.out),
             '--denylist', str(self.deny)],
            env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        with zipfile.ZipFile(self.out) as archive:
            names = archive.namelist()
            self.assertIn('run-1/new.log', names)
            self.assertNotIn('run-1/old.log', names)
        missing = subprocess.run(
            [sys.executable, str(SCRIPT), '--last-run', '-o', str(self.out),
             '--denylist', str(self.deny)],
            env=dict(self.env, XDG_STATE_HOME=str(self.base / 'empty')),
            capture_output=True, text=True)
        self.assertEqual(missing.returncode, 2)


if __name__ == '__main__':
    unittest.main()
