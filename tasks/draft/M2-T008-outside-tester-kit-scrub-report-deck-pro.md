+++
id = "M2-T008"
title = "Outside-tester kit: --scrub-report, deck-proxy scripts"
milestone = 2
deps = ["M2-T007"]
agent = "any"
owns = ["tools/tester/**", "src/smzuggler/**"]
docs = ["docs/testing.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
§14.8 kit and §14.9 deck-proxy; `--scrub-report` reuses `tools/share/make_bundle.py` (M0-T023) and refuses save files.

## Acceptance criteria
- [ ] Scrubbed report has no host/user/path (scan passes)
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/).

Spec §14.8, §14.9.
