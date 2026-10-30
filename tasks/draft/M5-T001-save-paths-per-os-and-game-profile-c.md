+++
id = "M5-T001"
title = "Save paths per OS and game_profile.c"
milestone = 5
deps = []
agent = "any"
owns = ["src/smzuggler/game_profile.c", "src/smzuggler/**"]
docs = ["docs/saves.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Save dir per OS from `dir_save_name` read from plugins.js; game-specific values only in `src/smzuggler/game_profile.c` (§5.3, §13).

## Acceptance criteria
- [ ] Path tests per OS
- [ ] names gate passes
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M5 is planned. Not scheduled (tasks/draft/).

Spec §5.3, §13.
