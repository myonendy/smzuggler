+++
id = "M1-T027"
title = "V8 becomes the only engine"
milestone = 1
deps = ["M1-T026", "M1-T008", "M1-T010", "M1-T012", "M1-T013"]
agent = "any"
owns = ["src/jsb/**", "src/engine/**", "CMakeLists.txt", "third_party/CMakeLists.txt", "ARCHITECTURE.md"]
docs = ["ARCHITECTURE.md", "docs/js-binding.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Remove QuickJS: drop `SMZ_JS_ENGINE`, `src/jsb/jsb_quickjs.c`, `src/engine/js_engine.c`, the qjs dependency in third_party/CMakeLists.txt and the `.jsc` path; v8 is the only build. Update ARCHITECTURE.md and docs.

## Acceptance criteria
- [ ] `git grep -n -i 'quickjs\|JS_NewRuntime' -- src CMakeLists.txt third_party` only shows history notes in docs.
- [ ] The full ctest suite passes on V8.
- [ ] ARCHITECTURE.md and docs/js-binding.md no longer describe the transitional backend.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.

## Notes
Spec §4.1, §9 M1. Human checkpoint H3: `tools/forge/build.sh` is switched to v8-only by a human when this merges. Also ends DEC-SETUP-003.
