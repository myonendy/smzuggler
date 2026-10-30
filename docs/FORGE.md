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
| `names` | the game's names and app ID appear nowhere in `src`, `tools`, `tests`, `CMakeLists.txt`, `third_party` |
| `build` | offline Release build with tests (`tools/forge/build.sh`) |
| `test` | ctest plus the Python tool tests, with game paths unset (`tools/forge/test.sh`) |

## Human checkpoints
Agents cannot change gates or `forge.toml`. A human does these when the card merges:
- after M0-T005: add `format` (`tools/format/check_format.py --base $FORGE_BASE`) and ES5 checks;
- after M0-T006/T007/T008/T014: add `doclint`, `docs_touched`, `mdlinks`, `headers`;
- after M0-T010: add `compat_surface`;
- after M0-T016: add `game_smoke` (dev-laptop only);
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
forge status          # cards, states, budget
forge doctor          # config and environment check
forge gates --base upstream-main   # run all gates on the current tree
```
Start a run only from a tmux session named `forge-smzuggler`.
