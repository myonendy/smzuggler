# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Keep provider smoke failures observable with Release assertions disabled."""

from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest


class AngleSwitchReleaseTest(unittest.TestCase):
    """Exercise the real probe with deterministic EGL client responses."""

    def test_release_provider_contract(self):
        """NDEBUG must preserve success and failure for both provider choices."""
        compiler = shutil.which("cc")
        self.assertIsNotNone(compiler, "a C compiler is required for this probe")
        source = Path(__file__).resolve().parents[2] / "tests/smz/test_angle_switch.c"
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / "EGL").mkdir()
            (directory / "EGL/egl.h").write_text(
                "#define EGL_NO_DISPLAY ((void *)0)\n"
                "#define EGL_EXTENSIONS 0x3055\n"
                "const char *eglQueryString(void *, int);\n"
            )
            mock = directory / "egl.c"
            mock.write_text(
                "#include <stdlib.h>\n"
                "const char *eglQueryString(void *display, int name) {\n"
                "    (void)display; (void)name;\n"
                '    return getenv("SMZ_TEST_EGL_EXTENSIONS");\n'
                "}\n"
            )
            for provider in ("ANGLE", "SYSTEM"):
                binary = directory / ("probe_" + provider.lower())
                subprocess.run(
                    [compiler, "-DNDEBUG", "-DSMZ_GLES_" + provider + "=1",
                     "-I", str(directory), str(source), str(mock), "-o", str(binary)],
                    check=True, capture_output=True, text=True,
                )
                for extensions in (None, "EGL_EXT_client_extensions",
                                   "EGL_ANGLE_platform_angle"):
                    with self.subTest(provider=provider, extensions=extensions):
                        environment = os.environ.copy()
                        environment.pop("SMZ_TEST_EGL_EXTENSIONS", None)
                        if extensions is not None:
                            environment["SMZ_TEST_EGL_EXTENSIONS"] = extensions
                        result = subprocess.run(
                            [str(binary)], env=environment,
                            capture_output=True, text=True,
                        )
                        success = extensions is not None and (
                            (provider == "ANGLE") == ("EGL_ANGLE_" in extensions)
                        )
                        self.assertEqual(result.returncode == 0, success)
                        if not success:
                            self.assertIn("angle_switch:", result.stderr)


if __name__ == "__main__":
    unittest.main()
