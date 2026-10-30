+++
id = "U11"
title = "U11: canPlayType answers match what the build can decode"
milestone = 0
deps = ["M0-T013", "M0-T005", "U2"]
agent = "any"
owns = ["src/shims/dom_shim.js", "tests/test_dom_shim.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
`canPlayType` (dom_shim.js 1150/1193–1197) answers from a table of what this build decodes (today MPEG-1 video, Ogg Vorbis and WAV audio) and returns `''` for everything else, including `video/webm; codecs="vp9,opus"` until M6 adds it.

## Acceptance criteria
- [ ] New cases in tests/test_dom_shim.c for each supported type (`probably`/`maybe` as Chrome would answer for a supported codec) and for webm/vp9/opus/mp4 returning `''`.
- [ ] `tools/format/es5_check.py --base upstream-main` (from M0-T005) passes on the changed shim files.
- [ ] No file outside `src/shims/dom_shim.js` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U11-canplaytype` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.7. Depends on U2 only to serialise edits to dom_shim.js.
