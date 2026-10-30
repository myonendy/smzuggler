+++
id = "M6-T005"
title = "M6 exit: §14 S4–S8 on dev-laptop"
milestone = 6
deps = ["M6-T001", "M6-T003"]
agent = "any"
owns = ["docs/milestones/**"]
docs = ["docs/milestones/M6.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Trade screen, naval combat, video, dialogue, save/load pass §14.5.

## Acceptance criteria
- [ ] Reports with gate=pass for S4–S8
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M6 is planned. Not scheduled (tasks/draft/). Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.

Spec §9 M6, §14.
