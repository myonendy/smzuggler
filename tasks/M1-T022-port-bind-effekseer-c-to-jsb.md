+++
id = "M1-T022"
title = "Port bind_effekseer.c to jsb"
milestone = 1
deps = ["M1-T005", "M1-T006", "M1-T007"]
agent = "any"
owns = ["src/bindings/bind_effekseer.c", "src/bindings/bind_effekseer.h"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Rewrite `src/bindings/bind_effekseer.c` on jsb (core, buffers and callbacks as needed), keeping its JS-visible API byte-for-byte (names, argument order, return shapes, errors). Near-mechanical: keep the structure so upstream diffs re-apply (§8.3).

## Acceptance criteria
- [ ] tests/test_effekseer.c passes in both engine modes.
- [ ] `git grep -n 'JS_\|quickjs' -- src/bindings/bind_effekseer.c src/bindings/bind_effekseer.h` is empty.
- [ ] `--dump-surface` output for the `effekseer` module is identical before and after (diff in the card report).
- [ ] `docs/js-binding.md` has a row for bind_effekseer (functions, buffers, callbacks used).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] While M1 is in progress both engine modes build (`SMZ_JS_ENGINE=quickjs|v8`, from M1-T002) and the ctest suite passes in both; see DEC-SETUP-008 / SI-11 for the transitional QuickJS jsb backend.

## Notes
Spec §4.1, §9 M1 ("all 12 bindings ported").
