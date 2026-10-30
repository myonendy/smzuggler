+++
id = "M1-T025"
title = "Port bind_tilemap.c to jsb"
milestone = 1
deps = ["M1-T005", "M1-T006", "M1-T007"]
agent = "any"
owns = ["src/bindings/bind_tilemap.c", "src/bindings/bind_tilemap.h"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Rewrite `src/bindings/bind_tilemap.c` on jsb (core, buffers and callbacks as needed), keeping its JS-visible API byte-for-byte (names, argument order, return shapes, errors). Near-mechanical: keep the structure so upstream diffs re-apply (§8.3). Outsider's renderer bindings are kept only through M1 as the measuring baseline (§4.2); port them mechanically, no redesign.

## Acceptance criteria
- [ ] tests/test_tilemap.c passes in both engine modes.
- [ ] `git grep -n 'JS_\|quickjs' -- src/bindings/bind_tilemap.c src/bindings/bind_tilemap.h` is empty.
- [ ] `--dump-surface` output for the `tilemap` module is identical before and after (diff in the card report).
- [ ] `docs/js-binding.md` has a row for bind_tilemap (functions, buffers, callbacks used).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] While M1 is in progress both engine modes build (`SMZ_JS_ENGINE=quickjs|v8`, from M1-T002) and the ctest suite passes in both; see DEC-SETUP-008 / SI-11 for the transitional QuickJS jsb backend.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1, §9 M1 ("all 12 bindings ported").
