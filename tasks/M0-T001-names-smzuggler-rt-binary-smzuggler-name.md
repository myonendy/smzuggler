+++
id = "M0-T001"
title = "Names: smzuggler-rt binary, SMZUGGLER_NAME constant, README not-affiliated paragraph"
milestone = 0
deps = []
agent = "any"
owns = ["CMakeLists.txt", "src/smzuggler/names.h", "src/platform/platform.h", "src/main.c", "src/bindings/bind_platform.c", "README.md", "tests/smz/test_names.c", "tests/smz/names.cmake", "tests/integration/test_acceptance.c"]
docs = ["README.md", "CHANGELOG.md", "docs/DECISIONS.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Introduce the §13 names as build constants without changing behaviour. Add `src/smzuggler/names.h` with `SMZUGGLER_NAME` ("smzuggler") and `SMZUGGLER_BIN` ("smzuggler-rt"), passed from CMake (`SMZUGGLER_NAME`/`SMZUGGLER_BIN` cache variables) so the name can change without touching code. The executable's `OUTPUT_NAME` becomes `smzuggler-rt` (the CMake target may keep its upstream name to ease syncs). `RMMZ_APP_NAME`, the window title fallback (bind_platform.c) and the startup log line in main.c use the constant. README.md's first paragraph says what smzuggler is, that it is based on Outsider by General Arcade, and the §13 not-affiliated statement; the README names the game only in a plain compatibility statement.

## Acceptance criteria
- [ ] `cmake --build` produces `smzuggler-rt` (and no `outsider` executable); the install rules install it under that name.
- [ ] A new C test `tests/smz/test_names.c` (registered by `tests/smz/names.cmake`) checks `SMZUGGLER_NAME`, `SMZUGGLER_BIN` and that `RMMZ_APP_NAME` equals `SMZUGGLER_NAME`.
- [ ] `git grep -n 'Outsider' -- src` shows no user-visible string naming Outsider as this program (comments crediting upstream are fine).
- [ ] README.md's first paragraph contains the not-affiliated statement of §13 and the line "based on Outsider by General Arcade"; the rest of the README no longer says saves or logs go next to the game.
- [ ] The `names` gate (game names in src/tools/tests) passes.
- [ ] Upstream's `build_pipeline_structure` check in `tests/integration/test_acceptance.c` is changed only to look for `smzuggler-rt` instead of `outsider` (same paths, same number of ASSERTs); no other line of that file changes, and the acceptance test passes from a `build/` build dir.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §13. Files changed from upstream get the §12 "modified" header line. Keep upstream-facing code paths (RMMZ_* macros) so upstream syncs stay mechanical; the constant feeds them. Record DEC-M0-T001 (target name kept vs. renamed).
