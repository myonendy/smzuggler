+++
id = "M0-T008"
title = "Markdown link and anchor check"
milestone = 0
deps = ["M0-T006"]
agent = "any"
owns = ["tools/doclint/mdlinks.py", "tests/tools/test_mdlinks.py", "README.md"]
docs = ["CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `tools/doclint/mdlinks.py`: checks every relative link and `#anchor` in tracked `*.md` files (README, ARCHITECTURE, docs/, module READMEs), with GitHub's heading-slug rules. External links are not fetched (offline). Exit 1 with file:line for each broken link.

## Acceptance criteria
- [ ] `tests/tools/test_mdlinks.py` covers a good file link, a missing file, a good anchor, a missing anchor, duplicate headings (`-1` suffix), links inside code blocks being ignored, and an external link being ignored.
- [ ] Running it on the repo passes, or each finding is fixed in this card (except under `docs/spec/`, which is read-only; findings there are listed in the card report).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §15.7.
