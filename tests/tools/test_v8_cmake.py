# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Configure-time checks of SMZ_JS_ENGINE and cmake/V8.cmake (spec section 4.1).

Why: a missing or wrong V8 must fail at configure time with a message naming
SMZ_V8_DIR, before any dependency is fetched, and the CMake pin must stay
equal to the pin of tools/v8/build_v8.sh. These run without a real V8: they
use fake install trees in temp dirs. The real smoke test (tests/smz/
test_v8_smoke.cc) runs only in a v8-mode build.
"""

import os
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
V8_CMAKE = ROOT / "cmake" / "V8.cmake"


def cmake_pin():
    """Return SMZ_V8_VERSION_PIN as written in cmake/V8.cmake."""
    m = re.search(r'set\(SMZ_V8_VERSION_PIN "([0-9.]+)"\)', V8_CMAKE.read_text())
    return m.group(1)


def fake_install(prefix, version, complete=True):
    """Create a fake V8 install of `version` under `prefix` (a Path).

    Only the files V8.cmake inspects are written; the library is empty, so
    the tree is good for configure checks and nothing else.
    """
    major, minor, build, patch = version.split(".")
    inc = prefix / "include"
    inc.mkdir(parents=True)
    (inc / "v8-version.h").write_text(
        f"#define V8_MAJOR_VERSION {major}\n#define V8_MINOR_VERSION {minor}\n"
        f"#define V8_BUILD_NUMBER {build}\n#define V8_PATCH_LEVEL {patch}\n")
    if complete:
        (prefix / "lib").mkdir()
        (prefix / "lib" / "libv8_monolith.a").write_bytes(b"")
        (prefix / "share").mkdir()
        (prefix / "share" / "icudtl.dat").write_bytes(b"")


class V8CMakeTest(unittest.TestCase):
    def setUp(self):
        self.tmp = pathlib.Path(tempfile.mkdtemp(prefix="smz-v8cmake-"))
        self.addCleanup(shutil.rmtree, self.tmp, True)

    def configure(self, *defs):
        """Configure the whole project in a temp build dir; return the result."""
        env = dict(os.environ)
        env.pop("SMZ_V8_DIR", None)
        return subprocess.run(
            ["cmake", "-S", str(ROOT), "-B", str(self.tmp / "build"),
             "-DFETCHCONTENT_FULLY_DISCONNECTED=ON", *defs],
            capture_output=True, text=True, env=env, timeout=300)

    def check_dir(self, v8_dir):
        """Run only V8.cmake's directory check in CMake script mode."""
        script = self.tmp / "check.cmake"
        script.write_text(f'include("{V8_CMAKE.as_posix()}")\n'
                          f'set(SMZ_V8_DIR "{v8_dir.as_posix()}")\n'
                          '_smz_v8_check_dir()\nmessage(STATUS "CHECK-OK")\n')
        return subprocess.run(["cmake", "-P", str(script)],
                              capture_output=True, text=True, timeout=60)

    def test_v8_mode_without_dir_names_the_variable(self):
        r = self.configure("-DSMZ_JS_ENGINE=v8")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("SMZ_V8_DIR", r.stderr)
        self.assertIn("SMZ_JS_ENGINE=v8", r.stderr)

    def test_v8_mode_with_wrong_version_fails(self):
        fake_install(self.tmp / "v8", "12.4.254.21")
        r = self.configure("-DSMZ_JS_ENGINE=v8", f"-DSMZ_V8_DIR={self.tmp / 'v8'}")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("SMZ_V8_DIR", r.stderr)
        self.assertIn("12.4.254.21", r.stderr)
        self.assertIn(cmake_pin(), r.stderr)

    def test_v8_mode_with_missing_dir_fails(self):
        r = self.configure("-DSMZ_JS_ENGINE=v8",
                           f"-DSMZ_V8_DIR={self.tmp / 'nowhere'}")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("is not a directory", r.stderr)

    def test_unknown_engine_is_rejected(self):
        r = self.configure("-DSMZ_JS_ENGINE=spidermonkey")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("SMZ_JS_ENGINE must be", r.stderr)
        self.assertIn("or v8", r.stderr)
        self.assertIn("not 'spidermonkey'", r.stderr)

    def test_pinned_complete_install_passes_check(self):
        fake_install(self.tmp / "v8", cmake_pin())
        r = self.check_dir(self.tmp / "v8")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn("CHECK-OK", r.stdout)

    def test_headers_without_library_fail(self):
        fake_install(self.tmp / "v8", cmake_pin(), complete=False)
        r = self.check_dir(self.tmp / "v8")
        self.assertNotEqual(r.returncode, 0)
        self.assertIn("libv8_monolith.a", r.stderr)

    def test_cmake_pin_matches_build_script(self):
        script = (ROOT / "tools" / "v8" / "build_v8.sh").read_text()
        m = re.search(r"^V8_VERSION=([0-9.]+)$", script, re.M)
        self.assertEqual(cmake_pin(), m.group(1))

    def test_defines_match_gn_args(self):
        # Pointer compression on and sandbox off in build_v8.sh mean exactly
        # these two defines; a mismatch corrupts object layout silently.
        script = (ROOT / "tools" / "v8" / "build_v8.sh").read_text()
        self.assertIn("v8_enable_pointer_compression = true", script)
        self.assertIn("v8_enable_sandbox = false", script)
        text = V8_CMAKE.read_text()
        self.assertIn('"V8_COMPRESS_POINTERS;V8_31BIT_SMIS_ON_64BIT_ARCH"', text)
        self.assertNotIn("V8_ENABLE_SANDBOX", text)


if __name__ == "__main__":
    unittest.main()
