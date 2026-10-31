+++
id = "M0-T010"
title = "compat-surface rows for pixi_shim.js; full table passes the check"
milestone = 0
deps = ["M0-T010b"]
agent = "any"
owns = []
docs = ["docs/compat-surface.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Add the remaining `docs/compat-surface.md` rows (every `pixi_shim.js` member, ~1,020 rows after M0-T009b) and remove the "not seeded yet" note, so the table has one row per dumped member (§15.4). Read each shim to fill in status, test and notes, and mark every no-op as `stub`.

## Acceptance criteria
- [ ] `python3 tools/doclint/compat_check.py build/surface.tsv`, with no `--shim`, passes against the current build's dump (command and output in the card's report).
- [ ] The stubs known from spec §2/§4.2/§5 are rows with status `stub` or `missing` (e.g. `PIXI.Geometry`, `PIXI.Buffer`, `nw.Window` events, `PIXI.Circle`).
- [ ] The card's diff adds at most 1,500 lines.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §15.4, DEC-M0-T010-SPLIT. This card keeps the ID M0-T010, so M0-T020, M0-T022 and U3 still depend on the complete table. Data-only, `tests_required = false`; after this card merges, the human adds the `compat_surface` gate (docs/FORGE.md).
