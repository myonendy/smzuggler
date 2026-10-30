# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Tests for tools/commitmsg/check_subject.py (rules in CONTRIBUTING.md)."""
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import check_subject as cs  # after the sys.path entry above

TOOL = HERE / "check_subject.py"


class SubjectRules(unittest.TestCase):
    def test_good_subjects_pass(self):
        for s in ("Add removeAllListeners and once to nw.Window shim",
                  "Port bind_audio.c to the jsb layer",
                  "Fix close event delivery from SDL_EVENT_QUIT",
                  "Embed the V8 snapshot in the binary"):
            self.assertEqual(cs.problems(s), [], s)

    def test_length_limit(self):
        self.assertEqual(cs.problems("Add " + "x" * 68), [])
        self.assertTrue(cs.problems("Add " + "x" * 69))

    def test_bare_and_prefixed_card_ids(self):
        for s in ("M0-T003", "U5", "[M1-T027]", "M0-T003:"):
            self.assertTrue(any("only a card ID" in p for p in cs.problems(s)), s)
        self.assertTrue(any("starts with a card ID" in p
                            for p in cs.problems("M0-T003: add privacy workflow")))

    def test_generic_words(self):
        for s in ("wip", "fix", "update", "changes", "misc", "fix stuff", "WIP!", "more fixes"):
            self.assertTrue(any("generic" in p for p in cs.problems(s)), s)

    def test_forge_prefix(self):
        self.assertTrue(any("forge:" in p for p in cs.problems("forge: add grok worker")))

    def test_non_imperative(self):
        self.assertTrue(cs.problems("Added the close event"))
        self.assertTrue(cs.problems("Fixing the save path"))

    def test_empty(self):
        self.assertEqual(cs.problems("   "), ["subject is empty"])

    def test_subject_skips_comments(self):
        self.assertEqual(cs.subject_of("# comment\n\nAdd X\n\nbody"), "Add X")


class Modes(unittest.TestCase):
    def run_tool(self, *args, cwd=None):
        return subprocess.run([sys.executable, str(TOOL), *args], capture_output=True,
                              text=True, cwd=cwd)

    def test_msg_file(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d) / "MSG"
            p.write_text("Add once to nw.Window shim\n\nCard: U3\n")
            self.assertEqual(self.run_tool("--msg-file", str(p)).returncode, 0)
            p.write_text("wip\n")
            r = self.run_tool("--msg-file", str(p))
            self.assertEqual(r.returncode, 1)
            self.assertIn("generic", r.stderr)

    def test_range_mode_and_forge_machine_format(self):
        with tempfile.TemporaryDirectory() as d:
            env = dict(os.environ, GIT_AUTHOR_NAME="t", GIT_AUTHOR_EMAIL="t@example.invalid",
                       GIT_COMMITTER_NAME="t", GIT_COMMITTER_EMAIL="t@example.invalid")
            def git(*a):
                subprocess.run(["git", "-C", d, *a], check=True, capture_output=True, env=env)
            git("init", "-q")
            git("commit", "-q", "--allow-empty", "--no-verify", "-m", "Start the test repo")
            git("tag", "base")
            git("commit", "-q", "--allow-empty", "--no-verify", "-m", "M0-T003: attempt 1 by codex")
            git("commit", "-q", "--allow-empty", "--no-verify", "-m", "Add the privacy workflow")
            r = self.run_tool("--repo", d, "--range", "base..HEAD")
            self.assertEqual(r.returncode, 0, r.stdout)
            self.assertIn("1 AgentForge machine-format", r.stdout)
            self.assertEqual(self.run_tool("--repo", d, "--range", "base..HEAD", "--strict").returncode, 1)
            git("commit", "-q", "--allow-empty", "--no-verify", "-m", "update")
            r = self.run_tool("--repo", d, "--range", "base..HEAD")
            self.assertEqual(r.returncode, 1)
            self.assertIn("generic", r.stdout)

    def test_bad_range_is_usage_error(self):
        with tempfile.TemporaryDirectory() as d:
            subprocess.run(["git", "init", "-q", d], check=True)
            self.assertEqual(self.run_tool("--repo", d, "--range", "nope..HEAD").returncode, 2)


if __name__ == "__main__":
    unittest.main()
