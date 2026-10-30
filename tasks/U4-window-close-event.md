+++
id = "U4"
title = "U4: deliver the close event from SDL quit and window close"
milestone = 0
deps = ["M0-T013", "M0-T005", "U3"]
agent = "any"
owns = ["src/platform/platform_sdl3.c", "src/engine/game_loop.c", "src/shims/storage_shim.js", "tests/test_game_loop.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
SDL quit / window-close requests emit `close` on the `nw.Window` object instead of quitting immediately; if a listener exists the app keeps running until it calls `win.close(true)` or `nw.App.quit()`; with no listener the app quits as today. `close(true)` and `nw.App.quit()` end the loop cleanly.

## Acceptance criteria
- [ ] A C test (new cases in tests/test_game_loop.c) pushes an SDL quit event and asserts the JS `close` listener ran and the loop keeps running, then that `close(true)` stops it; without a listener the loop stops.
- [ ] A control-mode test or scripted run shows CleanExit's close path ends the process with exit code 0 (attached to the card report).
- [ ] `tools/format/es5_check.py --base upstream-main` (from M0-T005) passes on the changed shim files.
- [ ] No file outside `src/platform/platform_sdl3.c`, `src/engine/game_loop.c`, `src/shims/storage_shim.js` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U4-window-close-event` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.2. U4 depends on U3 upstream as well.
