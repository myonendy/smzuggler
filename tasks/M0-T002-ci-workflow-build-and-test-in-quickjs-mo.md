+++
id = "M0-T002"
title = "CI workflow: build and test in QuickJS mode on Linux, Windows and macOS"
milestone = 0
deps = []
agent = "any"
owns = [".github/workflows/ci.yml", "tests/tools/test_ci_workflow.py", "README.md"]
docs = ["docs/testing.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `.github/workflows/ci.yml` that configures, builds and runs ctest plus the Python tool tests (`tools/forge/test.sh` equivalent) on ubuntu, windows and macos runners, using upstream's `release.yml` as the reference for dependencies and caching. Runs on push and pull_request. Needs no game files: tests that need the game skip only through upstream's existing `RMMZ_TEST_GAME_DIR` mechanism. Start `docs/testing.md` with how to run the same steps locally.

## Acceptance criteria
- [ ] `tests/tools/test_ci_workflow.py` loads ci.yml with PyYAML and asserts: jobs for linux, windows and macos; triggers on push and pull_request; each job runs the cmake build with `RMMZ_BUILD_TESTS=ON`, then ctest, then the Python tool tests; no job sets `SMZUGGLER_GAME_DIR` or `RMMZ_TEST_GAME_DIR`.
- [ ] The workflow pins actions by full commit SHA (checked by the same test).
- [ ] `docs/testing.md` documents the local equivalent (`tools/forge/build.sh`, `tools/forge/test.sh`) and is linked from README.md.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §9 M0, §15.7. CI cannot run until the owner pushes (docs/SPEC_ISSUES.md SI-6); this card is verified locally. Do not edit release.yml.
