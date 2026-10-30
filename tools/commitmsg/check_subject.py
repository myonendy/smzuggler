#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Commit subject check for smzuggler (CONTRIBUTING.md, "Commit messages").

A subject must say at a glance what the commit adds or changes:
  * 72 characters or fewer, not empty;
  * not a bare card ID (`M0-T003`, `U5`) and not starting with one: the card ID
    belongs in a `Card: M0-T003` trailer;
  * not only generic words (`wip`, `fix`, `update`, `changes`, `misc`, ...);
  * not starting with `forge:`;
  * imperative: it does not start with a past or -ing form such as `Added`/`Fixing`.

Modes:
  --msg-file FILE   check one message (commit-msg hook); comment lines ignored.
  --range A..B      check every non-merge commit in A..B (forge gate / CI).
                    Commits in AgentForge's fixed machine format
                    (`<ID>: attempt N by <vendor>` and its two siblings) are
                    counted and reported, not failed, because forge cannot be
                    configured to write another subject (docs/FORGE.md).
                    `--strict` fails them too.
Exit 0 when every subject passes, 1 on a violation, 2 on a usage error.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys

MAX_LEN = 72
CARD_ID = r"(?:[A-Z][0-9]*-T[0-9]{3}|U[0-9]{1,2}|D[0-9]*-T[0-9]{3}|FIX-[0-9]+)"
CARD_ONLY = re.compile(rf"^\s*\[?{CARD_ID}\]?[\s:.,-]*$", re.I)
CARD_PREFIX = re.compile(rf"^\s*\[?{CARD_ID}\]?\s*[:\]-]", re.I)
FORGE_PREFIX = re.compile(r"^\s*forge\s*:", re.I)
GENERIC = {
    "wip", "fix", "fixes", "fixed", "fixup", "update", "updates", "updated", "change",
    "changes", "changed", "misc", "stuff", "things", "cleanup", "clean", "up", "tweak",
    "tweaks", "minor", "more", "some", "small", "various", "work", "temp", "tmp", "test",
    "tests", "commit", "save", "it", "the", "a", "and", "of", "code", "files", "edits",
    "edit", "progress", "squash", "do", "done", "final", "again", "try", "attempt",
}
NON_IMPERATIVE = {
    "added", "adding", "adds", "fixed", "fixing", "fixes", "updated", "updating", "updates",
    "changed", "changing", "changes", "removed", "removing", "removes", "moved", "moving",
    "renamed", "renaming", "refactored", "refactoring", "implemented", "implementing",
    "created", "creating", "deleted", "deleting", "made", "making", "ported", "porting",
    "replaced", "replacing", "improved", "improving", "wrote", "writing", "documented",
}
FORGE_MACHINE = re.compile(
    rf"^{CARD_ID}: (?:attempt [0-9]+ by [\w.-]+"
    r"|uncommitted changes picked up by (?:regate|resume)"
    r"|partial work by [\w.-]+ \(stopped by usage limit\))$")


def problems(subject: str) -> list[str]:
    """Return the rule violations of one subject line (empty list: it passes)."""
    s = subject.strip()
    out = []
    if not s:
        return ["subject is empty"]
    if len(s) > MAX_LEN:
        out.append(f"subject is {len(s)} characters; the limit is {MAX_LEN}")
    if FORGE_PREFIX.match(s):
        out.append("subject starts with 'forge:'; say what changed instead")
    if CARD_ONLY.match(s):
        out.append("subject is only a card ID; describe the change and put the ID in a 'Card:' trailer")
    elif CARD_PREFIX.match(s):
        out.append("subject starts with a card ID; move it to a 'Card:' trailer")
    words = re.findall(r"[a-z0-9']+", s.lower())
    if words and all(w in GENERIC or re.fullmatch(CARD_ID, w, re.I) for w in words):
        out.append("subject is only generic words; say what changed and where")
    if words and words[0] in NON_IMPERATIVE:
        out.append(f"subject starts with '{words[0]}'; use the imperative ('Add', 'Fix', 'Port')")
    return out


def subject_of(message: str) -> str:
    """First non-comment, non-blank line of a commit message."""
    for line in message.splitlines():
        if line.startswith("#") or not line.strip():
            continue
        return line.rstrip("\n")
    return ""


def check_range(rng: str, strict: bool, repo: str) -> int:
    """Check the subjects of every non-merge commit in rng; print one line per problem."""
    r = subprocess.run(["git", "-C", repo, "log", "--no-merges", "--format=%h%x00%s", rng],
                       capture_output=True, text=True)
    if r.returncode != 0:
        print(f"commit-subject: cannot read {rng}: {r.stderr.strip()}", file=sys.stderr)
        return 2
    bad = machine = 0
    for line in r.stdout.splitlines():
        sha, _, subj = line.partition("\x00")
        if FORGE_MACHINE.match(subj) and not strict:
            machine += 1
            continue
        for p in problems(subj):
            bad += 1
            print(f"commit-subject: {sha}: {p}")
    if machine:
        print(f"commit-subject: {machine} AgentForge machine-format commit(s) not checked "
              "(forge writes those subjects itself; see docs/FORGE.md)")
    return 1 if bad else 0


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--msg-file")
    g.add_argument("--range")
    ap.add_argument("--strict", action="store_true", help="also fail forge machine-format subjects")
    ap.add_argument("--repo", default=".")
    a = ap.parse_args(argv)
    if a.range:
        return check_range(a.range, a.strict, a.repo)
    with open(a.msg_file, encoding="utf-8") as f:
        subj = subject_of(f.read())
    errs = problems(subj)
    for p in errs:
        print(f"commit-subject: {p}", file=sys.stderr)
    if errs:
        print("commit-subject: see CONTRIBUTING.md, 'Commit messages'", file=sys.stderr)
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main())
