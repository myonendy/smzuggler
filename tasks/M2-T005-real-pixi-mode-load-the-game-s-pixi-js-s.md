+++
id = "M2-T005"
title = "Real-PIXI mode: load the game's pixi.js, skip pixi_shim/tilemap_shim"
milestone = 2
deps = ["M2-T003"]
agent = "or-deepseek"
owns = ["src/engine/script_loader.c", "src/engine/script_loader.h", "src/main.c", "tests/smz/test_real_pixi_loader.c", "tests/smz/real_pixi_loader.cmake"]
docs = ["docs/webgl.md", "docs/shims.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
In real-PIXI mode (the default once WebGL2 exists), `script_loader.c` loads the game's own `js/libs/pixi.js` and does **not** load `pixi_shim.js` or `tilemap_shim.js` (shim list around `script_loader.c:246–256`, post-core tilemap shim around ~294/470 per §4.2). Keep Outsider's renderer selectable with `--renderer=outsider` for comparison and upstream tracking (§4.2, §8.3). `getContext('webgl2')` wiring is M2-T001; this card owns the loader and CLI mode switch.

## Acceptance criteria
- [ ] New ctest `real_pixi_loader`: in real-PIXI mode the loader's planned script list includes the game's `js/libs/pixi.js` (or the documented resolve of that path) and excludes `pixi_shim.js` and `tilemap_shim.js`; with `--renderer=outsider` the shims stay and the game's `pixi.js` is not forced in.
- [ ] `--renderer=outsider` still boots the previous Outsider renderer path (no regression in script order for that mode).
- [ ] On a GUI-capable run: title and Map 92 render in real-PIXI mode; Mode 7 floor is not black. Runs on the owner's machine under the GUI recipe below. If the worker cannot open a window or reach the game copy, stop and report `BLOCKED: needs dev-laptop GUI run` instead of faking screenshots or fps numbers.
- [ ] `docs/shims.md` and `docs/webgl.md` describe real-PIXI vs `--renderer=outsider`.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.2. GUI recipe: dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. Workers must not invent fps or screenshot diffs.

Owner 2026-11-04/05 agent workflow: run `forge check` once when finished and once after each fix; mid-work run just the one relevant test (`ctest -R real_pixi_loader`); read files by line range instead of whole files unless under 100 lines.
