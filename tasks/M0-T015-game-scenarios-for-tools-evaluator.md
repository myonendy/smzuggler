+++
id = "M0-T015"
title = "Game scenarios for tools/evaluator"
milestone = 0
deps = ["M0-T001", "M0-T023"]
agent = "any"
owns = ["tools/evaluator/**", "tests/tools/test_scenarios.py"]
docs = ["docs/testing.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add smzuggler scenarios to tools/evaluator for the §10 / §14.6 basics: boot → title; new game → first frames on Map 92; open the main menu; save to a slot in a scratch save directory and reload it; close the window and check a clean exit. Scenarios read the game from `SMZUGGLER_GAME_DIR`, write all output under `$XDG_STATE_HOME/smzuggler-rt/evaluator/`, and never write into the game dir. Reference captures from NW.js are made locally by the owner (recipe in `docs/testing.md`) and never committed.

## Acceptance criteria
- [ ] `tests/tools/test_scenarios.py` validates every new scenario file against the evaluator's schema and asserts none contains an absolute path, a game name or a save file.
- [ ] A dry run (`--dry-run`) prints the resolved paths and refuses a game dir that looks like a Steam or Proton folder (`steamapps`, `compatdata`).
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] The real run is executed once and its scrubbed summary (frame counts, pass/fail per step; no screenshots) is attached to the card report.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §9 M0, §10, §14.6. Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results. Scenario names use neutral words (title, map, menu, save), never the game's title.
