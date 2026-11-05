+++
id = "M1-T031"
title = "Jitless run measurement"
milestone = 1
deps = ["M1-T029"]
agent = "or-deepseek"
owns = ["docs/perf/**"]
docs = ["docs/perf.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Run Map 92 with `--js-flags=--jitless` and with JIT, same route and timings as M0-T018, and write `docs/perf/m1-jitless.md` (JS time mean/p95/p99 for both, memory).

## Acceptance criteria
- [ ] The report has the §14.4 machine-state block and passes `tools/privacy/scan.py --paths`.
- [ ] Both runs use the same build and command line apart from the flag.
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §9 M1. Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.
