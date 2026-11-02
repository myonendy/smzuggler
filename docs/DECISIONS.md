<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-10-30: document runtime naming and format policy.
Modified 2026-10-31: record the share-bundle last-run path.
Modified 2026-10-31: record process.argv shape and throwing require.
Modified 2026-11-01: record evaluator scratch-root isolation.
Modified 2026-11-01: record evaluator game-process XDG isolation.
Modified 2026-11-01: default host data directories when SMZUGGLER_NAME is set.
Modified 2026-11-01: record Map 92 report owner-run and JS-time gap definitions.
Modified 2026-11-01: propose the V8 pin (DEC-M1-T001).
Modified 2026-11-01: record jsb persistent callback handles.
Modified 2026-11-01: record the V8 script watchdog design.
Modified 2026-11-01: record how the game loop reaches JS without QuickJS.
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

## DEC-M0-T014b — Remaining section 12 notices outside M0-T014's owns

`.gitignore`, `tests/CMakeLists.txt` and `tests/integration/test_acceptance.c`
gain our copyright line and a dated Modified note naming the earlier smzuggler
change; upstream copyright and SPDX lines are untouched. `CONTRIBUTING.md`
already passes and is unchanged. No checker exemption changes.

`docs/upstream-CONTRIBUTING.md` (under `docs/**`, not protected) had packed
SPDX, copyright and note into one comment line, which the checker, reading one
item per line, rejected. Its comment now has one item per line, and
`tests/tools/test_pr_template.py::test_upstream_bytes` skips the whole leading
comment block (up to the first `-->` line), checks the SPDX and copyright lines
in it, and still compares every remaining byte with upstream `CONTRIBUTING.md`.
That test lies outside this card's `owns`; the reviewer required the edit.

`python3 tools/doclint/headers.py --base upstream-main` (2026-10-31) exits 1
with 262 problems in 131 files, all under forge.toml `protected` (which includes `LICENSES/**`): `.gitattributes`, `AGENTS.md`, `CLAUDE.md`,
`forge.toml`, `LICENSES/README.md`, `.claude/agents/investigator.md`,
`.claude/hooks/guard.sh`, `.codex/config.toml`, `docs/spec/{CHANGES,README,SPEC,
V8_MZ_HOST_SPEC_v0.2}.md`, `prompts/{auditor,planner,reviewer,worker}.md`,
`tools/privacy/allowlist.txt`, and the task cards under `tasks/` and
`tasks/draft/`. A human adds those headers and confirms the run exits 0.

The cumulative `git diff upstream-main -- tests/` also shows earlier functional
changes (fragment registration, DEC-SETUP-009; the `smzuggler-rt` executable
name, M0-T001). This card adds only comment lines to those files; assertions and
the ASSERT count are unchanged.

## DEC-M0-T023 — Share bundle last-run path and fail-closed binaries
`--share-bundle` has no run directory, so the packager treats the newest
directory under `$XDG_STATE_HOME/smzuggler-rt/runs/` (or `~/.local/state/...`
when that variable is unset) as the last run. `scan.py` skips NUL binaries;
the packager still searches them for deny-list terms and refuses to write the
zip on a hit, printing only the file and `chunk`, never the matched text.

## DEC-M0-T020 — Chrome/NW.js process.argv and throwing require
`process.argv` is the pair `["chrome", "index.html"]`: argv[0] is the
Chromium/NW.js binary name and argv[1] is the game-relative main HTML NW.js
reports when cwd is the game root. Host argv is not forwarded, because
smzuggler flags (`--game`, `--frames`) are not Chrome/NW.js shaped.
`require('nw.gui')` returns the `nw` object so existing `typeof` checks
keep working; any other unknown name throws `MODULE_NOT_FOUND` as Node does.
`process.exit(code)` sets `process.exitCode` and calls the same `_nwQuit`
path as `nw.App.quit()`; `src/main.c` returns that code after teardown.

## DEC-M0-T016 — Fail closed without filesystem containment

The read-only smoke driver requires usable bubblewrap instead of running an
unconfined fallback. A before/after game hash can detect game changes, but cannot
prove that no write landed elsewhere on the host. Mounting the host read-only
and scratch writable gives the smoke run that property without scanning private
host files. The synthetic tests distinguish orchestration coverage from native
namespace enforcement; unavailable namespaces produce a failed smoke run, never
an unprotected launch. The real GUI/battery run remains a separate requirement.
## DEC-M0-T010b-HEADER — compat_check.py skips only the exact header row
M0-T010b blocked after four attempts. `table_rows()` skipped every row whose first cell is
`name`, so the real `name` member (`window.name`) in dom_shim.js could never be matched, and
the card's `owns = []` made the policy gate reject the checker fix. M0-T010b now owns
`compat_check.py` and `tests/tools/test_compat_check.py`, may change only that header test to
skip the exact header and separator rows, and must add a new test proving `name` is matched,
with existing tests unedited (`tests_required = true`). No gate is relaxed and
`max_diff_lines` is unchanged.

## DEC-U9-FAIL-ASSERT — the policy gate counts upstream's FAIL( checks as assertions
U9 blocked twice on "task requires tests but no new assertions". Its new
`tests/test_data_dir.c` follows upstream's C test style, where each check is
`if (...) FAIL(...)`, and `assert_regex` did not match `FAIL(`. With the owner's approval
(2026-11-01), `FAIL` joins `ASSERT*`, `CHECK` and `assert` in `assert_regex`. This makes
the gate stricter for upstream-style tests too: removing an existing `FAIL(` check now
counts as removing an assertion. Nothing else in the gate changes.

## DEC-M0-T015-OWNER-RUN — M0-T015's real game run is an owner checkpoint
M0-T015 used all four attempts. Its branch had working scenarios, tooling and green gates,
but reviewers failed it for the real-run summary and bundle, which forge workers cannot
produce: they have no display and the test gate hides the game directory. With the owner's
approval (2026-11-01), the real run and its bundle move to a human checkpoint after merge
(docs/FORGE.md). In their place the card must add a test that builds a real
`make_bundle.py` bundle from a synthetic run, check `$gameMap.mapId() === 92` directly,
and prove the save and reload steps are not stubbed. The card may update ARCHITECTURE.md's
evaluator rows. No gate is relaxed.

## DEC-M0-T015 — isolate the evaluator's writable game root
The inherited host derives saves and logs from its game root. The local scenario entry
point therefore copies inputs beneath the run directory, excluding owner saves and logs.
Symlinks become independent copies so host writes cannot follow them back to the original
game. This costs one local game copy per run but adds no tool dependency and avoids
changing runtime modules outside the card's scope. The read-only smoke's bubblewrap mount
is not reused because save and reload need a writable save directory. The map step checks
`$gameMap.mapId() === 92` directly, as §10 names that map, instead of trusting the
configured start map. Control-mode perf samples keep native 0.1 ms precision and are
diagnostic; they cannot establish the later device acceptance gate. See
[local scenario instructions](testing.md#local-title-map-menu-and-save-scenarios).

## DEC-M0-T021 — Default host data directories when SMZUGGLER_NAME is set
U9/U10's `--data-dir` stays opt-in in `rmmz_resolve_log_dir` so the upstream
C test keeps writing logs to the game folder without the flag. When
`SMZUGGLER_NAME` is defined, `src/main.c` ignores that default and uses
`smz_host_dirs()` (`SMZUGGLER_BIN` under the XDG bases, or `SDL_GetPrefPath`
elsewhere). That helper always resolves the config directory as well as logs
and the `.jsc` cache, even though reading the config file is a later card.
Empty or relative `XDG_*` values are treated as unset and fall
back to `$HOME/.local/state` / `.cache` / `.config`; the resolver never
mkdir's and never returns a cwd-relative path or a path under the game root.
`--data-dir=PATH` still wins for logs when PATH is an absolute location
outside the game folder; a bare `--data-dir` uses the state dir. `--data-dir`
does not move cache or config.

## DEC-M0-T015-SAVE-ISOLATION — M0-T015 isolates the game's whole environment
M0-T015 blocked again after the owner-run change. The last review found that since U5 made
`process.env` a real snapshot, the evaluator started the game with the owner's environment,
so the game's save-location plugin would target the owner's real save folder (the write
guard refused it this time). The card now requires the game process to get scratch `HOME`
and `XDG_*` directories under the evaluator's output root, a save check that looks in that
scratch XDG tree, tests for both, and a clean boot-error path. Owner approved 2026-11-01.

## DEC-M0-T015b — cut storage stubs from the prelude instead of restoring them
Upstream's evaluator prelude stubs `StorageManager.saveObject`, `loadObject` and `exists`,
which turns the save and reload steps into no-ops. Rather than patching `prelude.js` to stub
and later restore them, `smzuggler.py` loads the upstream file without its storage section,
cut between upstream's own section headers, and fails loudly if those headers move.
The cut keeps `var storagePatched = false` so `ev.status()` does not read an undeclared
name. `prelude.js` stays identical to upstream, a test can show the loaded text has no stub, and
the inherited CLI's new `--prelude` option lets the NW.js reference run use the same text.
Bundle README step lines are appended by the entry point because the share packager's
README has no step section.

## DEC-M0-T018-OWNER-RUN — M0-T018 builds the report tool; the owner runs it
With the owner's approval (2026-11-01), DEC-M0-T015-OWNER-RUN extends to M0-T018. The card
now builds and tests `tools/perf/map92_report.py` on synthetic inputs and writes the run
recipe; it must not commit real-looking profile numbers. The owner does the dev-laptop
profiling run after merge and commits `docs/perf/m0-map92-profile.md` (docs/FORGE.md).
No gate is relaxed.

## DEC-M0-T018b — JS update time and the 15% GameLoopStats gap
`tools/perf/map92_report.py` needs one definition of "JS time" so the 15% flag
is testable and the owner's report is comparable to Gate G1:

- **JS update time** (displayed mean) is `timers_ms + jobs_ms + raf_ms` from
  the frame-time CSV. Those are the GameLoopStats phases that run JS (timer
  flush, microtasks, rAF). Events and swap stay out.
- **GameLoopStats JS time** for the cross-check is the same sum, weighted by
  the frame count on each `perf:` log line.
- **Attributed JS time** is the self share of the QuickJS interpreter, GC,
  shim JS and game JS buckets, times mean frame time.
- **Gap** is `|attributed − GameLoopStats| / GameLoopStats`, as a percent.
  More than 15% writes a FLAG in the report; the process still exits 0. p95
  frame time uses nearest-rank.

## DEC-U15-SAVE-READS — save reads go where save writes may go
M0-T022's run showed that with the U7 guard on, `existsSync` and the other read calls still
reject absolute paths outside the game root, so the save never lands in
`$XDG_DATA_HOME/<dir_save_name>/` and could not be read back. U15 lets those read calls take an
absolute path outside the root only when the guard is on and the write guard would accept the
same path. Guard off, relative paths and paths inside the root behave as before. Owner approved
(2026-11-01). It is an upstream-candidate change; nothing is sent upstream automatically.

## DEC-M0-T022-OWNER-RUN — the owner does the M0 exit run
With the owner's approval (2026-11-01), DEC-M0-T015-OWNER-RUN and DEC-M0-T018-OWNER-RUN extend
to M0-T022. Workers cannot run on battery with `power-saver` or with real graphics, so the card
writes the checklist and a step-by-step recipe, marks game-run items `pending owner run`, and
the owner does the dev-laptop run after merge and updates `docs/milestones/M0.md`. M0-T022 now
depends on U15. No gate is relaxed.

## DEC-M1-T001 — proposed V8 pin 13.6.233.8 (owner approval required)
Proposed answer to §11 Q9: V8 tag `13.6.233.8`, the branch of Chrome 136 and the Node.js 24
LTS line. A long-lived non-Chromium embedder builds it with the system C++ library, it has the
Fast API calls and code cache M1 needs, and a fixed tag makes `gclient sync` reproducible. The
tag was chosen offline; `tools/v8/build_v8.sh` checks it against `include/v8-version.h` and
stops on a mismatch. With it: `v8_enable_i18n_support=true` with a separate `icudtl.dat`
(v0.2's Q8 recommendation; data size revisited in M6), pointer compression on (matches
Chrome/NW.js, smaller heap) and the V8 sandbox off (it forbids the no-copy external
ArrayBuffers of `jsb`). Reasons per arg are in docs/build-v8.md. **This is a proposal: the
owner must approve the pin at checkpoint H1, and run the build once on dev-laptop, before
M1-T002 merges.**

## DEC-M1-T002 — SMZ_V8_DIR is checked against the headers, and v8 mode still runs QuickJS
`cmake/V8.cmake` defines `smz::v8` itself instead of including the install's
`v8-config.cmake`, and takes the version from `include/v8-version.h`, not from the manifest,
so a stale or hand-assembled directory cannot claim the pin. The pin
(`SMZ_V8_VERSION_PIN`) is duplicated from `tools/v8/build_v8.sh`;
`tests/tools/test_v8_cmake.py` fails if the two or the pointer-compression defines drift. The
check runs before `third_party` is configured, so a bad V8 fails in seconds. Until the
engine port lands, `SMZ_JS_ENGINE=v8` only adds `smz::v8` and the smoke test: the runtime
still links QuickJS, and no `SMZ_JS_ENGINE_*` define reaches the sources yet.

## DEC-M1-T003 — jsb getters are strict, and jsb.h has a small runtime API
The typed argument getters do not coerce: `jsb_arg_i32`/`_f64` need a Number, `_bool` a
Boolean, `_str` a String and `_obj` an Object, and a missing argument counts as undefined, so
it is a TypeError too. QuickJS's `JS_ToInt32` and friends coerce silently, which hides
shim bugs; a binding that wants WebIDL-style coercion converts in its JS shim, where the
reference behaviour is visible. Numbers given to `_i32` still wrap like ToInt32. A string
too long for the caller's buffer is a RangeError, never a silent truncation. Only the
first `jsb_throw_*` of a call counts. §4.1 lists no host-side calls, but bindings and their
C tests need a live engine, so `jsb.h` also has `jsb_runtime_create/_destroy/_setup/_eval`;
`js_engine.c` is expected to build on them when it moves to V8. `jsb_val`s are kept in a
per-call deque inside the stack-allocated `jsb_ctx`, which dies before its HandleScope,
so §15.8's "no Local in a struct" holds in spirit: nothing outlives the scope.

## DEC-M1-T005 — external buffers are not double-counted, and free callbacks may run off-thread
`jsb_new_external_buffer` does not call `AdjustAmountOfExternalAllocatedMemory`: V8
already counts every ArrayBuffer backing store, external ones included, toward its GC
heuristics, so reporting the bytes again would double them and run GC too often. The
explicit call, `jsb_adjust_external_memory`, is for native memory held behind other JS
objects (textures, decoders). jsb keeps its own per-runtime total and returns it, so both
backends give the same results and a negative total (a caller bug) is refused with -1.
The free function is V8's `BackingStore` deleter, which V8 may call on a GC helper thread;
rather than queue it to the main thread, jsb documents that it must only release memory.
`jsb_set_flags` was added because the tests need `gc()` (`--expose-gc`) and V8 takes flags
only before it starts; the QuickJS backend honours `--expose-gc` with a global `gc()` that
runs `JS_RunGC` and ignores V8-only flags. QuickJS has no C API for DataView, so its backend
calls the intrinsic `DataView.prototype` getters, saved before any user script runs.
QuickJS frees an external buffer's memory as soon as it is detached; V8 moves the memory to
the new buffer on `transfer()`. Either way the free function runs exactly once.

## DEC-M1-T006 — jsb callbacks are generation-checked value handles with C-built arguments
`jsb_callback` is a small struct (runtime, slot, generation) passed by value, not an
opaque pointer. The card asks that calling a freed callback be a checked error; a freed
pointer cannot be checked without keeping its memory forever, while a slot plus generation
can: freeing marks the slot free, reuse bumps its generation, and every copy of an old
handle is refused. Each live slot owns one `v8::Global` (one reference in QuickJS), so the
§15.8 one-owner rule holds and `jsb_runtime_destroy` releases what is left.
`jsb_callback_call` takes C values (`jsb_cval`: undefined, null, Boolean, Number, String)
and gives back a `jsb_result` with strings copied into a caller buffer, because the host
calls it with no native call around it, so no `jsb_ctx` or `jsb_val` exists to pass. The
result is a fourth parameter beyond the card's `(cb, argc, argv)` so that the card's
"return value" criterion can be met; it may be NULL. Callback exceptions go to a
per-runtime handler set with `jsb_set_error_handler` (default: stderr), since the engine's
`error_handler.c` is not yet on jsb; it will register there when ported (§4.1, item 5).

## DEC-M1-T008 — the watchdog thread requests an interrupt, which terminates on the main thread
§4.1 item 3 asks for `Isolate::TerminateExecution` from a timer thread. Called straight
from that thread it stops the script, but the card also wants the script's file and line,
and only the main thread, inside the running script, can read V8's current stack. So the
timer thread (`src/engine/js_watchdog.cc`) calls the thread-safe
`Isolate::RequestInterrupt`; the interrupt reads the top frame, reports it and calls
`TerminateExecution` on the main thread. Calling both from the timer thread was rejected:
V8 does not promise to service an API interrupt before a pending termination, so the
interrupt could stay queued and run during the *next* script. Each arm bumps a generation, and the interrupt
drops a request whose generation is no longer current, so a script that finished in time
is never hit by a late request. The watchdog keeps QuickJS mode's rule that the clock runs
from the last arm (no disarm). The `--js-timeout-ms` flag and the control-mode default
named by the card need `src/main.c`, which the card does not own; `--watchdog <ms>`
reaches the same code meanwhile.

## DEC-M1-T009 — the game loop calls JS through js_engine.h and one JS helper
`game_loop.c` cannot call jsb directly yet: in quickjs mode `js_engine_get_jsb()` is NULL
(the transitional backend cannot adopt the engine's context), and neither the `outsider`
executable nor `test_performance` links `smz_jsb`; both are set in CMake files the card
does not own. So the loop drops QuickJS for the engine-neutral `js_engine.h`, which in v8
mode is itself built on jsb: it installs a JS helper, `__smz_gameLoop(op, ...)`, and
evaluates one short call to it per JS call. The helper keeps the snapshotted `__dom_*`
functions, catches exceptions (dispatching them to `window.onerror` like
`error_handler_report_js_exception`) and queues `__native_control_send` lines, so C holds
no engine handle at all. Phase order, the two job drains and the rAF timestamp are
unchanged. Costs: five tiny compiles per frame on QuickJS, one (the rAF call) on V8, and
in v8 mode the watchdog re-arms per eval. Once the runtime links jsb in both modes, the
calls can move to `jsb_callback_call` on the same helper without changing its contract.
