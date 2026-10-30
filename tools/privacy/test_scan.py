#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Unit tests for tools/privacy/scan.py (spec §16).

Why: the scan is the only thing between a slip and a public leak, so each rule
is proven to fire, and the "never print the matched text" rule is checked on
every test. Runs with `python3 -m unittest tools/privacy/test_scan.py` from the
repo root; needs git, no network, no deny-list of the real user (a fake one is
written to a temp dir).
"""
from __future__ import annotations

import io
import os
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stdout, redirect_stderr
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import scan  # noqa: E402

SECRET = "zqxsecretname"
IDENT_ENV = {
    "GIT_AUTHOR_NAME": scan.PROJECT_NAME, "GIT_AUTHOR_EMAIL": scan.PROJECT_EMAIL,
    "GIT_COMMITTER_NAME": scan.PROJECT_NAME, "GIT_COMMITTER_EMAIL": scan.PROJECT_EMAIL,
}


class ScanTest(unittest.TestCase):
    """Builds a throwaway repo per test and runs scan.main() against it."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name) / "repo"
        self.root.mkdir()
        self.deny = Path(self.tmp.name) / "deny.txt"
        self.deny.write_text(f"# comment\n{SECRET}\nre:\\bsecr3t-host\\b\n")
        self.env = dict(os.environ, **IDENT_ENV)
        self.env.pop("PRIVACY_DENYLIST", None)
        self.git("init", "-q", "-b", "main")

    def tearDown(self):
        self.tmp.cleanup()

    def git(self, *args, env=None):
        """Run git in the temp repo with the project identity."""
        subprocess.run(["git", "-C", str(self.root), *args], check=True, env=env or self.env,
                       capture_output=True)

    def write(self, rel, data):
        """Write a file (str or bytes) into the temp repo and stage it."""
        p = self.root / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data if isinstance(data, bytes) else data.encode())
        self.git("add", rel)

    def run_scan(self, *args):
        """Run the scanner; return (exit code, stdout+stderr). Asserts no secret leaks."""
        out, err = io.StringIO(), io.StringIO()
        with redirect_stdout(out), redirect_stderr(err):
            try:
                code = scan.main(["--repo", str(self.root), "--denylist", str(self.deny), *args])
            except SystemExit as e:
                code = e.code
        text = out.getvalue() + err.getvalue()
        self.assertNotIn(SECRET, text.lower(), "scan output must never contain the matched text")
        self.assertNotIn("secr3t-host", text)
        return code, text

    def test_clean_tree_passes(self):
        self.write("a.txt", "hello ~/games and $HOME and example@example.invalid\n")
        self.git("commit", "-qm", "init")
        self.assertEqual(self.run_scan("--tree")[0], 0)

    def test_denylist_literal_is_case_insensitive_and_reports_location(self):
        self.write("docs/x.md", "line one\nby ZQXSecretName here\n")
        code, text = self.run_scan("--staged")
        self.assertEqual(code, 1)
        self.assertIn("docs/x.md:2: deny-list", text)

    def test_denylist_regex_entry(self):
        self.write("x.c", "/* built on secr3t-host */\n")
        self.assertEqual(self.run_scan("--staged")[0], 1)

    def test_generic_rules_fire(self):
        cases = {
            "p1.txt": "/home/alice/x", "p2.txt": "/Users/bob/y", "p3.txt": "C:\\Users\\carol\\z",
            "e.txt": "mail me: dave@corp.example.com", "s.txt": "erin@laptop:~$ ls",
        }
        for rel, body in cases.items():
            self.write(rel, body + "\n")
        code, text = self.run_scan("--staged")
        self.assertEqual(code, 1)
        for rel in cases:
            self.assertIn(f"{rel}:1:", text)

    def test_noreply_and_placeholder_are_allowed(self):
        self.write("ok.txt", f"{scan.PROJECT_EMAIL}\n/home/<name>/ and ~/x\n")
        self.assertEqual(self.run_scan("--staged")[0], 0)

    def test_allowlist_silences_generic_but_never_denylist(self):
        self.write("tools/privacy/allowlist.txt", "email\tAUTHORS\tup@stream\\.org\n")
        self.write("AUTHORS", "up@stream.org\n")
        self.assertEqual(self.run_scan("--staged")[0], 0)
        self.write("tools/privacy/allowlist.txt", f"deny-list\tAUTHORS\t.*\nemail\tAUTHORS\t.*\n")
        self.write("AUTHORS", f"up@stream.org {SECRET}\n")
        code, text = self.run_scan("--staged")
        self.assertEqual(code, 1)
        self.assertIn("AUTHORS:1: deny-list", text)

    def test_save_files_are_forbidden(self):
        self.write("fixtures/file12.rmmzsave", b"\x00\x01")
        code, text = self.run_scan("--staged")
        self.assertEqual(code, 1)
        self.assertIn("save-file", text)

    def test_png_text_chunk_is_a_finding(self):
        import struct, zlib
        def chunk(t, d):
            return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d))
        png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", b"\0\0\0\1\0\0\0\1\x08\0\0\0\0")
               + chunk(b"tEXt", b"Author\0" + SECRET.encode()) + chunk(b"IEND", b""))
        self.write("shot.png", png)
        code, text = self.run_scan("--staged")
        self.assertEqual(code, 1)
        self.assertIn("image-metadata", text)
        self.assertIn("deny-list", text)

    def test_commit_identity_and_message(self):
        self.write("a.txt", "x\n")
        self.git("commit", "-qm", "base")
        self.write("b.txt", "y\n")
        bad = dict(self.env, GIT_AUTHOR_NAME="someone", GIT_AUTHOR_EMAIL="someone@example.invalid")
        self.git("commit", "-qm", f"fix by {SECRET}", env=bad)
        code, text = self.run_scan("--range", "HEAD~1..HEAD")
        self.assertEqual(code, 1)
        self.assertIn("commit-identity", text)
        self.assertIn("message:1: deny-list", text)

    def test_missing_denylist_is_an_error(self):
        self.deny.unlink()
        self.write("a.txt", "x\n")
        self.assertEqual(self.run_scan("--staged")[0], 2)


if __name__ == "__main__":
    unittest.main()
