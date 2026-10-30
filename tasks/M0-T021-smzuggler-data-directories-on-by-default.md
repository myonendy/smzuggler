+++
id = "M0-T021"
title = "smzuggler data directories on by default"
milestone = 0
deps = ["U9", "U10", "M0-T001"]
agent = "any"
owns = ["src/smzuggler/data_dirs.c", "src/smzuggler/data_dirs.h", "src/main.c", "src/engine/js_engine.c", "CMakeLists.txt", "tests/smz/test_data_dirs.c", "tests/smz/data_dirs.cmake"]
docs = ["docs/data-dirs.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Make smzuggler always use its own data directories (§5.3, §5.6, §13): logs in `$XDG_STATE_HOME/smzuggler-rt/` (SDL_GetPrefPath equivalent on Windows/macOS), the `.jsc` cache in `$XDG_CACHE_HOME/smzuggler-rt/`, the config at `$XDG_CONFIG_HOME/smzuggler-rt/`. Builds on U9/U10's opt-in `--data-dir` by making it the default when `SMZUGGLER_NAME` is set; upstream behaviour stays the default in upstream builds. Nothing is ever written in the game dir.

## Acceptance criteria
- [ ] New test `tests/smz/test_data_dirs.c` (via `tests/smz/data_dirs.cmake`) sets scratch XDG variables and asserts the resolved paths for logs, cache and config, the fallbacks when XDG vars are unset (`~/.local/state` etc.), and that a game-root-relative path is never returned.
- [ ] With an unset or empty `XDG_*`, nothing is created relative to the working directory.
- [ ] `docs/data-dirs.md` documents each directory, its contents and how to clear it.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.

## Notes
Spec §5.3, §5.6, §13.
