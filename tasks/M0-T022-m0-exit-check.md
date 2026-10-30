+++
id = "M0-T022"
title = "M0 exit check"
milestone = 0
deps = ["M0-T002", "M0-T003", "M0-T004", "M0-T005", "M0-T006", "M0-T007", "M0-T010", "M0-T011", "M0-T012", "M0-T013", "M0-T014", "M0-T016", "M0-T018", "M0-T019", "M0-T020", "M0-T021", "M0-T023", "U1", "U2", "U3", "U4", "U5", "U6", "U7", "U8", "U9", "U10", "U11", "U12"]
agent = "any"
owns = ["docs/milestones/**"]
docs = ["docs/milestones/M0.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Verify and record the §9 M0 exit: smzuggler (QuickJS mode) boots the game to the map with no compat script, no game-dir writes and a correct save dir; the profile report exists; all local gates pass. Run `tools/smoke/game_smoke.sh` and the evaluator scenarios, save once from the map, and check the save lands in `$XDG_DATA_HOME/<dir_save_name>/` of a scratch XDG tree. Write `docs/milestones/M0.md` with the checklist, the evidence (scrubbed) and any open items.

## Acceptance criteria
- [ ] `docs/milestones/M0.md` has one line per M0 exit criterion with pass/fail and evidence, and passes `tools/privacy/scan.py --paths`.
- [ ] The run uses no compat or diagnostic script (the command line and shim list are in the report).
- [ ] The game dir tree hash is unchanged after the run; the save file is in the scratch data dir under the game's `dir_save_name`.
- [ ] CI items that need a push are listed as pending, not as passed (SI-6).
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.

## Notes
Spec §9. Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.
