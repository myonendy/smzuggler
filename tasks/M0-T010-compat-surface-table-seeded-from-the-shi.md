+++
id = "M0-T010"
title = "Compat-surface table seeded from the shims, plus its consistency check"
milestone = 0
deps = ["M0-T009"]
agent = "any"
owns = ["tools/doclint/compat_check.py", "tests/tools/test_compat_check.py"]
docs = ["docs/compat-surface.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Create `docs/compat-surface.md` (§15.4): one row per dumped member, with columns name, status (`real`, `partial`, `stub`, `missing`), shim file, test and notes. Seed it from `--dump-surface` and a read of each shim; mark every no-op as `stub`. Add `tools/doclint/compat_check.py DUMP_FILE`: fails when the table and the dump disagree (members missing from either side), when a row's shim file does not exist, or when a `real`/`partial` row names a test file that does not exist.

## Acceptance criteria
- [ ] `tests/tools/test_compat_check.py` covers: table and dump equal passes; extra dump member fails; extra table row fails; missing shim file fails; missing test file fails.
- [ ] The seeded table passes the check against the current build's dump (run in the card's report).
- [ ] The stubs known from spec §2/§4.2/§5 are rows with status `stub` or `missing` (e.g. `PIXI.Geometry`, `PIXI.Buffer`, `nw.Window` events, `PIXI.Circle`).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.

## Notes
Spec §15.4. The dump is produced by the build; the check reads it from a file so it stays a pure Python test.
