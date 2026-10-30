+++
id = "U10"
title = "U10: .jsc cache in the cache dir when --data-dir is set"
milestone = 0
deps = ["M0-T013", "U9"]
agent = "any"
owns = ["src/engine/js_engine.c", "src/engine/js_engine.h", "tests/test_js_engine.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
With `--data-dir`, the `.jsc` bytecode cache (js_engine.c:437–571) lives under the platform cache dir (`$XDG_CACHE_HOME/<app>/jsc/`) keyed by the script's game-relative path, never next to the script.

## Acceptance criteria
- [ ] New cases in tests/test_js_engine.c: with the option set, compiling a script creates the cache file under the cache dir and not beside the script; the cache is reused on the second load; without the option the old behaviour is unchanged.
- [ ] No file outside `src/engine/js_engine.c`, `src/engine/js_engine.h` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U10-jsc-cache-dir` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.6. Depends on U9 upstream as well.
