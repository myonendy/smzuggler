+++
id = "U3"
title = "U3: nw.Window.get() as a minimal EventEmitter plus window methods"
milestone = 0
deps = ["M0-T013", "M0-T005", "M0-T010"]
agent = "any"
owns = ["src/shims/storage_shim.js", "tests/test_storage.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
`nw.Window.get()` returns one window object with `on`/`once`/`off`/`removeListener`/`removeAllListeners`/`emit`/`listeners` (Node EventEmitter semantics for these), plus the §5.2 window methods with working or documented no-op behaviour. Today only a no-op `on` exists (storage_shim.js 374–418), so CleanExit throws on `removeAllListeners`.

## Acceptance criteria
- [ ] New cases in tests/test_storage.c: listener order, `once` fires once, `off`/`removeListener` remove one registration, `removeAllListeners(name)` and `()`, `emit` returns whether listeners ran, `listeners` returns a copy, `get()` returns the same object each time.
- [ ] Each §5.2 window method exists; no-ops are listed in docs/compat-surface.md as `stub` with a reason.
- [ ] `tools/format/es5_check.py --base upstream-main` (from M0-T005) passes on the changed shim files.
- [ ] No file outside `src/shims/storage_shim.js` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U3-nw-window-events` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.2.
