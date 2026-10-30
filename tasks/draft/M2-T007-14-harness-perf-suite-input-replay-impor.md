+++
id = "M2-T007"
title = "§14 harness: --perf-suite, input replay, --import-fixture, analyser"
milestone = 2
deps = ["M2-T006"]
agent = "any"
owns = ["src/smzuggler/**", "tools/perf/**"]
docs = ["docs/perf.md", "docs/testing-fixtures.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Scripted scenes S1–S3 with 10 s warm-up / 120 s scored / 3 runs; fixtures from `$XDG_DATA_HOME/smzuggler-rt/fixtures/` (read-only); analyser with `gate` field (§14.4–14.7, §14.10).

## Acceptance criteria
- [ ] Missing/wrong-map fixture stops with a clear message
- [ ] Reports pass the privacy `--paths` scan
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/).

Spec §14.
