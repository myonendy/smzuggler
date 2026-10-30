+++
id = "M0-T018"
title = "Map 92 frame profile report in QuickJS mode"
milestone = 0
deps = ["M0-T017", "M0-T015", "U5", "M0-T023"]
agent = "any"
owns = ["docs/perf/**"]
docs = ["docs/perf.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Record and attribute a perf profile of the Map 92 frame on the current QuickJS build: start a new game (it lands on Map 92) in control mode, let it warm up 10 s, record 60 s, attribute with `tools/perf/attribute.py`, and cross-check against the runtime's own GameLoopStats. Write `docs/perf/m0-map92-profile.md` with the bucket table, mean/p95 frame time, JS update time, the machine state block of §14.4, and the conclusions §6 needs for Gate G1 planning.

## Acceptance criteria
- [ ] The report contains the §14.4 machine-state fields (power source, profile, battery start/end, EPP) and no host name, user name or absolute path (`tools/privacy/scan.py --paths` passes on it).
- [ ] Bucket percentages sum to 100 ± 0.5 and match GameLoopStats' JS time within 15% (or the gap is explained).
- [ ] The exact commands used are in the report so the run can be repeated.
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §6.1, §9 M0 exit ("profile report written"). Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.
