+++
id = "M1-T032"
title = "§2 test drive repeated on V8"
milestone = 1
deps = ["M1-T027"]
agent = "any"
owns = ["docs/milestones/**"]
docs = ["docs/milestones/M1-drive.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Repeat the §2 Outsider test drive on V8 with Outsider's renderer: title, new game to Map 92, main menu, save/load, using the M0-T015 scenarios. Write `docs/milestones/M1-drive.md` comparing each §2 row (fps, `dt_avg`, `js_update_avg`, max frame, crashes, driver timeouts) QuickJS vs V8.

## Acceptance criteria
- [ ] Every §2 row has a V8 value or a stated reason it cannot be measured.
- [ ] No compat or diagnostic script is used.
- [ ] The report passes `tools/privacy/scan.py --paths`.
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.

## Notes
Spec §2, §9 M1 exit. Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.
