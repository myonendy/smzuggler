+++
id = "M0-T014b"
title = "§12 headers for changed files outside M0-T014's owns"
milestone = 0
deps = ["M0-T014", "M0-T012"]
agent = "any"
owns = [".gitignore", "CONTRIBUTING.md", "tests/CMakeLists.txt", "tests/integration/test_acceptance.c"]
docs = ["docs/DECISIONS.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = true
+++
## Goal
Add the §12 header (our copyright line and a dated "modified" note; upstream copyright and SPDX lines unchanged) to every file that DEC-M0-T014 lists as failing `tools/doclint/headers.py --base upstream-main` and that this card owns. Header lines only: no other change to any test file (same assertions, same ASSERT count). Protected files (AGENTS.md, CLAUDE.md, forge.toml, tasks/**, docs/spec/**, prompts/**, .claude/**, .codex/**, tools/forge|privacy|commitmsg/**, .gitattributes) are fixed by a human in the same change set, not by an agent.

## Acceptance criteria
- [ ] `python3 tools/doclint/headers.py --base upstream-main` exits 0 on the full tree once the human's protected-file headers are in (command and output in the report).
- [ ] `git diff upstream-main -- tests/` shows only added comment lines in the test files this card touches.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
DEC-M0-T014-SCOPE. Draft: the owner decides when to promote it, because it edits upstream test files (header-only) and needs a human for the protected paths. Data-only header edits, so `tests_required = false`; the header check run is the test.
