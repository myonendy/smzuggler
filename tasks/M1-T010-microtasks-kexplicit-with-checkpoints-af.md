+++
id = "M1-T010"
title = "Microtasks: kExplicit with checkpoints after each macrotask"
milestone = 1
deps = ["M1-T009"]
agent = "any"
owns = ["src/engine/js_engine_v8.cc", "src/engine/game_loop.c", "tests/smz/test_microtasks.c", "tests/smz/microtasks.cmake"]
docs = ["docs/js-binding.md", "docs/DECISIONS.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Set the V8 microtask policy to `kExplicit` and run a checkpoint after every macrotask the loop runs (each timer callback, each rAF callback batch, each native event dispatch), matching Chrome's ordering; `process.nextTick` jobs (U5) run before promise jobs.

## Acceptance criteria
- [ ] A JS ordering test (in `tests/smz/test_microtasks.c`) logs the order of setTimeout, Promise.then, nextTick and rAF callbacks and asserts the Chrome/NW.js order, in v8 mode (and in quickjs mode where its existing job loop matches; differences listed in the DEC).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.

## Notes
Spec §4.1. Record DEC-M1-T010 with the exact order.
