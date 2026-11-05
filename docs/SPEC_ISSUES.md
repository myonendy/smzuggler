<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Contributor reference; follow the architecture and policy in ARCHITECTURE.md.
-->

# Spec issues

Open questions and contradictions found in `docs/spec/SPEC.md` (v0.5.1) while setting up.
Each needs the project owner's ruling; until then the listed default applies.

| # | Where | Issue | Default until ruled |
|---|---|---|---|
| SI-1 | §8.3 | Upstream's branch is `master`, not `main`. | `upstream-main` tracks `upstream/master` (DEC-SETUP-001). |
| SI-2 | §8.2, §9 | `up/*` branches come from `upstream/main`, but forge cards branch from `main`. | DEC-SETUP-005. |
| SI-3 | §16 | "Every commit … is authored and committed as" the project identity, but `main` contains upstream's history and every upstream sync merge brings in commits by upstream authors. | The identity check covers commits made in smzuggler (each card's range and each PR's range), not merged upstream commits. |
| SI-4 | §16 | Upstream files contain upstream's public contact and author emails (AUTHORS, LICENSE, README, CONTRIBUTING, LICENSES/COMMERCIAL.md) and a sample Windows temp path in `tests/test_paths.h`. | Reviewed entries in `tools/privacy/allowlist.txt`. |
| SI-5 | §12, §8.2 | §12 asks for the smzuggler copyright line on every file we change; §8.2 says upstream PRs carry upstream's header. | U-card changes to upstream files keep upstream's header only. |
| SI-6 | §9 M0 | "All CI gates green" needs CI on Linux, Windows and macOS, but nothing is pushed yet. | Workflows are written and checked locally; the first green run happens after the owner pushes. |
| SI-7 | §9 M0 | CONTRIBUTING.md is an upstream file with upstream's rules. | smzuggler's CONTRIBUTING.md replaces it; upstream's text is kept as `docs/upstream-CONTRIBUTING.md` for the U-PR rules. |
| SI-8 | §11 Q9 | The V8 pin is still open. | M1 card M1-T001 records the pin as `DEC-M1-T001` with the owner's approval; building V8 itself is a human checkpoint (hours of build time). |
| SI-9 | §15.6 | Soft limit of ~60 lines per function; upstream files are up to ~4,900 lines, so the forge file-size cap is set high. | `max_file_lines = 5100` until `pixi_shim.js` is split or retired in M2. |
| SI-10 | §5.3 | `require` of an unknown module should throw, "after re-scanning all 57 plugins". The plugins are game files and can't be in the repo. | The scan runs locally against the game copy and its result (plugin names and counts only) goes in the card's report. |
| SI-11 | §4.1, §9 M1 | jsb has one implementation (V8), but porting 12 bindings one at a time with green tests needs a temporary second backend. | Transitional QuickJS backend (DEC-SETUP-008), owner approved 2026-11-04. |
| SI-12 | §9 M0 | M0 lists "§5.1–5.3 and §5.6 fixes" but §5.3 also covers per-OS save paths and the Proton import, which §9 puts in M5. | M0 does the `process`/`navigator`/save-guard parts (U5–U7, M0-T020/T021); paths per OS and import stay in M5. |
| SI-13 | §9 M0 | "Reference captures from NW.js" need NW.js and the game, which only the owner has locally. | M0-T015 writes the scenarios and recipe; the owner makes the captures. |
| SI-14 | §1, §14.7 | The share bundle may include game screenshots for other developers, but §1 and §14.10 treat game screenshots as game content that the project must not ship. | Screenshots are opt-in (`--include-screenshots`) and docs/sharing.md tells the sharer to check the game's terms first. |
| SI-15 | §15, owner rule | Commit subjects must describe the change, but AgentForge writes `<ID>: attempt N by <vendor>` subjects and fast-forwards main to them; it has no option for another format. | The gate reports forge's exact machine forms and checks everything else; docs/FORGE.md lists the forge change needed. |
| SI-16 | §4.1, §9 M1 | Binding ports need a jsb runtime on the engine's QuickJS context, seam calls jsb lacks, and their callers and tests still pass `JSContext *`; none of that is in a port card's `owns`. | Prerequisite card M1-T007b and transitional register wrappers (DEC-M1-T014-JSB-ADOPT), owner approved 2026-11-04. |
