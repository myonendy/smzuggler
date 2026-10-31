+++
id = "M0-T010b"
title = "compat-surface rows for dom_shim.js"
milestone = 0
deps = ["M0-T010a"]
agent = "any"
owns = ["tools/doclint/compat_check.py", "tests/tools/test_compat_check.py"]
docs = ["docs/compat-surface.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `docs/compat-surface.md` rows for every dump member credited to `dom_shim.js` (~365 rows after M0-T009b). Read each shim to fill in status, test and notes, and mark every no-op as `stub`. Don't change other rows. `table_rows()` in `compat_check.py` skips every row whose first cell is `name`, which drops the real `name` member (`window.name`). Change only that header test so it skips the exact header row (`name | status | shim file | test | notes`) and the separator row; change nothing else in the checker. Remove the caveat paragraph about the `name` row from `docs/compat-surface.md` (lines 31–33 in an earlier attempt) if it is there.

## Acceptance criteria
- [ ] `compat_check.py build/surface.tsv --shim` with every shim file seeded so far passes against the current build's dump. That is every shim except `pixi_shim.js` (command and output in the report).
- [ ] A new test proves a documented member called `name` is matched. Put it in a new file under `tests/tools/` (the card has `may_edit_tests = false`, so `tests/tools/test_compat_check.py` is not edited); the existing tests pass unedited (command and output in the report).
- [ ] Notes are specific to the member or its class, not one sentence pasted onto every row of a shim.
- [ ] The card's diff adds at most 1,500 lines.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
DEC-M0-T010-SPLIT made this a data-only card; DEC-M0-T010b-HEADER adds the one-line checker fix, so `tests_required = true`. The checker run in the report is still required, and the `compat_surface` gate enforces it after M0-T010.
