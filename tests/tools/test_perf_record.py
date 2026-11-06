# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Exercise the perf recipe offline, preserving jitdump mmap path semantics.

The fake perf owns all temporary artifacts; no runtime or game is needed.
See docs/perf.md for the recording contract.
"""

import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
FAKE_PERF = '''#!/usr/bin/env python3
import pathlib
import sys

args = sys.argv[1:]
if args[0] == "record":
    assert args[args.index("-k") + 1] == "mono", "monotonic clock required"
    dump = pathlib.Path("jit-123.dump").resolve()
    dump.write_text("jitdump")
    pathlib.Path(args[args.index("-o") + 1]).write_text(str(dump))
elif args[0] == "inject":
    recording = pathlib.Path(args[args.index("-i") + 1])
    assert pathlib.Path(recording.read_text()).is_file(), "mmap dump missing"
    pathlib.Path("jitted-123-1.so").write_text("jit symbols")
    pathlib.Path(args[args.index("-o") + 1]).write_text("injected")
elif args[0] == "script":
    assert pathlib.Path(args[args.index("-i") + 1]).read_text() == "injected"
    print("resolved JIT sample")
else:
    raise AssertionError(args)
'''


class PerfRecordTests(unittest.TestCase):
    """Check clock selection and artifact ordering without invoking real perf."""

    def test_inject_before_archiving(self):
        """Require mmap paths to survive injection and archive generated symbols."""
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            binary = work / "bin"
            binary.mkdir()
            perf = binary / "perf"
            perf.write_text(FAKE_PERF)
            perf.chmod(0o755)
            old_dump = work / "jit-99.dump"
            old_dump.write_text("previous run")
            env = dict(os.environ, PATH=str(binary) + os.pathsep + os.environ["PATH"])
            result = subprocess.run(
                ["sh", str(ROOT / "tools/perf/record.sh"),
                 "--out", "profile output", "--", "runtime"],
                cwd=work, env=env, capture_output=True, text=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            output = work / "profile output"
            self.assertEqual((output / "run.perf.txt").read_text(), "resolved JIT sample\n")
            self.assertTrue((output / "jit-123.dump").is_file())
            self.assertTrue((output / "jitted-123-1.so").is_file())
            self.assertFalse((work / "jit-123.dump").exists())
            self.assertFalse((work / "jitted-123-1.so").exists())
            self.assertEqual(old_dump.read_text(), "previous run")


if __name__ == "__main__":
    unittest.main()
