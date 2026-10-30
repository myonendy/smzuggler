+++
id = "M3-T001"
title = "Root-cause the missing HUD text (§5.5)"
milestone = 3
deps = []
agent = "any"
owns = ["docs/canvas2d-text.md", "tests/smz/**"]
docs = ["docs/canvas2d-text.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Find why HUD panels draw without text while message windows draw: font, a Canvas2D op or a plugin path.

## Acceptance criteria
- [ ] Root cause with file:line and a minimal repro test
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M3 is planned. Not scheduled (tasks/draft/). Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.

Spec §5.5.
