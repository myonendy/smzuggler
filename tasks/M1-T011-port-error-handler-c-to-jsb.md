+++
id = "M1-T011"
title = "Port error_handler.c to jsb"
milestone = 1
deps = ["M1-T007"]
agent = "any"
owns = ["src/engine/error_handler.c", "src/engine/error_handler.h"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Rewrite `src/engine/error_handler.c`'s JS parts (exception formatting, `window.onerror` dispatch, stack capture) on jsb; the log scrub from M0-T004 stays in the path.

## Acceptance criteria
- [ ] The error_handler tests that use only the public API pass in both modes; the direct-QuickJS ones are ported in M1-T027.
- [ ] `git grep -n 'JS_' -- src/engine/error_handler.c` is empty.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] While M1 is in progress both engine modes build (`SMZ_JS_ENGINE=quickjs|v8`, from M1-T002) and the ctest suite passes in both; see DEC-SETUP-008 / SI-11 for the transitional QuickJS jsb backend.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1.
