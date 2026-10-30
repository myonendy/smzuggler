+++
id = "M1-T004"
title = "Transitional QuickJS backend for jsb"
milestone = 1
deps = ["M1-T003"]
agent = "any"
owns = ["src/jsb/jsb_quickjs.c", "CMakeLists.txt"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `src/jsb/jsb_quickjs.c` implementing jsb.h on QuickJS, so bindings can be ported one at a time while both engine modes stay green. It is marked transitional in its header and removed by M1-T027.

## Acceptance criteria
- [ ] `tests/smz/test_jsb_core.c` runs unchanged in quickjs mode and passes.
- [ ] The file header and docs/js-binding.md state the removal card (M1-T027).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.

## Notes
Spec §4.1 says jsb has one implementation (V8). This card is a proposed deviation for the transition only (docs/SPEC_ISSUES.md SI-11); if the owner rejects it, replace M1-T014…T025 with a single big-bang port card and drop this one.
