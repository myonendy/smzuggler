+++
id = "M5-T004"
title = "Byte-identical saves vs NW.js"
milestone = 5
deps = ["M5-T001"]
agent = "any"
owns = ["tools/**", "tests/smz/**"]
docs = ["docs/saves.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Save diff identical to NW.js.

## Acceptance criteria
- [ ] Diff tool shows identical saves for a scripted save
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M5 is planned. Not scheduled (tasks/draft/). Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.

Spec §9 M5.
