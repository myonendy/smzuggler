#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Privacy and commit-hygiene scan for smzuggler (spec section 16).

Why this exists: smzuggler is public, and nothing personal (names, emails, user
names, host names, home paths, save files) may reach code, docs, commits,
reports or fixtures. This script is run by the pre-commit hook
(tools/privacy/pre-commit), by the forge `privacy` gate and by the CI job
`privacy-scan`. See docs/privacy.md (written in M0) and docs/spec/SPEC.md §16.

Rules a reader must not break:
  * Findings print the path, the line and the rule id, never the matched text,
    so logs cannot leak what the deny-list protects.
  * The deny-list lives outside the repository. A deny-list hit can never be
    allowed; the in-repo allow-list (tools/privacy/allowlist.txt) only
    silences reviewed generic-rule hits.
  * A missing deny-list is an error, not a silent pass.

Exit status: 0 clean, 1 findings, 2 usage or setup error.
"""
from __future__ import annotations

import argparse
import fnmatch
import os
import re
import subprocess
import sys
from pathlib import Path

PROJECT_NAME = "myonendy"
PROJECT_EMAIL = "350390780+myonendy@users.noreply.github.com"
DEFAULT_DENYLIST = "~/.config/smzuggler-rt/privacy-denylist.txt"
ALLOWLIST_REL = "tools/privacy/allowlist.txt"
SAVE_SUFFIXES = (".rmmzsave", ".rpgsave")
IMAGE_SUFFIXES = (".png", ".jpg", ".jpeg")

# Generic rules (in the repo). Each is (rule id, compiled regex).
GENERIC_RULES = [
    ("home-path", re.compile(r"(?:/home/|/Users/)[A-Za-z0-9._-]+(?:/|\b)")),
    ("home-path", re.compile(r"\b[A-Za-z]:(?:\\\\?|/)Users(?:\\\\?|/)[A-Za-z0-9._ -]+(?:\\\\?|/)")),
    ("email", re.compile(r"[A-Za-z0-9._%+-]+@[A-Za-z0-9-]+(?:\.[A-Za-z0-9-]+)*\.[A-Za-z]{2,}")),
    ("shell-prompt", re.compile(r"\b[a-z_][a-z0-9_-]{0,31}@[A-Za-z0-9][A-Za-z0-9-]{0,62}:[~/]")),
]
EMAIL_OK = re.compile(r"@(?:users\.noreply\.github\.com|example\.invalid)$", re.I)


class Finding:
    """One scan hit. Holds the location and rule only, never the matched text."""

    def __init__(self, where: str, line: int, rule: str):
        self.where, self.line, self.rule = where, line, rule

    def __str__(self) -> str:
        return f"privacy: {self.where}:{self.line}: {self.rule}"


def load_denylist(path: str | None) -> list[re.Pattern]:
    """Read deny-list entries (literal or `re:<regex>`), case-insensitive.

    Source order: the PRIVACY_DENYLIST env var's content (CI secret), then the
    given path, then the default path under ~/.config. Raises SystemExit(2)
    when no list is found, because scanning without it would pass silently.
    """
    text = os.environ.get("PRIVACY_DENYLIST")
    if not text:
        p = Path(os.path.expanduser(path or os.environ.get("SMZUGGLER_PRIVACY_DENYLIST", DEFAULT_DENYLIST)))
        if not p.is_file():
            print("privacy: deny-list not found (see docs/privacy.md); refusing to scan", file=sys.stderr)
            raise SystemExit(2)
        text = p.read_text(encoding="utf-8")
    pats = []
    for raw in text.splitlines():
        entry = raw.strip()
        if not entry or entry.startswith("#"):
            continue
        if entry.startswith("re:"):
            pats.append(re.compile(entry[3:], re.I))
        else:
            pats.append(re.compile(re.escape(entry), re.I))
    if not pats:
        print("privacy: deny-list is empty; refusing to scan", file=sys.stderr)
        raise SystemExit(2)
    return pats


def load_allowlist(root: Path) -> list[tuple[str, str, re.Pattern]]:
    """Read reviewed allow-list entries: `rule<TAB>path-glob<TAB>regex`.

    An entry silences a generic-rule hit whose matched text fully matches the
    regex in a file matching the glob. It never affects deny-list hits.
    """
    f = root / ALLOWLIST_REL
    out = []
    if not f.is_file():
        return out
    for n, raw in enumerate(f.read_text(encoding="utf-8").splitlines(), 1):
        if not raw.strip() or raw.lstrip().startswith("#"):
            continue
        parts = raw.split("\t")
        if len(parts) != 3:
            print(f"privacy: {ALLOWLIST_REL}:{n}: malformed entry (need 3 tab-separated fields)", file=sys.stderr)
            raise SystemExit(2)
        out.append((parts[0], parts[1], re.compile(parts[2])))
    return out


class Scanner:
    """Applies the deny-list, the generic rules and the allow-list to text."""

    def __init__(self, deny: list[re.Pattern], allow: list[tuple[str, str, re.Pattern]]):
        self.deny, self.allow, self.findings = deny, allow, []

    def _allowed(self, rule: str, where: str, matched: str) -> bool:
        """True when a reviewed allow-list entry covers this generic hit."""
        for a_rule, glob, rx in self.allow:
            if a_rule == rule and fnmatch.fnmatch(where, glob) and rx.fullmatch(matched):
                return True
        return False

    def scan_line(self, where: str, line_no: int, text: str, generic: bool = True) -> None:
        """Record every rule hit on one line of text (deny-list first)."""
        if any(rx.search(text) for rx in self.deny):
            self.findings.append(Finding(where, line_no, "deny-list"))
        if not generic:
            return
        for rule, rx in GENERIC_RULES:
            for m in rx.finditer(text):
                hit = m.group(0)
                if rule == "email" and EMAIL_OK.search(hit):
                    continue
                if not self._allowed(rule, where, hit):
                    self.findings.append(Finding(where, line_no, rule))

    def scan_text(self, where: str, text: str, generic: bool = True) -> None:
        """Scan each line of a text blob."""
        for n, line in enumerate(text.splitlines(), 1):
            self.scan_line(where, n, line, generic)

    def scan_path_name(self, rel: str) -> None:
        """Check a file name: forbidden save types and deny-list terms."""
        if rel.lower().endswith(SAVE_SUFFIXES):
            self.findings.append(Finding(rel, 0, "save-file"))
        self.scan_line(rel, 0, rel, generic=False)

    def scan_blob(self, rel: str, data: bytes) -> None:
        """Scan file content: image metadata for images, text for the rest."""
        if rel.lower().endswith(IMAGE_SUFFIXES):
            for chunk in image_metadata(data):
                self.findings.append(Finding(rel, 0, "image-metadata"))
                self.scan_line(rel, 0, chunk, generic=False)
            return
        if b"\0" in data[:8192]:
            return  # other binary files: name check only
        self.scan_text(rel, data.decode("utf-8", errors="replace"))


def image_metadata(data: bytes) -> list[str]:
    """Return text found in PNG text/EXIF chunks or JPEG APP1/COM segments.

    Any such chunk is itself a finding (spec §16: no metadata text chunks in
    committed images); the text is returned so the deny-list can be applied.
    """
    out = []
    if data.startswith(b"\x89PNG\r\n\x1a\n"):
        i = 8
        while i + 8 <= len(data):
            length = int.from_bytes(data[i:i + 4], "big")
            ctype = data[i + 4:i + 8]
            if ctype in (b"tEXt", b"iTXt", b"zTXt", b"eXIf"):
                out.append(data[i + 8:i + 8 + length].decode("latin-1"))
            if ctype == b"IEND":
                break
            i += 12 + length
    elif data.startswith(b"\xff\xd8"):
        i = 2
        while i + 4 <= len(data) and data[i] == 0xFF:
            marker = data[i + 1]
            if marker in (0xD9, 0xDA):
                break
            length = int.from_bytes(data[i + 2:i + 4], "big")
            if marker in (0xE1, 0xFE):
                out.append(data[i + 4:i + 2 + length].decode("latin-1"))
            i += 2 + length
    return out


def git(root: Path, *args: str) -> str:
    """Run git in the repo and return stdout; exit 2 with git's error on failure."""
    r = subprocess.run(["git", "-C", str(root), *args], capture_output=True, text=True)
    if r.returncode != 0:
        print(f"privacy: git {' '.join(args[:2])} failed: {r.stderr.strip()}", file=sys.stderr)
        raise SystemExit(2)
    return r.stdout


def git_bytes(root: Path, *args: str) -> bytes:
    """Like git(), but returns raw bytes (file contents)."""
    r = subprocess.run(["git", "-C", str(root), *args], capture_output=True)
    if r.returncode != 0:
        raise SystemExit(2)
    return r.stdout


def scan_tree(sc: Scanner, root: Path, rev: str) -> None:
    """Scan every tracked file at `rev`: names, text and image metadata."""
    for rel in git(root, "ls-tree", "-r", "--name-only", rev).splitlines():
        sc.scan_path_name(rel)
        sc.scan_blob(rel, git_bytes(root, "show", f"{rev}:{rel}"))


def scan_staged(sc: Scanner, root: Path) -> None:
    """Scan the staged diff: added lines, names of added files, staged images."""
    for rel in git(root, "diff", "--cached", "--name-only", "--diff-filter=ACMR").splitlines():
        sc.scan_path_name(rel)
        if rel.lower().endswith(IMAGE_SUFFIXES):
            sc.scan_blob(rel, git_bytes(root, "show", f":{rel}"))
    scan_diff_text(sc, git(root, "diff", "--cached", "-U0", "--no-color"))


def scan_diff_text(sc: Scanner, diff: str) -> None:
    """Scan only the added lines of a unified diff, with new-file line numbers."""
    rel, line_no = "?", 0
    for line in diff.splitlines():
        if line.startswith("+++ "):
            rel = line[6:] if line.startswith("+++ b/") else line[4:]
        elif line.startswith("@@"):
            m = re.search(r"\+(\d+)", line)
            line_no = int(m.group(1)) if m else 0
        elif line.startswith("+"):
            sc.scan_line(rel, line_no, line[1:])
            line_no += 1


def scan_commits(sc: Scanner, root: Path, rng: str) -> None:
    """Check identity and message of every commit in `rng` (e.g. BASE..HEAD).

    Author and committer must both be the project identity (spec §16).
    Upstream Outsider history is excluded by passing a range that starts at
    upstream-main or later.
    """
    fmt = "%H%x1f%an%x1f%ae%x1f%cn%x1f%ce%x1f%B%x1e"
    for rec in git(root, "log", "--no-merges", f"--format={fmt}", rng).split("\x1e"):
        rec = rec.strip("\n")
        if not rec:
            continue
        sha, an, ae, cn, ce, body = rec.split("\x1f", 5)
        where = f"commit {sha[:10]}"
        if (an, ae, cn, ce) != (PROJECT_NAME, PROJECT_EMAIL, PROJECT_NAME, PROJECT_EMAIL):
            sc.findings.append(Finding(where, 0, "commit-identity"))
        sc.scan_text(where + " message", body)


def main(argv: list[str] | None = None) -> int:
    """Parse arguments, run the requested scans and print findings."""
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--repo", default=".", help="repository root (default: .)")
    ap.add_argument("--denylist", help=f"deny-list file (default {DEFAULT_DENYLIST})")
    ap.add_argument("--staged", action="store_true", help="scan the staged diff (pre-commit)")
    ap.add_argument("--msg-file", help="also scan this commit-message file (commit-msg hook)")
    ap.add_argument("--tree", nargs="?", const="HEAD", help="scan every tracked file at REV (default HEAD)")
    ap.add_argument("--range", dest="rng", help="scan commits in A..B: identity, message, added lines")
    ap.add_argument("--paths", nargs="*", default=[], help="scan these files (reports) by path")
    a = ap.parse_args(argv)
    root = Path(git(Path(a.repo), "rev-parse", "--show-toplevel").strip())
    sc = Scanner(load_denylist(a.denylist), load_allowlist(root))
    if not (a.staged or a.msg_file or a.tree or a.rng or a.paths):
        ap.error("nothing to scan: give --staged, --msg-file, --tree, --range or --paths")
    if a.staged:
        scan_staged(sc, root)
    if a.msg_file:
        sc.scan_text("commit message", Path(a.msg_file).read_text(encoding="utf-8", errors="replace"))
    if a.tree:
        scan_tree(sc, root, a.tree)
    if a.rng:
        scan_commits(sc, root, a.rng)
        scan_diff_text(sc, git(root, "diff", "-U0", "--no-color", a.rng.replace("..", "...", 1) if "..." not in a.rng else a.rng))
    for p in a.paths:
        sc.scan_path_name(p)
        sc.scan_blob(p, Path(p).read_bytes())
    seen = set()
    for f in sc.findings:
        key = str(f)
        if key not in seen:
            seen.add(key)
            print(key)
    if sc.findings:
        print(f"privacy: {len(seen)} finding(s); fix the text (see docs/privacy.md)", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
