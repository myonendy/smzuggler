+++
id = "M7-T004"
title = "30-minute soak and full §14 suite on dev-laptop"
milestone = 7
deps = []
agent = "any"
owns = ["docs/milestones/**"]
docs = ["docs/milestones/M7.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
S1–S9 including soak on battery power-saver.

## Acceptance criteria
- [ ] All §14.5 criteria including soak drift and memory growth
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.

## Notes
First-pass draft from the §9 row; to be split and detailed when M7 is planned. Not scheduled (tasks/draft/). Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.

Spec §14.
