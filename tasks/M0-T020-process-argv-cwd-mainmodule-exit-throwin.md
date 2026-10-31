+++
id = "M0-T020"
title = "process.argv/cwd/mainModule/exit, throwing require, plugin require scan"
milestone = 0
deps = ["U5", "U3", "M0-T010"]
agent = "any"
owns = ["src/shims/storage_shim.js", "src/main.c", "tools/compat/**", "tests/smz/test_process_shim.c", "tests/smz/process_shim.cmake", "tests/tools/test_scan_requires.py", "ARCHITECTURE.md"]
docs = ["docs/compat-surface.md", "docs/shims.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Finish the §5.3 `process` surface not covered by U5: `process.argv`, `process.cwd()`, `process.mainModule.filename` (Chrome/NW.js-shaped values, game-relative where NW.js gives game-relative), `process.exit(code)` routed through the clean quit path. `require()` of an unknown module throws an Error with `code: 'MODULE_NOT_FOUND'` instead of returning undefined. Add `tools/compat/scan_requires.py`, which lists every `require(...)` and `process.*` use in a game's `js/plugins/*.js` from `SMZUGGLER_GAME_DIR`, printing plugin file names and counts only.

## Acceptance criteria
- [ ] `ARCHITECTURE.md` has a directory-map row for `tools/compat/` (required by `tests/tools/test_architecture_refs.py`); no other ARCHITECTURE.md change.
- [ ] Shim tests (new file `tests/smz/test_process_shim.c` via `tests/smz/process_shim.cmake`) assert each new member's value and that `require('nope')` throws with `code === 'MODULE_NOT_FOUND'` while the modules the shims provide still load.
- [ ] `tests/tools/test_scan_requires.py` runs the scanner over synthetic plugin files.
- [ ] The scan is run once against the test copy; its summary (plugin names, counts) is in the card report and shows no plugin depends on `require` returning undefined, or lists those that do.
- [ ] `docs/compat-surface.md` rows updated.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §5.3 ("after re-scanning all 57 plugins"), docs/SPEC_ISSUES.md SI-10. This is smzuggler code, not a U-PR.
