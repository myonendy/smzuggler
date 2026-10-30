+++
id = "M3-T002"
title = "Fix the HUD text cause"
milestone = 3
deps = ["M3-T001"]
agent = "any"
owns = ["src/rendering/**", "src/shims/**"]
docs = ["docs/canvas2d-text.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Fix per M3-T001; U13 if the cause is in Outsider's code.

## Acceptance criteria
- [ ] Repro test passes
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M3 is planned. Not scheduled (tasks/draft/).

Spec §5.5.
