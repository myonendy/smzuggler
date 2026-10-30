+++
id = "M1-T030"
title = "docs/js-binding.md complete, doc comments on all of jsb and the bindings"
milestone = 1
deps = ["M1-T027"]
agent = "any"
owns = ["src/jsb/**", "src/bindings/**", "src/engine/**"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Finish `docs/js-binding.md`: the full jsb API, the §15.8 V8 handle and lifetime rules with examples, how to add a binding, error conventions, threading rules. Every function in `src/jsb/` and `src/bindings/` has a §15.1 doc comment.

## Acceptance criteria
- [ ] `tools/doclint/doclint.py --paths src/jsb src/bindings src/engine` reports nothing.
- [ ] `tools/doclint/mdlinks.py` passes on the doc.
- [ ] Every §15.8 rule is a heading with an example.

## Notes
Spec §9 M1 docs, §15.8.
