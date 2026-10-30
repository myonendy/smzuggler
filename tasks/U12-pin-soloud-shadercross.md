+++
id = "U12"
title = "U12: pin SoLoud and SDL_shadercross to fixed revisions"
milestone = 0
deps = ["M0-T013"]
agent = "any"
owns = ["third_party/CMakeLists.txt", "tests/tools/test_pins.py"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Pin SoLoud (`GIT_TAG master`) and SDL_shadercross (`main`) in third_party/CMakeLists.txt to the full commit SHAs the current build uses (from the local deps cache), with a comment giving the date and upstream branch.

## Acceptance criteria
- [ ] `tests/tools/test_pins.py` parses third_party/CMakeLists.txt and asserts every FetchContent `GIT_TAG` is a 40-hex SHA or a version tag (no branch names).
- [ ] A clean configure and build with `FETCHCONTENT_FULLY_DISCONNECTED=ON` from the deps cache at those SHAs passes, and the full ctest suite passes.
- [ ] No file outside `third_party/CMakeLists.txt` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U12-pin-soloud-shadercross` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §8.1, Appendix A. `tests/tools/test_pins.py` stays in smzuggler; the upstream PR carries only the CMake change.
