+++
id = "M1-T002"
title = "CMake: import prebuilt V8 and add SMZ_JS_ENGINE"
milestone = 1
deps = ["M1-T001"]
agent = "any"
owns = ["cmake/V8.cmake", "CMakeLists.txt", "tests/smz/test_v8_smoke.cc", "tests/smz/v8_smoke.cmake"]
docs = ["docs/build-v8.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `cmake/V8.cmake` importing V8 from `SMZ_V8_DIR` (an imported static target `smz::v8` with headers, defines and link flags) and a `SMZ_JS_ENGINE` cache option (`quickjs` default, `v8`). Configuring with `v8` and a missing or wrong-version `SMZ_V8_DIR` fails at configure time with a clear message. Add `tests/smz/test_v8_smoke.cc` (via `tests/smz/v8_smoke.cmake`, built only in v8 mode): create an isolate and context, run `1+1`, check the V8 version equals the pin.

## Acceptance criteria
- [ ] Configure+build in quickjs mode is unchanged (all existing tests pass).
- [ ] Configure in v8 mode without `SMZ_V8_DIR` fails with the message naming the variable.
- [ ] In v8 mode the smoke test passes and reports the pinned version.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.

## Notes
Spec §4.1. Human checkpoint H2: after merge, `tools/forge/build.sh` and `test.sh` are extended by a human to also build and test the v8 mode (DEC-SETUP-006).
