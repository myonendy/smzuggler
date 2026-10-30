+++
id = "M1-T007"
title = "js_engine on V8 behind the existing js_engine.h"
milestone = 1
deps = ["M1-T003", "M1-T004"]
agent = "any"
owns = ["src/engine/js_engine_v8.cc", "src/engine/js_engine.h", "src/engine/js_engine.c", "CMakeLists.txt", "tests/smz/test_js_engine_v8.c", "tests/smz/js_engine_v8.cmake"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `src/engine/js_engine_v8.cc` implementing js_engine.h on V8 (built in v8 mode instead of js_engine.c): platform and isolate setup, one context, eval of strings and files with `ScriptOrigin` (game-relative file name, line/column) so stack traces match NW.js, console wiring, error reporting with stack, and `js_engine_get_jsb()` returning the jsb context. Any js_engine.h function that exposes QuickJS types is replaced by a jsb-typed equivalent, and callers in quickjs mode use it too.

## Acceptance criteria
- [ ] Existing tests/test_js_engine.c cases that use only js_engine.h pass in v8 mode; a list of the cases that need M1-T027 (direct QuickJS use) is in the card report.
- [ ] A new test checks that an exception thrown in `foo.js` line 3 reports `foo.js:3` in the stack in both modes.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] While M1 is in progress both engine modes build (`SMZ_JS_ENGINE=quickjs|v8`, from M1-T002) and the ctest suite passes in both; see DEC-SETUP-008 / SI-11 for the transitional QuickJS jsb backend.

## Notes
Spec §4.1.
