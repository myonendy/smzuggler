<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-10-30: document runtime naming and format policy.
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

## DEC-SETUP-010 — Claude works and reviews, never on overage
Until 2026-10-31 Claude was review-only. From 2026-10-31 (owner) Claude is a worker
alongside Codex (`workers = ["codex", "claude"]`) and reviews (`review_pool`). Grok is the
last-resort fallback worker, used only when Codex and Claude are both cooling
(`fallback_workers = ["codex", "claude", "grok"]`). Planner and auditor stay Codex.
`allow_overage = false`: a Claude run that reaches paid overage is stopped. Caps in
`forge.toml` are conservative placeholders for the owner to set.

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

## DEC-M0-T005 — Changed-line formatting and dependency-free ES5 guard

Use clang-format with four-space LLVM-derived layout, Linux braces, unlimited
columns, preserved include order and single-line guards to match upstream.
Measured with clang-format 23.1.3 against unchanged `upstream-main:src/main.c`:
396 source lines, 2 removed and 5 added. Residuals are one preprocessor-split call's
closing parenthesis and expansion of one compact multi-statement conditional.
No upstream sources were reformatted. Changed-line diagnostics filter formatter
replacements against Git's new-side lines; deleted lines and untracked files are
outside the check. A base revision is mandatory and invalid revisions fail loudly.

For now use a Python tokenizer for ES5 syntax instead of adding Node, eslint or
prettier dependencies to offline gates. Check whole changed shared shim files;
`--paths` selects explicit repository-relative globs for other shared locations.
The checker is deliberately not a complete JavaScript grammar validator; regex
recognition uses expression context, including logical operators and unary
keywords; regression fixtures prevent regex contents becoming syntax diagnostics.
All 13 upstream shim files pass the guard.
Full JS formatting and jsdoc tooling remain future work under §15.6/§15.7.
A human wires the commands into gates after merge (DEC-SETUP-006).
`ARCHITECTURE.md` is absent at this card's baseline; the style guide links its
intended location for the architecture documentation card to supply.
## DEC-M0-T004 — Filter diagnostics before sink dispatch
The engine filters the formatted message once before console, file and callback
output, so callback consumers cannot bypass the privacy boundary. File-open
failures filter their path separately. The allocation-free filter accepts both
slash styles and checks the end of the home component to preserve sibling paths.
The test fragment attaches the filter source to existing test targets that compile
`error_handler.c` directly; this keeps upstream test definitions unchanged.
User and host identity lookup APIs are absent from `src`; diagnostics must not add
such identity sources. See [privacy.md](privacy.md).

## DEC-M0-T009 — Observe shims in an isolated engine
The surface dump uses the loader's combined shim list and snapshots own property
descriptors after each evaluation. It records additions and replacements without
calling getters, constructing browser objects, or initializing devices. The native
file binding is part of the host baseline and temporarily resolves paths inside
the selected shim directory, so storage startup cannot probe a game or the caller's
working directory. The prior file root is restored for in-process callers.
Build configuration stages shims inside the build tree for the default CLI path.
Post-core patches run without game classes and retain their ordinary guards.
## DEC-M0-T005b — Inherited format violations remain visible

Validation after merging the logger card reports `tests/smz/test_log_scrub.c:17`
and `:42` against `upstream-main`: compact multi-statement and nested conditional
lines expand under clang-format. These existing tests are outside this card’s
ownership and cannot be edited here. Keep diagnostics visible rather than
excluding tests or changing the measured upstream style to hide the failures.

## DEC-M0-T014 — Header gate scope and what counts as a modification

`tools/doclint/headers.py` checks all regular changed files, including Markdown,
TOML, YAML, and extensionless configuration. Only explicit formats without comment
syntax are skipped; its module docstring and `SKIPPED_SUFFIXES` list them. Checked
files must be UTF-8: decoding failures produce a licensing error rather than a
binary-file exemption. Symlinks and deletions are ignored.
A changed file whose base header names only smzuggler follows the new-file rule.
Changed upstream files retain their copyright and SPDX lines and require an
additional dated modification note. Header lines may move within the first 40
lines; comment delimiters do not form part of the licensing text.
A rename that git detects (`--find-renames`, default 50% similarity) is checked
as a change against the source path's base header, so a moved Outsider file
keeps upstream's copyright and SPDX lines and adds ours and a dated note.
Untracked files cannot be paired with a source and count as added.
In `--upstream` mode, added files must carry exactly the canonical General Arcade
copyright and complete dual-license SPDX expression, in upstream order. An
unrelated holder or substituted licence expression fails even if both kinds of
line are present; existing offers retain their base header unchanged.

Verbatim licence texts (`LICENSE`, `LICENSES/*.txt`) are skipped by explicit
path, because a licence text must be distributed unaltered; project-written prose
such as `LICENSES/README.md` is still checked.

Following DEC-M0-T014-SCOPE, the full-tree run (`headers.py --base upstream-main`,
2026-10-31) is clean for every file this card owns, adds or changes, and reports
262 problems in 131 files outside `owns`, left for draft card M0-T014b. No
exemption list hides them and no check was relaxed.
- Outside `owns`, fixable by M0-T014b: `.gitignore`, `CONTRIBUTING.md`,
  `tests/CMakeLists.txt`, `tests/integration/test_acceptance.c` (each changed
  upstream file lacks our copyright line and a dated Modified note).
- Protected, for a human (no SPDX or smzuggler copyright line unless noted):
  `.gitattributes` (upstream file: our line and Modified note missing),
  `AGENTS.md`, `CLAUDE.md`, `forge.toml`, `LICENSES/README.md`,
  `.claude/agents/investigator.md`, `.claude/hooks/guard.sh`,
  `.codex/config.toml`, `docs/spec/CHANGES.md`, `docs/spec/README.md`,
  `docs/spec/SPEC.md`, `docs/spec/V8_MZ_HOST_SPEC_v0.2.md`,
  `prompts/auditor.md`, `prompts/planner.md`, `prompts/reviewer.md`,
  `prompts/worker.md`, `tools/privacy/allowlist.txt`, and 110 task cards under
  `tasks/` (`tasks/*.md` and `tasks/draft/*.md`).
## DEC-M0-T013 — Upstream extraction uses exact paths and atomic publication

The tracker expands abbreviated §8.1 filenames to their upstream-tree paths and
includes the existing test files allowed by DEC-SETUP-005. U13's provisional shim
path is canvas2d_shim.js; revisit the row if root-cause analysis needs another
file, rather than permitting every shim. Mixed-path commits are rejected whole,
not filtered: silently discarding part of a fix could produce an invalid offer.
The helper replays in a detached temporary worktree and publishes the local topic
branch only after validation and optional offline QuickJS build/ctest succeed.
## DEC-M0-T009b — What counts as a dump "member"
Spec §15.4 defines the surface as "every global and prototype member the shims install".
Intrinsic function properties (`length`, `name`, and `prototype` as a row of its own) are not
APIs, and the members of instances reached from `document` (`document.body.*` and so on) repeat
`HTMLElement.prototype`. The dump leaves both out. Nothing a shim installs on a global,
constructor, prototype or namespace is dropped, `_`-prefixed members included. No test or gate
gets weaker: the existing dump test is unchanged, a new test pins the exclusions, and the
compat-surface check stays exact table-vs-dump equality.

## DEC-M0-T010-SPLIT — Seed the compat table in three cards
Even after DEC-M0-T009b the table has about 2,000 rows, more than one card's 1,500-line SIZE
cap, so the table is seeded per shim file. M0-T010a adds the checker (with `--shim`) and the
small shims. M0-T010b adds dom_shim.js. M0-T010 adds pixi_shim.js, and the full check with no
`--shim` must pass there. M0-T010b and M0-T010 are data-only (`tests_required = false`): the
checker run in each report is their test, and the `compat_surface` gate takes over after
M0-T010. `max_diff_lines` is unchanged.

## DEC-SETUP-014 — Upstream contact address in docs/upstream-CONTRIBUTING.md
`docs/upstream-CONTRIBUTING.md` must be byte-identical to upstream's CONTRIBUTING.md (M0-T012,
SI-7), and that file contains Outsider's public company address.
That address is already allow-listed for the inherited CONTRIBUTING.md, LICENSE and README.md.
The allow-list gets the same exact-match entry for the preserved copy. It is a generic-rule
override for one public, non-personal address in one file. Deny-list rules are untouched, and
the scan still flags any other email.

## DEC-M0-T014-SCOPE — Header check: full-tree pass moves to M0-T014b
M0-T014 blocked after four attempts because "`headers.py --base upstream-main` passes on the
tree" required header edits to files outside its `owns`, some of them protected (AGENTS.md,
forge.toml, tasks/**, tools/forge/** and so on) and some upstream tests. M0-T014 now has to be
clean only for the files it owns, adds or changes, and must list every remaining violation;
verbatim licence texts are skipped by explicit path. No exemption list is added and no check is
relaxed: the full-tree pass is still required, by draft card M0-T014b plus a human for the
protected files, before `headers.py` becomes a gate.

## DEC-SETUP-015 — Cards that add a src/ or tools/ directory own ARCHITECTURE.md
`tests/tools/test_architecture_refs.py::test_directory_map` requires an ARCHITECTURE.md row for
every directory under `src/` and `tools/`. Cards that create one (M0-T013, M0-T016, M0-T017,
M0-T019, M0-T020, M0-T023, M1-T001, M1-T003) own ARCHITECTURE.md, limited to adding that row.

## DEC-M0-T014b — Remaining section 12 notices

Add copyright and dated modification comments to the owned upstream files;
CONTRIBUTING.md already had valid notices, which are retained. Normalize the
archived upstream contribution guide header into separate comment lines so the
existing checker recognizes its SPDX expression. No checker exemptions change.

Validation command: `python3 tools/doclint/headers.py --base upstream-main`.
Exit status: 1. Output follows verbatim; every remaining violation is protected
and requires a human header edit before the full-tree check can pass.

```text
headers: 262 problem(s); see spec section 12
.claude/agents/investigator.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
.claude/agents/investigator.md: missing "Copyright (C) 2026 The smzuggler contributors"
.claude/hooks/guard.sh: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
.claude/hooks/guard.sh: missing "Copyright (C) 2026 The smzuggler contributors"
.codex/config.toml: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
.codex/config.toml: missing "Copyright (C) 2026 The smzuggler contributors"
.gitattributes: missing "Copyright (C) 2026 The smzuggler contributors"
.gitattributes: needs a new "Modified YYYY-MM-DD: <what changed>" note
AGENTS.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
AGENTS.md: missing "Copyright (C) 2026 The smzuggler contributors"
CLAUDE.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
CLAUDE.md: missing "Copyright (C) 2026 The smzuggler contributors"
LICENSES/README.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
LICENSES/README.md: missing "Copyright (C) 2026 The smzuggler contributors"
docs/spec/CHANGES.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
docs/spec/CHANGES.md: missing "Copyright (C) 2026 The smzuggler contributors"
docs/spec/README.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
docs/spec/README.md: missing "Copyright (C) 2026 The smzuggler contributors"
docs/spec/SPEC.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
docs/spec/SPEC.md: missing "Copyright (C) 2026 The smzuggler contributors"
docs/spec/V8_MZ_HOST_SPEC_v0.2.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
docs/spec/V8_MZ_HOST_SPEC_v0.2.md: missing "Copyright (C) 2026 The smzuggler contributors"
forge.toml: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
forge.toml: missing "Copyright (C) 2026 The smzuggler contributors"
prompts/auditor.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
prompts/auditor.md: missing "Copyright (C) 2026 The smzuggler contributors"
prompts/planner.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
prompts/planner.md: missing "Copyright (C) 2026 The smzuggler contributors"
prompts/reviewer.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
prompts/reviewer.md: missing "Copyright (C) 2026 The smzuggler contributors"
prompts/worker.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
prompts/worker.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T001-names-smzuggler-rt-binary-smzuggler-name.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T001-names-smzuggler-rt-binary-smzuggler-name.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T002-ci-workflow-build-and-test-in-quickjs-mo.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T002-ci-workflow-build-and-test-in-quickjs-mo.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T003-ci-privacy-job-and-docs-privacy-md.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T003-ci-privacy-job-and-docs-privacy-md.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T004-logger-privacy-filter-home-path-becomes.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T004-logger-privacy-filter-home-path-becomes.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T005-docs-style-md-formatter-configs-and-a-ch.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T005-docs-style-md-formatter-configs-and-a-ch.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T006-doc-lint-file-headers-and-function-doc-c.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T006-doc-lint-file-headers-and-function-doc-c.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T007-docs-touched-gate-tool.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T007-docs-touched-gate-tool.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T008-markdown-link-and-anchor-check.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T008-markdown-link-and-anchor-check.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T009-dump-surface-list-the-js-surface-the-shi.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T009-dump-surface-list-the-js-surface-the-shi.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T009b-dump-surface-api-members-only.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T009b-dump-surface-api-members-only.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T010-compat-surface-table-seeded-from-the-shi.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T010-compat-surface-table-seeded-from-the-shi.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T010a-compat-check-tool-and-small-shim-rows.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T010a-compat-check-tool-and-small-shim-rows.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T010b-compat-surface-rows-dom-shim.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T010b-compat-surface-rows-dom-shim.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T011-architecture-md-first-version.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T011-architecture-md-first-version.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T012-contributing-md-and-pr-template-for-smzu.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T012-contributing-md-and-pr-template-for-smzu.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T013-docs-upstream-md-and-tools-upstream-cut.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T013-docs-upstream-md-and-tools-upstream-cut.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T014-header-check-gpl-headers-and-modified-ma.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T014-header-check-gpl-headers-and-modified-ma.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T014b-headers-for-files-outside-m0-t014-owns.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T014b-headers-for-files-outside-m0-t014-owns.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T015-game-scenarios-for-tools-evaluator.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T015-game-scenarios-for-tools-evaluator.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T016-read-only-game-smoke-run.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T016-read-only-game-smoke-run.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T017-perf-attribution-tool-6-1.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T017-perf-attribution-tool-6-1.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T018-map-92-frame-profile-report-in-quickjs-m.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T018-map-92-frame-profile-report-in-quickjs-m.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T019-measure-binary-size.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T019-measure-binary-size.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T020-process-argv-cwd-mainmodule-exit-throwin.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T020-process-argv-cwd-mainmodule-exit-throwin.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T021-smzuggler-data-directories-on-by-default.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T021-smzuggler-data-directories-on-by-default.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T022-m0-exit-check.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T022-m0-exit-check.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M0-T023-share-bundle-scrubbed-zip-of-a-run-s-log.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M0-T023-share-bundle-scrubbed-zip-of-a-run-s-log.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T001-v8-pin-and-reproducible-v8-build-script.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T001-v8-pin-and-reproducible-v8-build-script.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T002-cmake-import-prebuilt-v8-and-add-smz-js.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T002-cmake-import-prebuilt-v8-and-add-smz-js.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T003-jsb-seam-core-api-on-v8.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T003-jsb-seam-core-api-on-v8.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T004-transitional-quickjs-backend-for-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T004-transitional-quickjs-backend-for-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T005-jsb-zero-copy-buffers.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T005-jsb-zero-copy-buffers.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T006-jsb-persistent-callbacks.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T006-jsb-persistent-callbacks.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T007-js-engine-on-v8-behind-the-existing-js-e.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T007-js-engine-on-v8-behind-the-existing-js-e.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T008-script-watchdog-with-terminateexecution.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T008-script-watchdog-with-terminateexecution.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T009-port-game-loop-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T009-port-game-loop-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T010-microtasks-kexplicit-with-checkpoints-af.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T010-microtasks-kexplicit-with-checkpoints-af.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T011-port-error-handler-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T011-port-error-handler-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T012-v8-code-cache-in-the-cache-dir.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T012-v8-code-cache-in-the-cache-dir.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T013-replace-stack-and-js-stack-size-with-sta.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T013-replace-stack-and-js-stack-size-with-sta.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T014-port-bind-platform-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T014-port-bind-platform-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T015-port-bind-input-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T015-port-bind-input-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T016-port-bind-io-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T016-port-bind-io-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T017-port-bind-image-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T017-port-bind-image-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T018-port-bind-font-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T018-port-bind-font-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T019-port-bind-audio-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T019-port-bind-audio-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T020-port-bind-video-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T020-port-bind-video-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T021-port-bind-canvas2d-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T021-port-bind-canvas2d-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T022-port-bind-effekseer-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T022-port-bind-effekseer-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T023-port-bind-renderer-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T023-port-bind-renderer-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T024-port-bind-filters-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T024-port-bind-filters-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T025-port-bind-tilemap-c-to-jsb.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T025-port-bind-tilemap-c-to-jsb.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T026-port-direct-quickjs-tests-to-the-jsb-js.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T026-port-direct-quickjs-tests-to-the-jsb-js.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T027-v8-becomes-the-only-engine.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T027-v8-becomes-the-only-engine.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T028-cpu-prof-writes-a-cpuprofile.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T028-cpu-prof-writes-a-cpuprofile.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T029-js-flags-pass-through-for-perf-prof-prof.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T029-js-flags-pass-through-for-perf-prof-prof.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T030-docs-js-binding-md-complete-doc-comments.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T030-docs-js-binding-md-complete-doc-comments.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T031-jitless-run-measurement.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T031-jitless-run-measurement.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T032-2-test-drive-repeated-on-v8.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T032-2-test-drive-repeated-on-v8.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/M1-T033-gate-g1-map-92-js-time-on-v8.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/M1-T033-gate-g1-map-92-js-time-on-v8.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U1-dom-head-body.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U1-dom-head-body.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U10-jsc-cache-dir.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U10-jsc-cache-dir.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U11-canplaytype.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U11-canplaytype.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U12-pin-soloud-shadercross.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U12-pin-soloud-shadercross.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U14-pixi-prof-warnings.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U14-pixi-prof-warnings.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U2-element-queries.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U2-element-queries.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U3-nw-window-events.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U3-nw-window-events.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U4-window-close-event.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U4-window-close-event.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U5-process-env-platform.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U5-process-env-platform.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U6-navigator-platform.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U6-navigator-platform.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U7-refuse-bad-save-paths.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U7-refuse-bad-save-paths.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U8-pixi-shapes.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U8-pixi-shapes.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/U9-data-dir-logs.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/U9-data-dir-logs.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/D-T001-inherited-docs-src-audio-headers-functio.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/D-T001-inherited-docs-src-audio-headers-functio.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/D-T002-inherited-docs-src-bindings-headers-func.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/D-T002-inherited-docs-src-bindings-headers-func.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/D-T003-inherited-docs-src-effects-headers-funct.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/D-T003-inherited-docs-src-effects-headers-funct.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/D-T004-inherited-docs-src-engine-headers-functi.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/D-T004-inherited-docs-src-engine-headers-functi.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/D-T005-inherited-docs-src-input-headers-functio.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/D-T005-inherited-docs-src-input-headers-functio.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/D-T006-inherited-docs-src-io-headers-function-c.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/D-T006-inherited-docs-src-io-headers-function-c.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/D-T007-inherited-docs-src-platform-headers-func.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/D-T007-inherited-docs-src-platform-headers-func.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/D-T008-inherited-docs-src-rendering-headers-fun.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/D-T008-inherited-docs-src-rendering-headers-fun.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/D-T009-inherited-docs-src-shims-headers-functio.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/D-T009-inherited-docs-src-shims-headers-functio.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/D-T010-inherited-docs-src-video-headers-functio.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/D-T010-inherited-docs-src-video-headers-functio.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M2-T001-webgl2-context-binding-over-gles-3-0-sta.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M2-T001-webgl2-context-binding-over-gles-3-0-sta.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M2-T002-webgl2-textures-fbos-renderbuffers-with.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M2-T002-webgl2-textures-fbos-renderbuffers-with.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M2-T003-webgl2-shaders-getextension-and-getparam.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M2-T003-webgl2-shaders-getextension-and-getparam.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M2-T004-angle-build-on-windows-macos-and-as-the.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M2-T004-angle-build-on-windows-macos-and-as-the.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M2-T005-real-pixi-mode-load-the-game-s-pixi-js-s.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M2-T005-real-pixi-mode-load-the-game-s-pixi-js-s.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M2-T006-frame-timing-instrumentation-frame-stats.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M2-T006-frame-timing-instrumentation-frame-stats.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M2-T007-14-harness-perf-suite-input-replay-impor.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M2-T007-14-harness-perf-suite-input-replay-impor.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M2-T008-outside-tester-kit-scrub-report-deck-pro.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M2-T008-outside-tester-kit-scrub-report-deck-pro.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M2-T009-m2-exit-14-acceptance-s1-s3-on-dev-lapto.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M2-T009-m2-exit-14-acceptance-s1-s3-on-dev-lapto.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M3-T001-root-cause-the-missing-hud-text-5-5.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M3-T001-root-cause-the-missing-hud-text-5-5.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M3-T002-fix-the-hud-text-cause.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M3-T002-fix-the-hud-text-cause.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M3-T003-per-font-golden-tests-within-q7-toleranc.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M3-T003-per-font-golden-tests-within-q7-toleranc.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M4-T001-opus-source-and-webm-demux-nestegg-for-s.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M4-T001-opus-source-and-webm-demux-nestegg-for-s.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M4-T002-decodeaudiodata-of-truncated-ogg.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M4-T002-decodeaudiodata-of-truncated-ogg.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M4-T003-webaudio-node-semantics-on-soloud.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M4-T003-webaudio-node-semantics-on-soloud.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M5-T001-save-paths-per-os-and-game-profile-c.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M5-T001-save-paths-per-os-and-game-profile-c.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M5-T002-game-and-config-file-lookup.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M5-T002-game-and-config-file-lookup.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M5-T003-one-time-proton-save-import-read-only-so.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M5-T003-one-time-proton-save-import-read-only-so.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M5-T004-byte-identical-saves-vs-nw-js.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M5-T004-byte-identical-saves-vs-nw-js.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M6-T001-vp9-opus-video-via-nestegg-libvpx-behind.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M6-T001-vp9-opus-video-via-nestegg-libvpx-behind.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M6-T002-effekseer-the-game-s-wasm-on-v8-q10.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M6-T002-effekseer-the-game-s-wasm-on-v8-q10.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M6-T003-coverage-route-over-all-57-plugins-and-t.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M6-T003-coverage-route-over-all-57-plugins-and-t.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M6-T004-docs-adding-an-api-md-with-two-worked-ex.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M6-T004-docs-adding-an-api-md-with-two-worked-ex.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M6-T005-m6-exit-14-s4-s8-on-dev-laptop.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M6-T005-m6-exit-14-s4-s8-on-dev-laptop.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M7-T001-steam-deck-build-sniper-runtime-and-linu.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M7-T001-steam-deck-build-sniper-runtime-and-linu.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M7-T002-windows-and-macos-packages-jit-entitleme.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M7-T002-windows-and-macos-packages-jit-entitleme.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M7-T003-appimage-flatpak-and-non-steam-game-doc.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M7-T003-appimage-flatpak-and-non-steam-game-doc.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M7-T004-30-minute-soak-and-full-14-suite-on-dev.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M7-T004-30-minute-soak-and-full-14-suite-on-dev.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M7-T005-deck-confirmation-pass.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M7-T005-deck-confirmation-pass.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/M7-T006-docs-build-md-docs-testing-md-and-releas.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/M7-T006-docs-build-md-docs-testing-md-and-releas.md: missing "Copyright (C) 2026 The smzuggler contributors"
tasks/draft/U13-canvas2d-text-fix.md: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tasks/draft/U13-canvas2d-text-fix.md: missing "Copyright (C) 2026 The smzuggler contributors"
tools/privacy/allowlist.txt: needs exactly one "SPDX-License-Identifier: GPL-3.0-or-later"
tools/privacy/allowlist.txt: missing "Copyright (C) 2026 The smzuggler contributors"
```

The cumulative `git diff upstream-main -- tests/` already contains earlier
functional changes: fragment registration in tests/CMakeLists.txt and runtime
executable names in tests/integration/test_acceptance.c. This card preserves
those changes and adds only header comment lines; assertions and ASSERT count
are unchanged relative to the starting worktree. Requiring the cumulative diff
to contain only comments conflicts with preserving those earlier changes.
