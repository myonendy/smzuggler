+++
id = "M0-T010b"
title = "compat-surface rows for dom_shim.js"
milestone = 0
deps = ["M0-T010a"]
agent = "any"
owns = []
docs = ["docs/compat-surface.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Add `docs/compat-surface.md` rows for every dump member credited to `dom_shim.js` (~365 rows after M0-T009b). Read each shim to fill in status, test and notes, and mark every no-op as `stub`. Don't change other rows or `compat_check.py`.

## Acceptance criteria
- [ ] `compat_check.py build/surface.tsv --shim` with every shim file seeded so far passes against the current build's dump. That is every shim except `pixi_shim.js` (command and output in the report).
- [ ] Notes are specific to the member or its class, not one sentence pasted onto every row of a shim.
- [ ] The card's diff adds at most 1,500 lines.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
DEC-M0-T010-SPLIT: this is a data-only card, so `tests_required = false`. The checker run in the report is the test, and the `compat_surface` gate enforces it after M0-T010.
