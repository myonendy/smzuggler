<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-10-31: add docs-touched gate release entry.
Modified 2026-10-31: record contribution guide and PR checklist.
Modified 2026-10-30: document runtime naming, CI coverage and format checks.
Modified 2026-10-31: record Markdown parser corrections.
Modified 2026-10-31: add the binary-size tool and M0 baseline.
Modified 2026-10-31: add the perf attribution tool entry.
Modified 2026-10-31: record the API-members-only surface dump.
Modified 2026-10-31: record the smaller-shim compatibility inventory.
Modified 2026-10-31: record the dom_shim.js compatibility inventory.
Modified 2026-10-31: add the share-bundle packager.
Modified 2026-10-31: record the nw.Window EventEmitter (U3).
Project documentation; identity decisions are in docs/DECISIONS.md.
-->
<!-- Records project changes for release review; keep entries free of private data. -->

# Changelog

All notable changes to smzuggler. Outsider's own history is in its git log.

## Unreleased
- Make `nw.Window.get()` a Node-style EventEmitter with the §5.2 window methods, so CleanExit-style `removeAllListeners("close")` works (U3).
- Add subtree tag lookup and simple tag/id/class selector queries with syntax errors for unsupported selectors (U2).
- Pin SoLoud and SDL_shadercross to the revisions in the offline dependency cache (U12).
- Include head, body and html in document tag lookups, including wildcard matches (U1).
- Add a read-only game smoke runner with SHA-256 tree checks and scratch XDG storage.
- Keep failed smoke diagnostics inside scratch, including when caller XDG paths name the game.
- Verify literal `name` compatibility rows through the checker CLI, including stale-row rejection.
- Recognize only the complete compatibility-table header, preserving the DOM `name` member.
- Add compatibility-surface rows for every dump member credited to `dom_shim.js`.
- Add compatibility-surface rows for every `pixi_shim.js` member; the full table
  now passes `compat_check.py` (M0-T010).
- Add `tools/share/make_bundle.py` and `smzuggler-rt --share-bundle` to pack a
  scrubbed zip of a run's logs, perf reports and frame times for sharing.
- Add `tools/perf/binsize.py`, which reports stripped `smzuggler-rt`, shared-library
  and shim sizes and a per-dependency archive breakdown, and record the M0 size baseline.
- Add the offline compatibility-surface checker and inventory for smaller shims.
- Add `tools/perf/attribute.py`, which splits `perf script` samples into the §6.1 buckets.
- Add §12 copyright lines and dated Modified notes to `.gitignore`,
  `tests/CMakeLists.txt` and `tests/integration/test_acceptance.c`; put the
  `docs/upstream-CONTRIBUTING.md` header items on separate lines.
- Skip verbatim licence texts (`LICENSE`, `LICENSES/*.txt`) by path in `headers.py`
  and list the remaining out-of-scope header violations for M0-T014b.
- Require the canonical General Arcade copyright and complete SPDX expression on
  added upstream-offer files; reject unrelated holders and substituted licences.
- Require licensing headers on prose and configuration and reject invalid UTF-8
  in checked files instead of silently treating source files as binary.
- Check renamed upstream files in `headers.py` against their source header so a
  moved Outsider file keeps upstream's licence lines.
- Add `tools/doclint/headers.py --base REV` to check spec section 12 licence headers
  and dated `Modified` notes; restore Outsider's header lines in `CMakeLists.txt`.
- Add smzuggler contribution rules, preserved upstream guidance and a tested PR checklist.
- Add the upstream offer tracker and isolated, offline branch extraction helper.
- Document upstream extraction ordering and verify atomic multi-commit replay.
- Add the architecture guide and checks for source references and directory coverage.
- Add headless `--dump-surface[=FILE]` shim inventory with sorted source attribution.
- Limit `--dump-surface` to API members: drop intrinsic function `length`,
  `name` and `prototype` rows and members of DOM elements under `document`.
- Ignore indented Markdown examples, check multiline link destinations, and
  decode heading entities while preserving literal code-span text in mdlinks.
- Add an offline Markdown link and anchor check (`tools/doclint/mdlinks.py`)
  and fix the broken architecture link in `docs/style.md`.
- Add a docs-touched gate with subsystem mappings and justified CI exemptions.
- Check documentation on compact C/C++ definitions independently in doclint.
- Add source documentation lint with changed-line checks and a legacy baseline.
- Recognize ES5 regex literals after logical operators and unary keywords.
- Added code style guidance, upstream-aligned clang-format and changed-file ES5 checks.
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
