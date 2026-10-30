+++
id = "D-T002"
title = "Inherited docs: src/bindings headers, function comments and module README"
milestone = 2
deps = ["M0-T006"]
agent = "any"
owns = ["src/bindings/**"]
docs = ["src/bindings/README.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Bring `src/bindings` up to §15.1: file headers, function doc comments, `src/bindings/README.md`. Comments only; no behaviour change.

## Acceptance criteria
- [ ] `tools/doclint/doclint.py --paths src/bindings` reports nothing
- [ ] Build output unchanged (object files identical apart from debug line info)

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/).

Spec §9 D, §15.1. Doc-only edits to upstream files keep upstream's header plus the §12 modified mark.
