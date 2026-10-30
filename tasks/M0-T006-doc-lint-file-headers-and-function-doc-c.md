+++
id = "M0-T006"
title = "Doc lint: file headers and function doc comments (§15.1)"
milestone = 0
deps = []
agent = "any"
owns = ["tools/doclint/doclint.py", "tests/tools/test_doclint.py"]
docs = ["docs/doclint-baseline.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `tools/doclint/doclint.py`. For C/C++ (`.c .cc .cpp .h`) and JS files it checks the §15.1 file header (SPDX line, copyright line, one-paragraph purpose) and that every function definition and every public header prototype has a doc comment directly above it. Modes: `--changed BASE` (only files changed since BASE, and only functions whose lines changed) and `--all`. Output is file:line plus a rule id; exit 1 on any finding.

## Acceptance criteria
- [ ] `tests/tools/test_doclint.py` covers, with small inline sample files: a missing header, a header without SPDX, an undocumented C function, a documented one, a static function, a prototype in a header, a JS function declaration, a JS method assigned to a prototype, a multi-line signature, and `--changed` ignoring untouched functions.
- [ ] `--changed upstream-main` passes on the current tree (no smzuggler code changed yet besides tools).
- [ ] `--all` runs over `src/` in under 10 s and its finding count is written to `docs/doclint-baseline.md` (the D milestone burns it down).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §15.1, §15.7. No compiler or node dependency: a careful line-based parser is enough for this codebase; document its limits in the module docstring. A human adds the gate after merge.
