<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-10-30: document runtime naming.
Project documentation; identity decisions are in docs/DECISIONS.md.
-->

# Decisions

One entry per non-obvious decision. Entries made by a card are named after the card
(`DEC-<card ID>`); setup decisions are `DEC-SETUP-NNN`. `§N.M` refers to
`docs/spec/SPEC.md` (v0.5.1).

## DEC-SETUP-001 — Fork point, branches and remotes
smzuggler starts at Outsider f789f46 with Outsider's full history. `main` is smzuggler;
`upstream-main` mirrors upstream's default branch and is fast-forward only (§8.3).
Upstream's default branch is called `master`, not `main` as §8.3 says; `upstream-main`
tracks `upstream/master`. The only remote is `upstream` (fetch only; its push URL is
disabled). Nothing is pushed until the project owner adds a public remote.

## DEC-SETUP-002 — Work is driven by AgentForge cards
Cards live in `tasks/*.md` (active) and `tasks/draft/*.md` (first-pass, not scheduled).
Milestones are numbered M0 = 0 … M7 = 7; `strict_milestones` means M1 starts only when
every M0 card is merged. U1–U12 and U14 are milestone 0 because the M0 exit (§9) needs
them; U13 waits for the M3 root cause. D cards (inherited docs) are drafts until the
doclint tool from M0 exists. See `docs/FORGE.md`.

## DEC-SETUP-003 — QuickJS warning with GCC 16
QuickJS v0.9.0 compiles itself with `-Werror`; GCC 16 rejects a const assignment in
`quickjs.c` (`-Werror=discarded-qualifiers`). The build passes
`-Wno-error=discarded-qualifiers` in `CMAKE_C_FLAGS`, which keeps the warning visible but
stops it failing the build. QuickJS leaves the tree in M1, which ends this.

## DEC-SETUP-004 — Privacy scan runs as a hook and as a forge gate
§16's hook (`tools/privacy/pre-commit`) runs on human commits. AgentForge commits agent
work with `--no-verify`, so the same scan also runs as the `privacy` gate on every card
(whole tree, plus identity, message and added lines of every commit since the card's
base). The deny-list is outside the repo at `~/.config/smzuggler-rt/privacy-denylist.txt`
(path from §16) and is hidden from OpenCode sandboxes.

## DEC-SETUP-005 — Upstream PR work inside forge
Forge cards branch from `main`, not from `upstream-main`. A U card therefore makes its
fix on `main`, limited to the upstream files of its §8.1 row, in ES5 and with upstream's
header (not the smzuggler copyright line: the patch is assigned to General Arcade, §8.2).
`tools/upstream/cut_up_branch.sh` (card M0-T013) then replays those commits on a fresh
`up/<id>-<slug>` branch cut from `upstream-main` and builds and tests it with QuickJS.
While M0 runs, `main` is still in QuickJS mode, so the card's own gates already test
the fix with QuickJS. Opening the PR is the owner's step.

## DEC-SETUP-006 — Gates that arrive with their M0 cards
Cards may not edit `forge.toml` or `tools/forge/**`. The format, doclint, docs-touched,
header, compat-surface and game-smoke gates are written by M0 cards and wired into
`[gates]` by a human once each card is merged (`docs/FORGE.md`, "Human checkpoints").

## DEC-SETUP-007 — Game access
No game files are in the repo. Tests and tools find the game through
`SMZUGGLER_GAME_DIR` (or Outsider's `RMMZ_TEST_GAME_DIR`), pointed at a disposable copy,
never at the Steam install. Fixture saves are read from
`$XDG_DATA_HOME/smzuggler-rt/fixtures/` (§14.10) and never committed.

## DEC-SETUP-008 — Transitional QuickJS backend for jsb during M1 (proposed)
§4.1 says the `jsb` seam has one implementation (V8). To port the 12 bindings one card at a
time with every test green, M1 adds a transitional QuickJS backend (`src/jsb/jsb_quickjs.c`,
card M1-T004) and an `SMZ_JS_ENGINE` build option; M1-T027 removes both. This needs the
owner's approval (docs/SPEC_ISSUES.md SI-11). The alternative is a single big-bang port card.

## DEC-SETUP-009 — Where new tests live
Upstream registers each test by hand in `tests/CMakeLists.txt`, and forge treats edits to
existing test files as tampering. smzuggler tests therefore live in `tests/smz/`, each with
its own `tests/smz/<name>.cmake` fragment that `tests/CMakeLists.txt` includes by glob.
Python tool tests live in `tests/tools/` (run by `tools/forge/test.sh`). U cards add tests to
upstream's existing test files in upstream's style (`may_edit_tests = true`, additions only)
so their patches apply upstream unchanged.

## DEC-SETUP-010 — Claude is review-only
Claude may review code (`agents.review_pool`) and is never a worker, fallback worker,
planner or auditor. `allow_overage = false`: a Claude run that reaches paid overage is
stopped. Caps in `forge.toml` are conservative placeholders for the owner to set.

## DEC-SETUP-011 — Build gate uses build/; the binary rename edits one upstream assertion
Upstream's acceptance test (`build_pipeline_structure` in `tests/integration/test_acceptance.c`)
looks for the binary at `build/outsider`, so `tools/forge/build.sh` builds in each worktree's
`build/`. M0-T001 renames the binary and is the only setup card allowed to edit that one
assertion (name only, same ASSERT count, `may_edit_tests = true`).

## DEC-SETUP-012 — Commit identity is myonendy
The owner changed the project identity on 2026-10-30, after spec v0.5.1 was written: every
commit is authored and committed as
`myonendy <350390780+myonendy@users.noreply.github.com>`. §16 of `docs/spec/SPEC.md`,
`docs/spec/CHANGES.md`, the pre-commit hook and `tools/privacy/scan.py` say so. The one
setup commit made under the earlier identity was rewritten before anything was pushed.

## DEC-SETUP-013 — Commit subjects say what changed
Owner rule (2026-10-30): imperative, specific subjects of at most 72 characters; card IDs
go in a `Card:` trailer; no generic or `forge:` subjects (CONTRIBUTING.md). Enforced by the
commit-msg hook and the `commit_subjects` gate (`tools/commitmsg/`). AgentForge's fixed
`<ID>: attempt N by <vendor>` subjects cannot be configured; the gate reports them and
docs/FORGE.md lists the forge change that would remove the exception (SI-15).

## DEC-M0-T001 — Keep the upstream target, configure the runtime identity
The CMake target remains `outsider` to keep upstream merges mechanical. Desktop
output and installation use `SMZUGGLER_BIN`; Android retains `libmain.so` because
SDLActivity requires it. Cache strings feed the shared names header and platform
alias; standalone compilation uses the same defaults. This changes naming only.
