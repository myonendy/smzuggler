+++
id = "M1-T033"
title = "Gate G1: Map 92 JS time on V8"
milestone = 1
deps = ["M1-T032", "M1-T029"]
agent = "any"
owns = ["docs/perf/**", "tools/perf/buckets.toml"]
docs = ["docs/perf.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Measure Map 92 JS time per frame on V8 (warm-up 10 s, 120 s scored, 3 runs, worst counts) and apply Gate G1 (§6): if the mean is above 12 ms the result is **STOP**: the report says so, no M2 work starts, and the owner reassesses the plan. Write `docs/perf/m1-g1.md` with the numbers, the §6.1 attribution on V8 and the verdict.

## Acceptance criteria
- [ ] The report states `G1: pass` or `G1: STOP` with the mean, p95, p99 for each run and the machine-state block.
- [ ] The attribution uses `tools/perf/attribute.py` with V8 buckets (JIT code, interpreter, GC, builtins) added to buckets.toml in this card.
- [ ] The report passes `tools/privacy/scan.py --paths`.
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) and the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §6, §9 M1 exit. Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results. A STOP verdict is a valid, successful card outcome; never tune the measurement to pass.
