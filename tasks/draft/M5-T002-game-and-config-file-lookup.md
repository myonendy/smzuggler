+++
id = "M5-T002"
title = "--game and config file lookup"
milestone = 5
deps = []
agent = "any"
owns = ["src/smzuggler/config.c", "src/smzuggler/config.h", "src/main.c"]
docs = ["docs/saves.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Find the game by `--game` or the config file in `$XDG_CONFIG_HOME/smzuggler-rt/`.

## Acceptance criteria
- [ ] Lookup order tests

## Notes
First-pass draft from the §9 row; to be split and detailed when M5 is planned. Not scheduled (tasks/draft/).

Spec §5.3.
