+++
id = "M1-T008"
title = "Script watchdog with TerminateExecution"
milestone = 1
deps = ["M1-T007"]
agent = "any"
owns = ["src/engine/js_engine_v8.cc", "src/engine/js_watchdog.cc", "src/engine/js_watchdog.h", "CMakeLists.txt", "tests/smz/test_js_watchdog.c", "tests/smz/js_watchdog.cmake"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
A watchdog thread terminates JS that runs longer than a configurable limit (`--js-timeout-ms`, default off in release, on in control mode) with `Isolate::TerminateExecution`, reports the script location, and lets the frame loop continue or exit cleanly.

## Acceptance criteria
- [ ] Test: `while(true){}` with a 200 ms limit is terminated within 400 ms, the error names the file and line, and the next eval succeeds after `CancelTerminateExecution`.
- [ ] No watchdog thread exists when the limit is off (checked).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.

## Notes
Spec §4.1.
