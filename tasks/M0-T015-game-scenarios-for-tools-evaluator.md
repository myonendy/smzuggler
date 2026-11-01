+++
id = "M0-T015"
title = "Game scenarios for tools/evaluator"
milestone = 0
deps = ["M0-T001", "M0-T023"]
agent = "any"
owns = ["tools/evaluator/**", "tests/tools/test_scenarios.py", "tests/tools/test_scenario_bundle.py", "ARCHITECTURE.md"]
docs = ["docs/testing.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add smzuggler scenarios to tools/evaluator for the §10 / §14.6 basics: boot → title; new game → first frames on Map 92; open the main menu; save to a slot in a scratch save directory and reload it; close the window and check a clean exit. Scenarios read the game from `SMZUGGLER_GAME_DIR`, write all output under `$XDG_STATE_HOME/smzuggler-rt/evaluator/`, and never write into the game dir. Reference captures from NW.js are made locally by the owner (recipe in `docs/testing.md`) and never committed.

## Acceptance criteria
- [ ] `tests/tools/test_scenarios.py` validates every new scenario file against the evaluator's schema and asserts none contains an absolute path, a game name or a save file.
- [ ] A dry run (`--dry-run`) prints the resolved paths and refuses a game dir that looks like a Steam or Proton folder (`steamapps`, `compatdata`).
- [ ] A run's output directory has the layout `tools/share/make_bundle.py` (M0-T023) accepts. A new test `tests/tools/test_scenario_bundle.py` builds a synthetic run directory in that layout (a log, a perf report, a frame-time CSV and a placeholder screenshot, all fake data), calls the real `make_bundle.py` on it, and asserts the bundle is written, contains README.txt with a pass/fail summary per step, and has no privacy hit. Nothing is posted anywhere by the card.
- [ ] The map step checks `$gameMap.mapId() === 92` directly, not the game's configured start map, and a test asserts that check is in the scenario.
- [ ] Save and reload really write to and read from the scratch save directory: the prelude the scenarios load does not stub `StorageManager.saveObject`, `loadObject` or `exists`, and a test inspects that prelude to prove it.
- [ ] All save IO is isolated to the scratch area: the game process is started with its own scratch `HOME`, `XDG_DATA_HOME`, `XDG_CONFIG_HOME`, `XDG_STATE_HOME` and `XDG_CACHE_HOME` under the evaluator's output root (never the caller's), so the game's save-location plugin resolves inside it, and the save check looks where that plugin writes (`$XDG_DATA_HOME/<dir_save_name>/` of the scratch tree) as well as the staged game's `save/`. A test captures the environment passed to the game process and asserts every one of those variables points inside the scratch root, and another asserts the save check finds a slot in the scratch XDG tree (both `forage_*file1` and `file1.rmmzsave` names).
- [ ] If the driver cannot start, `play()` reports the boot error itself (no `UnboundLocalError` from cleanup) and still writes its partial outputs.
- [ ] ARCHITECTURE.md's rows for `tools/evaluator/` and `tools/evaluator/scenarios/` are updated for the fork entry point and the new scenarios; no other row changes.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §9 M0, §10, §14.6. Implement and test everything above without a game window or the game copy; forge workers have neither. The real run (dev-laptop, battery, `power-saver`, test copy in `SMZUGGLER_GAME_DIR`, never the Steam folder) and its bundle are the owner's after-merge checkpoint in docs/FORGE.md (DEC-M0-T015-OWNER-RUN), so the card report does not need real-run evidence and must not fake any. Keep the work already on the branch and finish it rather than starting over. The last review (DEC-M0-T015-SAVE-ISOLATION) found that since U5 made `process.env` real, an inherited environment lets the game's save plugin target the owner's real save folder; fixing that is the priority. Scenario names use neutral words (title, map, menu, save), never the game's title.
