+++
id = "M7-T005"
title = "Deck confirmation pass"
milestone = 7
deps = []
agent = "any"
owns = ["docs/milestones/**"]
docs = ["docs/milestones/M7.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
On a borrowed Deck or Deck-equivalent; required before release (Q19).

## Acceptance criteria
- [ ] Report gate=confirm-pass from a steam-deck profile
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M7 is planned. Not scheduled (tasks/draft/).

Spec §14.8. Human-run; tester never named.
