+++
id = "M1-T013"
title = "Replace /STACK and JS_STACK_SIZE with --stack-size"
milestone = 1
deps = ["M1-T007"]
agent = "any"
owns = ["CMakeLists.txt", "src/engine/js_engine_v8.cc", "src/main.c", "tests/smz/test_stack_size.c", "tests/smz/stack_size.cmake"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Remove the Windows `/STACK` link flag and `JS_STACK_SIZE` in v8 mode; pass V8 a `--stack-size` that matches the main thread's stack (read it at startup, e.g. `getrlimit(RLIMIT_STACK)` or the platform equivalent, minus a safety margin), and document the value. JS, SDL and the event loop stay on the process main thread.

## Acceptance criteria
- [ ] Test: deep recursion throws `RangeError: Maximum call stack size exceeded` instead of crashing, in v8 mode.
- [ ] No new threads: `runtime_main` still runs on the process main thread in every engine mode (owner ruling DEC-M1-T013-MAIN-THREAD); comments and docs saying "on the main thread" stay true.
- [ ] `git grep -n 'JS_STACK_SIZE\|/STACK' -- CMakeLists.txt src` shows hits only in quickjs-mode code.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1 item 6.
OWNER 2026-11-04 (DEC-M1-T013-MAIN-THREAD): do not move the runtime to another thread; earlier attempts did and failed review (breaks the thread model, SDL video must run on the main thread on some platforms). Size V8's `--stack-size` to the main thread's real stack instead.
