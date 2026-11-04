# AGENTS.md — rules for every coding agent (Codex, Claude, OpenCode vendors, reviewers)

This file is canonical. `CLAUDE.md` imports it. Humans edit it; agents never do.

smzuggler is a fork of Outsider (an RPG Maker MZ runtime in C with JavaScript shims),
being moved from QuickJS to V8 per `docs/spec/SPEC.md` (v0.5.1). Outsider's history,
GPL-3.0 licence and headers are kept; new work is GPL-3.0-or-later.

## 0. How you are being run
- You are one of several agents working **in parallel**, each in its own git worktree,
  on exactly one task card. The orchestrator commits, merges, and runs the gates. **Do not
  `git commit`, `git push`, switch branches, or rewrite history.** Never add a remote.
- After you finish, objective gates run (policy, privacy, commit subjects, game names,
  build, tests). Every gate runs and all failures come back together in one report (a
  failed build skips the rest). Then a reviewer from a *different vendor* reads your diff
  and your final report against the card. Failures come back to you verbatim.
- A retry after a gate failure usually continues your own earlier session: the card and
  what you read are still there, so fix what the report names instead of re-exploring.
- You have a hard budget (wall-clock time, attempts). Spend it on the task.

## 1. Scope
<!-- forge:policy-lists BEGIN (generated from forge.toml [policy] by `forge sync-agents`; do not edit by hand) -->
- Touch only files matching the card's `owns` globs, plus the always-allowed paths
  `docs/**`, `CHANGELOG.md`, and **new** test files under
  `tests/**`. Anything else fails the SCOPE gate.
- Never modify these protected paths: `AGENTS.md`, `CLAUDE.md`, `forge.toml`, `schemas/**`, `prompts/**`, `.claude/**`, `.codex/**`, `docs/spec/**`, `tasks/**`, `tools/privacy/**`, `tools/commitmsg/**`, `tools/forge/**`, `.forge-samples`, `LICENSE`, `LICENSES/**`, `.gitattributes`.
  If one of them is wrong, say so in your final message under `SPEC/INFRA ISSUES`.
<!-- forge:policy-lists END -->
- If the card is ambiguous, choose the interpretation most consistent with
  `docs/spec/SPEC.md` and `ARCHITECTURE.md`, state it in your final message, and continue.
- If the spec contradicts itself, report it; do not silently pick a side.
- `§N.M` always means a section of `docs/spec/SPEC.md` (v0.5.1). The older v0.2 spec in
  `docs/spec/` is background only.

## 2. Workflow (test-first)
1. Read the card and its **context pack** (in your prompt): it already holds the spec
   sections the card cites, the decisions it names, the parts of `ARCHITECTURE.md` about
   the modules it owns, and the gate commands. Then read only the code you will touch and
   what it directly calls.
   **Never open these files whole**; grep them or read a line range for anything the pack
   lacks. Every line you read is re-sent on every later turn of your run:
   - `docs/compat-surface.md` (2,000+ table rows): grep for the rows of the members you
     touch, e.g. `grep -n 'nw.Window' docs/compat-surface.md`, and edit only those rows;
   - `docs/spec/SPEC.md` and `docs/spec/V8_MZ_HOST_SPEC_v0.2.md`;
   - `docs/DECISIONS.md`, `CHANGELOG.md`, `docs/js-binding.md`, `docs/UPSTREAM.md`.
   `ARCHITECTURE.md` is fine to grep; read whole sections only when the excerpt points there.
2. Write failing tests that encode the acceptance criteria.
3. Implement until they pass. Check with **`forge check`**: it runs every gate quietly in
   your worktree, including uncommitted work, and never commits. Don't also run the gate
   scripts one by one. If `forge` is not on PATH, run `tools/forge/build.sh` and
   `tools/forge/test.sh`. Builds are offline: dependencies come from the local cache;
   never download anything and never change a dependency pin unless the card says so.
4. Update the docs the card lists (section 6). Finish with the short report (section 9).

## 3. No cheating. These are detected automatically and fail the task.
- Deleting, weakening, skipping, or disabling tests (`SKIP`, `DISABLED_`, `#if 0`, removing
  asserts), or lowering a threshold, a timeout, or a frame/perf budget to make a test pass.
- Editing existing tests to match broken behaviour (unless the card says `may_edit_tests = true`).
- Hardcoding expected outputs, special-casing test inputs, or detecting test mode in
  production code.
- Stub or hollow implementations: `TODO`, `FIXME`, `UNIMPLEMENTED`, empty bodies,
  "placeholder" logic presented as done. If something can't be done in this task, leave
  it out and say so in your report.
- Tests that assert nothing meaningful.
- Suppressing warnings, sanitizers, or lints instead of fixing the cause.
A reviewer that finds any of these rejects the whole change.

## 4. Privacy and game material (spec §16, §13). Hard rules.
- No personal names, usernames, hostnames, home-directory paths (`/home/...`,
  `/Users/...`, `C:\Users\...`), personal emails or shell prompts in any file, test,
  comment, doc, log, screenshot or message. Write `~` for a home directory and
  `dev-laptop` for the developer's machine. The privacy gate fails on any of these.
- Never commit game files, game code, assets, saves (`*.rmmzsave`, `*.rpgsave`),
  screenshots of the game, or anything from a game's Unity or NW.js wrapper. Tests reach
  the game only through `SMZUGGLER_GAME_DIR` / `RMMZ_TEST_GAME_DIR` and save fixtures only
  through `$XDG_DATA_HOME/smzuggler-rt/fixtures/`; they skip loudly when unset only if the
  card says so.
- The game's own names and IDs appear in code only in `src/smzuggler/game_profile.c`.
- Never copy, paraphrase or port code from decompiled or proprietary programs.
- Never read or print the privacy deny-list.

## 5. Code standards (spec §15.6)
- Functions ≤ 60 lines; small cohesive files. No clever macros or metaprogramming.
- Respect the module map in `ARCHITECTURE.md`. C11 for C, C++ only where V8 needs it.
- New files carry the GPL header from spec §12 (`SPDX-License-Identifier:
  GPL-3.0-or-later` and `Copyright (C) 2026 The smzuggler contributors`). In an upstream
  file you change, keep its existing lines and add the smzuggler copyright line and a
  dated "modified" note.
- U cards (upstream offers U1–U14) are the exception: ES5 only in shims, upstream's style,
  upstream's header with **no** smzuggler line, only the files of the card's §8.1 row plus
  tests added to upstream's existing test files (docs/DECISIONS.md DEC-SETUP-005).
- Tests: new C/C++ tests go in `tests/smz/test_<name>.c` with a `tests/smz/<name>.cmake`
  fragment (included by glob; never edit `tests/CMakeLists.txt` unless the card says so).
  Python tool tests go in `tests/tools/test_<name>.py` and reach the tool by subprocess or
  by putting the repo root on `sys.path`. No absolute home paths in tests: use temp dirs.
- No new dependencies unless the card allows them. Errors fail loudly with context.

## 6. Documentation (gated, spec §15)
- Every public function/type has a doc comment explaining **why** and any contract
  (units, ownership, ranges, thread). Do not restate *what* the code says.
- Each card lists its docs in `docs`; they must change in the same diff (DOCS gate), with
  two exceptions below: `CHANGELOG.md` and `docs/DECISIONS.md`.
- Module boundary changes update `ARCHITECTURE.md` in the same change: edit the one
  section involved, found with grep, not the whole file.
- **Decisions:** only for choices a later agent could not recover from the code: a spec
  rule, a file/save/protocol format, a pin, a deliberate deviation from the spec or from
  upstream. Write a new file `docs/decisions/DEC-<card ID>.md` that starts with the spec §12
  header (as for any new file) and `## DEC-<card ID> — <title>`, 10-20 lines: context,
  decision, why, rejected alternative. Never append to `docs/DECISIONS.md` (older entries
  stay there and keep their IDs). Most cards need no decision.
- **Changelog:** don't edit `CHANGELOG.md`. Put one line in your final report under
  `CHANGELOG:` saying what now exists (same style as the existing entries, without the
  card ID); forge adds it under "Unreleased" with the card ID.
  When the card's `docs` list `CHANGELOG.md`, that line is required: never write `none`.

## 7. Token discipline
- Don't re-read files, don't dump large files or logs (use grep and ranges; section 2 lists
  the big docs), don't explore unrelated code, don't refactor beyond the card.
- Build output is long: on a failure read the first error, not the whole log.
- When a command fails, read the error and fix the cause. Don't retry blindly.
- Reuse before you write: grep for an existing helper (parsers, path helpers, test
  fixtures) and call it. Don't copy one into your files; if two cards need it, say so under
  `SPEC/INFRA ISSUES`.
- Run `forge check` (and builds and tests) in the foreground and wait for it to return.
  Never run it in the background and poll, and never list processes (`ps`, `pgrep`, `top`):
  their output is huge and is re-sent on every later turn. Read only the tail or the first
  error of a log, never whole test logs.
- Stop when the acceptance criteria are met and `forge check` passes.

## 8. Commit messages (CONTRIBUTING.md, "Commit messages")
You do not commit (forge does), but every card ends with a commit subject you propose:
- imperative and specific, 72 characters or fewer, saying what changed and where:
  `Add removeAllListeners and once to nw.Window shim`;
- never a bare card ID, `wip`, `fix`, `update`, `changes`, `misc`, and never `forge:`;
- the card ID goes in a trailer (`Card: M0-T003`), not in the subject;
- add a body line saying why when it is not obvious.
Check it with `python3 tools/commitmsg/check_subject.py --msg-file <file>` (in a temp file).
forge uses your `COMMIT SUBJECT:` line as the subject of the commit it makes and adds the
`Card:` and `Forge-Attempt:` trailers itself (docs/FORGE.md, "Commit subjects"). An invalid
subject is replaced by one made from the card title, so write a good one.

## 9. Final message format (keep it under 25 lines)
```
DONE: <one line>
COMMIT SUBJECT: <subject per section 8>
CHANGED: <files>
TESTS: <what the new tests prove>
CHANGELOG: <one line for CHANGELOG.md "Unreleased", or none>
INTERPRETATIONS: <assumptions, or none>
SPEC/INFRA ISSUES: <or none>
LEFT OUT: <anything not done, incl. dev-laptop runs a human must do, or none>
```
forge copies `LEFT OUT` and `SPEC/INFRA ISSUES` into `docs/todo/<card ID>.md` for the owner.
