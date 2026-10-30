+++
id = "M2-T009"
title = "M2 exit: §14 acceptance S1–S3 on dev-laptop"
milestone = 2
deps = ["M2-T005", "M2-T007"]
agent = "any"
owns = ["docs/milestones/**"]
docs = ["docs/milestones/M2.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Title, Map 92, menus within tolerance; no WebGL errors over a 10-minute walk; §14.5 criteria on dev-laptop battery power-saver.

## Acceptance criteria
- [ ] Reports with gate=pass for S1–S3
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/). Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.

Spec §9 M2, §14.
