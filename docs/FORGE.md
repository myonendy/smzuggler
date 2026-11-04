<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Contributor reference; follow the architecture and policy in ARCHITECTURE.md.
-->

# Working with AgentForge

smzuggler's work is split into small cards that AgentForge runs (`forge.toml` at the root).

## Layout
- `forge.toml`: agents, budget, gates and policy. Protected: agents never edit it.
- `tasks/*.md`: active cards (TOML front matter between `+++` lines, then Goal,
  Acceptance criteria, Notes). `tasks/draft/*.md`: first-pass cards for M2–M7, D and U13,
  which forge does not schedule. Move a draft into `tasks/` once it has been detailed.
- `AGENTS.md` / `CLAUDE.md`: rules every agent reads. `prompts/`, `schemas/`: forge's role
  prompts and output schemas.
- `docs/spec/SPEC.md`: the spec (v0.5.1, read-only). `docs/DECISIONS.md`: decisions.
  `docs/SPEC_ISSUES.md`: open questions for the owner.
- `tools/forge/*.sh`: the gate scripts. `tools/privacy/`: the §16 scan and hooks.

## Gates (in order)
| Gate | What it checks |
|---|---|
| `policy` | forge's built-in checks: protected files, test tampering, scope (`owns`), hollow code, test presence, file size, docs touched |
| `privacy` | `tools/privacy/scan.py` on the whole tree, plus identity, messages and added lines of every commit since the card's base |
| `commit_subjects` | every new non-merge commit's subject follows CONTRIBUTING.md (`tools/commitmsg/check_subject.py`) |
| `names` | the game's names and app ID appear nowhere in `src`, `tools`, `tests`, `CMakeLists.txt`, `third_party` |
| `build` | offline Release build with tests (`tools/forge/build.sh`) |
| `test` | ctest plus the Python tool tests, with game paths unset (`tools/forge/test.sh`) |

## Commit subjects
forge runs with `[project] commit_style = "descriptive"`. Each attempt commit's subject is
the worker's `COMMIT SUBJECT:` report line (AGENTS.md §8, §9) when it is one line of at most
72 characters without a card-ID or `forge:` prefix; otherwise forge makes one from the card
title ("Add ..." unless the title starts with a verb). The card and attempt go in trailers:

```
Add removeAllListeners and once to nw.Window shim

Card: M0-T003
Forge-Attempt: 2 by codex
```

Other forge commits: partial work after a usage limit (`Forge-Partial:` trailer), work
picked up by `regate`/resume (`Forge-Pickup:`), formatter fixes (`Apply the project
formatter`, `Forge-Fix:`), and syncs with main (`Merge main into the <card> task branch`,
skipped by the gate as a merge). `regate` reads the vendor from the `Forge-Attempt:` trailer.
Main is still fast-forwarded to the task branch, so a card lands as its attempt commits.

The `commit_subjects` gate still counts the old machine forms (`<ID>: attempt N by
<vendor>` and siblings) instead of failing them, because task branches started before
this change carry them. Once those branches have merged, switch the gate to `--strict` in
`tools/forge/commit_subjects.sh` so a machine subject fails like any other bad subject.

## Token use
Agents get each card's spec sections, named decisions, the `ARCHITECTURE.md` parts for the
modules it owns (`[context] architecture_always` sections whole) and the gate commands in
their prompt, and AGENTS.md §2 forbids opening the big docs whole. forge writes the
`CHANGELOG.md` line from the report's `CHANGELOG:` field; new decisions are small files in
`docs/decisions/`; human to-dos from reports land in `docs/todo/<ID>.md`. A retry after a
gate failure stays with the same vendor and, for Claude, resumes its session. `forge usage`
shows token use per role, vendor and card from `.forge/logs`.

## Human checkpoints
Agents cannot change gates or `forge.toml`. A human does these when the card merges:
- after M0-T005: add `format` (`tools/format/check_format.py --base $FORGE_BASE`) and ES5 checks;
- after M0-T006/T007/T008/T014: add `doclint`, `docs_touched`, `mdlinks`, `headers`;
- after M0-T010: add `compat_surface`;
- after M0-T016: add `game_smoke` (dev-laptop only);
- after M0-T015: run the evaluator scenarios once on dev-laptop (battery, `power-saver`, test copy)
  and make a share bundle with `tools/share/make_bundle.py`; check the per-step summary;
- after M0-T018: do the Map 92 profiling run from `docs/perf/m0-map92-profile-recipe.md`,
  commit the generated `docs/perf/m0-map92-profile.md` and make a share bundle;
- after M0-T022: do the M0 exit run from `docs/milestones/m0-exit-recipe.md` (battery,
  `power-saver`, test copy) and change its `pending owner run` lines in `docs/milestones/M0.md`;
- H1 after M1-T001: approve the V8 pin and build V8 once;
- H2 after M1-T002: build and test both engine modes in `build.sh`/`test.sh`;
- H3 after M1-T027: switch `build.sh` to V8 only.
Cards that need the game window or a 60 fps measurement run on dev-laptop on battery with
`power-saver` and stop with `BLOCKED` when they cannot.

## Dependency cache
The `build` gate never downloads. It expects one source tree per dependency in
`$SMZ_DEPS_DIR` (default `~/.cache/smzuggler-dev/deps`): `sdl3-src`, `quickjs-src`,
`soloud-src`, `glslang-src`, `spirv_cross-src`, `sdl_shadercross-src`, each checked out at
the tag pinned in `CMakeLists.txt`. Fill it once by configuring a normal online build and
copying `build/_deps/<name>-src`. Each worktree builds in its own `build/`.

## Commands
```
forge status          # cards, states, budget, human to-dos in docs/todo/
forge usage           # token use per role, vendor and card
forge check           # (inside a worktree) every gate on the uncommitted work
forge doctor          # config and environment check
forge gates --base upstream-main   # run all gates on the current tree
```
Start a run only from a tmux session named `forge-smzuggler`.
