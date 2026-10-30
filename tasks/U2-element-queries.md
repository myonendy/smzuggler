+++
id = "U2"
title = "U2: element getElementsByTagName and simple querySelector/querySelectorAll"
milestone = 0
deps = ["M0-T013", "M0-T005", "U1"]
agent = "any"
owns = ["src/shims/dom_shim.js", "tests/test_dom_shim.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Element-level `getElementsByTagName` (dom_shim.js:533) searches the element's subtree; `querySelector`/`querySelectorAll` on document and elements support simple selectors: a tag, `#id`, `.class`, a compound of those (`div.a#b`) and a comma list. Unsupported selectors throw a `SyntaxError`-named DOMException-like error rather than returning null silently.

## Acceptance criteria
- [ ] New cases in tests/test_dom_shim.c build a small tree and assert results and document order for each selector form, subtree-only matching, and the throw for an unsupported selector (`div > p`, `[x]`).
- [ ] `document.querySelector('head')` returns `document.head` (consistent with U1 when both are merged).
- [ ] `tools/format/es5_check.py --base upstream-main` (from M0-T005) passes on the changed shim files.
- [ ] No file outside `src/shims/dom_shim.js` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U2-element-queries` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.1, §5.7. Depends on U1 only to avoid conflicts in the same file; the upstream PRs stay independent.
