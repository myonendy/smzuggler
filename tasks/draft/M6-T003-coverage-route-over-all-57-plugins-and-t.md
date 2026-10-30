+++
id = "M6-T003"
title = "Coverage route over all 57 plugins and the replay corpus"
milestone = 6
deps = []
agent = "any"
owns = ["tools/evaluator/**"]
docs = ["docs/shims.md", "docs/compat-surface.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Exercise every plugin; replay corpus passes.

## Acceptance criteria
- [ ] Coverage report lists every plugin exercised
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M6 is planned. Not scheduled (tasks/draft/). Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.

Spec §9 M6, §10.
