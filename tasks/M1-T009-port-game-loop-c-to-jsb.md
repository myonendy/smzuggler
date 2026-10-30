+++
id = "M1-T009"
title = "Port game_loop.c to jsb"
milestone = 1
deps = ["M1-T006", "M1-T007"]
agent = "any"
owns = ["src/engine/game_loop.c", "src/engine/game_loop.h"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Rewrite the JS-facing parts of `src/engine/game_loop.c` (timers, rAF, phase calls into JS) on jsb, keeping its phases and timing unchanged.

## Acceptance criteria
- [ ] tests/test_game_loop.c passes in both modes.
- [ ] `git grep -n 'JS_' -- src/engine/game_loop.c` is empty.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] While M1 is in progress both engine modes build (`SMZ_JS_ENGINE=quickjs|v8`, from M1-T002) and the ctest suite passes in both; see DEC-SETUP-008 / SI-11 for the transitional QuickJS jsb backend.

## Notes
Spec §4.1.
