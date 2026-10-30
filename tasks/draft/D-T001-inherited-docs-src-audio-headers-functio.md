+++
id = "D-T001"
title = "Inherited docs: src/audio headers, function comments and module README"
milestone = 2
deps = ["M0-T006"]
agent = "any"
owns = ["src/audio/**"]
docs = ["src/audio/README.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Bring `src/audio` up to §15.1: file headers, function doc comments, `src/audio/README.md`. Comments only; no behaviour change.

## Acceptance criteria
- [ ] `tools/doclint/doclint.py --paths src/audio` reports nothing
- [ ] Build output unchanged (object files identical apart from debug line info)

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/).

Spec §9 D, §15.1. Doc-only edits to upstream files keep upstream's header plus the §12 modified mark.
