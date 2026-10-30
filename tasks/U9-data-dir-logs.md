+++
id = "U9"
title = "U9: opt-in --data-dir so logs go to the state dir"
milestone = 0
deps = ["M0-T013"]
agent = "any"
owns = ["src/main.c", "tests/test_data_dir.c", "tests/CMakeLists.txt"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Add an opt-in `--data-dir[=PATH]` flag: logs are written under the platform state dir (`$XDG_STATE_HOME/<app>/`, `SDL_GetPrefPath` elsewhere) or PATH, instead of the game dir (main.c:176–184). Without the flag behaviour is unchanged.

## Acceptance criteria
- [ ] A new C test `tests/test_data_dir.c` (registered in tests/CMakeLists.txt in upstream's style) checks the resolved log dir with the flag, with PATH, with XDG unset, and without the flag.
- [ ] No file outside `src/main.c` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U9-data-dir-logs` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.6. Keep main.c changes small: move path resolution into a function that the test can call.
