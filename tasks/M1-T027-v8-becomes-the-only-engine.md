+++
id = "M1-T027"
title = "V8 becomes the only engine"
milestone = 1
deps = ["M1-T026", "M1-T026b", "M1-T026c", "M1-T008", "M1-T010", "M1-T012", "M1-T013"]
agent = "or-deepseek"
owns = ["src/jsb/**", "src/engine/**", "CMakeLists.txt", "third_party/CMakeLists.txt", "ARCHITECTURE.md", "tests/CMakeLists.txt", "tests/smz/**", "tests/test_storage.c", "tests/test_performance.c", "src/smzuggler/dump_surface.c", "src/smzuggler/dump_surface.h", "src/platform/**"]
docs = ["ARCHITECTURE.md", "docs/js-binding.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = true  # OWNER 2026-11-05: only to remove QuickJS-only tests/branches and swap QuickJS-only expectations in test_storage.c and test_performance.c, per the criteria below
+++
## Goal
Remove QuickJS: drop `SMZ_JS_ENGINE`, `src/jsb/jsb_quickjs.c`, `src/engine/js_engine.c`, the qjs dependency in third_party/CMakeLists.txt and the `.jsc` path; v8 is the only build. Update ARCHITECTURE.md and docs.

## Acceptance criteria
- [ ] `git grep -n -i 'quickjs\|JS_NewRuntime' -- src CMakeLists.txt third_party` only shows history notes in docs.
- [ ] The full ctest suite passes on V8.
- [ ] `storage` and `performance` pass on V8: in `tests/test_storage.c` and `tests/test_performance.c` only, a `.jsc` expectation becomes the equivalent check on the V8 code cache that is actually written, and a QuickJS-specific microtask ordering expectation becomes V8's spec-correct order, each with equal strength and a one-line comment naming the QuickJS behaviour it replaced. No other assertion in those files changes.
- [ ] `dump_surface` and `dump_surface_members` pass unchanged: fix `src/smzuggler/dump_surface.c` so it lists accessor-backed members (such as those on `process`) on V8. Their tests are not edited.
- [ ] `platform` passes unchanged under `tools/forge/test.sh` (it passes on main). Find what this card's changes broke (for example SDL or video-driver setup in the test environment) and fix it in code or CMake wiring; `tests/test_platform.c` is not edited.
- [ ] Test edits only remove QuickJS: (a) delete test files and `tests/smz/*.cmake` fragments whose only subject is QuickJS code this card removes (for example `test_jsb_adopt.c` with `jsb_adopt.cmake`, and the QuickJS-backend or transitional tests), with their CMake registrations; (b) in tests that also cover V8 or shared code, delete only the QuickJS-only branches (`#if`/`#else` arms or skips for the QuickJS engine) and the `.jsc` assertions, leaving every V8 and engine-neutral assertion unchanged in substance. No assertion is loosened, rewritten or skipped, and no test that covers V8 or shared code is deleted. The final message lists each deleted test file and each edited test file with one line on what QuickJS-only content was removed. Reviewers fail any other test edit.
- [ ] ARCHITECTURE.md and docs/js-binding.md no longer describe the transitional backend.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Owner 2026-11-05: the first run blocked because `tests/smz/test_jsb_adopt.c` includes the removed `jsb_adopt.h` and tests could not be edited; removing QuickJS-only tests is now in scope (criterion above).
Owner 2026-11-05 (2nd): attempt 1 left storage, performance, dump_surface, dump_surface_members and platform failing; these tests had been built with QuickJS until now. They are in scope per the criteria above.
Spec §4.1, §9 M1. Human checkpoint H3: `tools/forge/build.sh` is switched to v8-only by a human when this merges. Also ends DEC-SETUP-003.
