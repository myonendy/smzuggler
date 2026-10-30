+++
id = "D-T008"
title = "Inherited docs: src/rendering headers, function comments and module README"
milestone = 2
deps = ["M0-T006"]
agent = "any"
owns = ["src/rendering/**"]
docs = ["src/rendering/README.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Bring `src/rendering` up to §15.1: file headers, function doc comments, `src/rendering/README.md`. Comments only; no behaviour change.

## Acceptance criteria
- [ ] `tools/doclint/doclint.py --paths src/rendering` reports nothing
- [ ] Build output unchanged (object files identical apart from debug line info)
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/).

Spec §9 D, §15.1. Doc-only edits to upstream files keep upstream's header plus the §12 modified mark.
