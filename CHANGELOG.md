<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-10-30: document runtime naming and CI coverage.
Project documentation; identity decisions are in docs/DECISIONS.md.
-->
<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The smzuggler contributors -->
<!-- Records project changes for release review; keep entries free of private data. -->

# Changelog

All notable changes to smzuggler. Outsider's own history is in its git log.

## Unreleased
- Add headless `--dump-surface[=FILE]` shim inventory with sorted source attribution.
- Harden logger scrubbing for punctuation, root homes and Windows path casing;
  register POSIX logger fixtures only on supported platforms.
- Add explicit assertions for logger privacy path and buffer boundaries.
- Scrub home path prefixes from logger console, file and callback output.
- Added SHA-pinned Linux, Windows and macOS QuickJS CI and local testing guidance.
- Added configurable smzuggler identity and the smzuggler-rt executable name.
- Handle new branches, tags and unavailable old push tips in CI subject checks.
- Added CI privacy and commit-subject checks and contributor privacy guidance.
- Forked from Outsider f789f46 as smzuggler (QuickJS mode, unchanged behaviour).
- Added the v0.5.1 spec, the privacy scan and hooks (§16), and the AgentForge setup.
- Added commit subject rules (CONTRIBUTING.md), a commit-msg hook and the `commit_subjects` gate.
