+++
id = "U7"
title = "U7: refuse save writes to a relative path or inside the game root"
milestone = 0
deps = ["M0-T013"]
agent = "any"
owns = ["src/bindings/bind_io.c", "src/io/file_io.c", "src/io/file_io.h", "tests/test_file_io.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
File writes coming from the save path (`bind_io.c` → `file_io.c`) are refused when the path is relative or resolves inside the game root (after `..` and symlink resolution), with an error logged and the JS call failing as a write error would. Reads are unchanged.

## Acceptance criteria
- [ ] New cases in tests/test_file_io.c: absolute path outside the root succeeds; relative path refused; `root/../root/x` refused; a symlink inside the root pointing out is resolved and judged by its target; the error is logged once per call.
- [ ] No file outside `src/bindings/bind_io.c`, `src/io/file_io.c`, `src/io/file_io.h` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U7-refuse-bad-save-paths` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.3.
