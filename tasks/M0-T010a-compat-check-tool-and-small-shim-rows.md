+++
id = "M0-T010a"
title = "compat_check.py and compat-surface rows for the smaller shims"
milestone = 0
deps = ["M0-T009b"]
agent = "any"
owns = ["tools/doclint/compat_check.py", "tests/tools/test_compat_check.py"]
docs = ["docs/compat-surface.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Create `docs/compat-surface.md` (§15.4) with columns name, status (`real`, `partial`, `stub`, `missing`), shim file, test and notes, and add `tools/doclint/compat_check.py DUMP_FILE [--shim FILE ...]`. It fails when the table and the dump disagree (members missing from either side), when a row's shim file does not exist, or when a `real`/`partial` row names a test file that does not exist. `--shim` limits both sides to rows and dump members credited to the named shim files. Without `--shim` everything is checked. Seed the rows for every shim except `dom_shim.js` and `pixi_shim.js` (webaudio, plugin_compat, canvas2d, navigator, font, storage, dom_overlay, xhr, effekseer: ~600 rows after M0-T009b). Seed by running `--dump-surface` and reading each shim, and mark every no-op as `stub`. Include the known §2/§4.2/§5 stub rows now, whatever shim they belong to: `PIXI.Geometry`, `PIXI.Buffer`, `PIXI.Program`, `PIXI.Circle` (`missing`), `nw.Window.get`, `process.env` and `process.platform`. Say above the table which shims are not seeded yet (M0-T010b, M0-T010).

## Acceptance criteria
- [ ] `tests/tools/test_compat_check.py` covers: table and dump equal passes; extra dump member fails; extra table row fails; missing shim file fails; missing test file fails; `--shim` ignores rows and members of other shims but still fails on a mismatch inside the named shim.
- [ ] `compat_check.py build/surface.tsv --shim F ...`, with every seeded shim file, passes against the current build's dump (command and output in the card's report).
- [ ] The §2/§4.2/§5 stub rows above exist with status `stub` or `missing`.
- [ ] The card's diff adds at most 1,500 lines (policy SIZE gate).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §15.4. DEC-M0-T010-SPLIT. The dump comes from the build (`./build/smzuggler-rt --dump-surface=build/surface.tsv`, headless, no game), and the check reads it from a file so it stays a pure Python test. The full unfiltered check only has to pass at M0-T010.
