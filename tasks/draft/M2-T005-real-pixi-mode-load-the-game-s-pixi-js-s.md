+++
id = "M2-T005"
title = "Real-PIXI mode: load the game's pixi.js, skip pixi_shim/tilemap_shim"
milestone = 2
deps = ["M2-T003"]
agent = "any"
owns = ["src/engine/script_loader.c", "src/shims/canvas2d_shim.js", "src/main.c"]
docs = ["docs/webgl.md", "docs/shims.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
script_loader.c loads the game's `js/libs/pixi.js` in real-PIXI mode; `--renderer=outsider` keeps Outsider's mode.

## Acceptance criteria
- [ ] Title and Map 92 render in real-PIXI mode; Mode 7 floor not black
- [ ] `--renderer=outsider` unchanged

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/). Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.

Spec §4.2.
