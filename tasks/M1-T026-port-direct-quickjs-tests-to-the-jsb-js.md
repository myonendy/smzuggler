+++
id = "M1-T026"
title = "Port direct-QuickJS tests to the jsb/js_engine API"
milestone = 1
deps = ["M1-T014", "M1-T015", "M1-T016", "M1-T017", "M1-T018", "M1-T019", "M1-T020", "M1-T021", "M1-T022", "M1-T023", "M1-T024", "M1-T025", "M1-T009", "M1-T011"]
agent = "any"
owns = ["tests/**"]
docs = ["docs/testing.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Rewrite the tests that call QuickJS directly (error_handler, acceptance, file_io, plugins, gameplay, title_screen and any others found by `git grep -l 'JS_\|quickjs' -- tests`) to use js_engine.h/jsb.h only, without dropping any case.

## Acceptance criteria
- [ ] `git grep -l 'JS_[A-Z][a-z]\|quickjs' -- tests` is empty.
- [ ] Total assertion count in tests/ does not go down (policy TAMPER gate) and every renamed case is listed old → new in the card report.
- [ ] All tests pass in both modes.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1. `may_edit_tests` is set because these are existing tests; changes are API substitutions only. A reviewer checks each assertion still tests the same thing.
