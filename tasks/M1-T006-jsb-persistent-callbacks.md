+++
id = "M1-T006"
title = "jsb persistent callbacks"
milestone = 1
deps = ["M1-T004"]
agent = "any"
owns = ["src/jsb/**", "tests/smz/test_jsb_callbacks.c", "tests/smz/jsb_callbacks.cmake"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `jsb_callback` (a persistent handle to a JS function, created with `jsb_callback_new`, called with `jsb_callback_call(cb, argc, argv)`, released with `jsb_callback_free`), with exceptions in the callback reported through the error handler and never crossing into C. Both backends.

## Acceptance criteria
- [ ] Tests in `tests/smz/test_jsb_callbacks.c`: call after the creating scope ended, call with arguments and return value, exception reported and the call returns an error code, free then GC releases the function (weak check), and calling a freed callback is a checked error.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] While M1 is in progress both engine modes build (`SMZ_JS_ENGINE=quickjs|v8`, from M1-T002) and the ctest suite passes in both; see DEC-SETUP-008 / SI-11 for the transitional QuickJS jsb backend.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1, §15.8.
