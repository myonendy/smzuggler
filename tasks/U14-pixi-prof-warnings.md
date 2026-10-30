+++
id = "U14"
title = "U14: clearer [PROF] warnings for missing PIXI members"
milestone = 0
deps = ["M0-T013", "M0-T005", "U8"]
agent = "any"
owns = ["src/shims/pixi_shim.js", "tests/test_pixi_shim.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Optional. Make pixi_shim.js log one `[PROF] missing: <Class>.<member>` warning per distinct missing member a game touches (today the `[PROF]` line lists renderer/plugin gaps only), rate-limited and off unless profiling is on.

## Acceptance criteria
- [ ] New cases in tests/test_pixi_shim.c touch two missing members three times each and assert exactly two warnings, and none with profiling off.
- [ ] `tools/format/es5_check.py --base upstream-main` (from M0-T005) passes on the changed shim files.
- [ ] No file outside `src/shims/pixi_shim.js` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U14-pixi-prof-warnings` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.4, §8.1 (optional). Depends on U8 only to serialise edits to pixi_shim.js.
