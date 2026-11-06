+++
id = "M2-T009"
title = "M2 exit: §14 acceptance S1–S3 on dev-laptop"
milestone = 2
deps = ["M2-T005", "M2-T007"]
agent = "or-deepseek"
owns = ["docs/milestones/**"]
docs = ["docs/milestones/M2.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Run the §14 acceptance scenes S1–S3 on the owner's machine and write the M2 exit record. Title, Map 92 (Mode 7 floor correct) and menus within §14.5 tolerance; no WebGL errors over a 10-minute walk; reports use `gate=pass` or `gate=fail` honestly. Document the outcome in `docs/milestones/M2.md`.

## Acceptance criteria
- [ ] `docs/milestones/M2.md` records the S1–S3 runs with machine-state block (§14.4), per-scene summaries, and `gate=pass` or `gate=fail` from the analyser — never a hand-waved pass.
- [ ] Mode 7 floor is noted as correct or failed with evidence (screenshot path inside the scrubbed bundle).
- [ ] A 10-minute walk note records whether WebGL errors occurred (log excerpt, scrubbed).
- [ ] The run's outputs (logs, perf reports, frame-time CSVs, screenshots) are packaged with `tools/share/make_bundle.py` (M0-T023) without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] Reports pass `tools/privacy/scan.py --paths`.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §9 M2, §14. Runs on the owner's machine on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.

Owner 2026-11-05: Gate G1 is advisory. The owner treated G1 as advisory and started M2 on 2026-11-05 without an official three-run G1 pass. Workers must not invent timings and must not write a fake `G1: pass` / `G1: STOP` into docs. Do not require another idle G1 session.

Owner 2026-11-04/05 agent workflow: run `forge check` once when finished and once after each fix; mid-work run just the one relevant test; read files by line range instead of whole files unless under 100 lines.
