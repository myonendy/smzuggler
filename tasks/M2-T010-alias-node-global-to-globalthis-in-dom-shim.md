+++
id = "M2-T010"
title = "Alias Node/NW.js global to globalThis in dom_shim"
milestone = 2
deps = []
agent = "or-deepseek"
owns = ["src/shims/dom_shim.js", "tests/test_dom_shim.c"]
docs = ["docs/shims.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
NW.js / Node expose a `global` alias to the same object as `globalThis`. This host already sets `globalThis.window` and `globalThis.self` in `src/shims/dom_shim.js` (around the existing Globals block near the `window` / `self` assignments) but never `global`. Game plugin `SafeTransferCleanup.js` runs `typeof global.gc === "function"` and may call `global.gc()` from a `Scene_Map.terminate` hook during map transfer. Evaluating `typeof global.gc` still resolves `global` first, so an undeclared `global` is a `ReferenceError` and the transfer dies. Alias `globalThis.global = globalThis` next to the `window` / `self` assignments. Do **not** edit the game copy or any plugin. After the alias, `global.gc` is simply missing unless a later card exposes GC, so the plugin's `typeof` check is false and it skips the call.

## Acceptance criteria
- [ ] After loading `dom_shim.js`, `global === globalThis` and `global === window` in the engine (same object identity).
- [ ] Existing test file `tests/test_dom_shim.c` gains a focused assertion (or a small helper eval) that `typeof global !== "undefined"`, `global === globalThis`, and `typeof global.gc === "function"` is false unless GC was explicitly exposed — proving the `SafeTransferCleanup.js` pattern does not throw.
- [ ] No files under the game copy / `SMZUGGLER_GAME_DIR` are modified.
- [ ] `docs/shims.md` notes the `global` alias and why it exists (NW.js parity; map-transfer plugins that probe `global.gc`).
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Unblocks map transfers and the later 10-minute walk (M2-T009). Mention plugins by filename only. Shares `src/shims/dom_shim.js` with M2-T001; forge serialises on overlapping owns — if both are ready, either order is fine as long as the alias remains.

Owner 2026-11-04/05 agent workflow: run `forge check` once when finished and once after each fix; mid-work run just the one relevant test (`ctest -R dom_shim` or the binary this repo uses for `tests/test_dom_shim.c`); read files by line range instead of whole files unless under 100 lines.
