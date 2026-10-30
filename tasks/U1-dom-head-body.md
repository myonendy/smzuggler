+++
id = "U1"
title = "U1: document.getElementsByTagName returns head, body and html"
milestone = 0
deps = ["M0-T013", "M0-T005"]
agent = "any"
owns = ["src/shims/dom_shim.js", "tests/test_dom_shim.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
`document.getElementsByTagName('head'|'body'|'html')` (case-insensitive) returns a one-element collection with `document.head`, `document.body` or `document.documentElement`, and `'*'` includes them. Today it returns `[]` for these (dom_shim.js 1337–1344), which crashes Mimosa_MouseCursor (§2, §5.1).

## Acceptance criteria
- [ ] New cases in tests/test_dom_shim.c: each tag name in lower and upper case returns exactly the same object as `document.head`/`body`/`documentElement`; `length` is 1; other tags behave as before; `'*'` contains them.
- [ ] `tools/format/es5_check.py --base upstream-main` (from M0-T005) passes on the changed shim files.
- [ ] No file outside `src/shims/dom_shim.js` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U1-dom-head-body` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.1 (dom_shim.js:1261/1269/1337–1344).
