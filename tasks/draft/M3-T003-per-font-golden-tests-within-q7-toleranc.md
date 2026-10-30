+++
id = "M3-T003"
title = "Per-font golden tests within Q7 tolerance"
milestone = 3
deps = ["M3-T002"]
agent = "any"
owns = ["tests/smz/**", "tools/evaluator/**"]
docs = ["docs/canvas2d-text.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Goldens for each game font (rendered locally, not committed if they are game content; Q7 tolerance).

## Acceptance criteria
- [ ] HUD text matches reference within Q7 tolerance
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M3 is planned. Not scheduled (tasks/draft/).

Spec §9 M3, Q6/Q7.
