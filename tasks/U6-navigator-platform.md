+++
id = "U6"
title = "U6: navigator.platform and userAgent consistent with the OS"
milestone = 0
deps = ["M0-T013", "M0-T005"]
agent = "any"
owns = ["src/shims/navigator_shim.js", "tests/test_dom_shim.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
`navigator.platform` and `navigator.userAgent` report the real OS in Chrome's format (e.g. `Linux x86_64` and an X11 UA on Linux, `MacIntel` on macOS, `Win32` on Windows) instead of always Windows (navigator_shim.js:24). Use `process.platform`/`arch` from U5 when present, else a native fallback.

## Acceptance criteria
- [ ] New cases in tests/test_dom_shim.c assert platform and UA per compile target and that the UA keeps the Chrome version token the shim already reports.
- [ ] `tools/format/es5_check.py --base upstream-main` (from M0-T005) passes on the changed shim files.
- [ ] No file outside `src/shims/navigator_shim.js` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U6-navigator-platform` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.3 (navigator_shim.js:15–40). U5 helps but is not required.
