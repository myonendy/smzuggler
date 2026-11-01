+++
id = "M0-T018"
title = "Map 92 frame profile report tool for QuickJS mode"
milestone = 0
deps = ["M0-T017", "M0-T015", "U5", "M0-T023"]
agent = "any"
owns = ["docs/perf/**", "tools/perf/map92_report.py", "tests/tools/test_map92_report.py"]
docs = ["docs/perf.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Build the tool that turns a Map 92 profiling run into the report, so the owner can run it once on dev-laptop after merge (DEC-M0-T015-OWNER-RUN, extended to this card). The run itself is: start a new game (it lands on Map 92) in control mode, warm up 10 s, record 60 s, attribute with `tools/perf/attribute.py`, and cross-check against the runtime's own GameLoopStats. `tools/perf/map92_report.py` takes that run's attribution output, GameLoopStats log, frame-time CSV and machine-state snapshot and writes `docs/perf/m0-map92-profile.md` with the bucket table, mean/p95 frame time, JS update time, the §14.4 machine state block and a conclusions section for Gate G1 planning. The card commits the tool, its tests and `docs/perf/m0-map92-profile-recipe.md` (the exact commands); the real report is written by the owner's run.

## Acceptance criteria
- [ ] `tests/tools/test_map92_report.py` runs the tool on synthetic fixture inputs (fake data, no game) and asserts the generated report has the §14.4 machine-state fields (power source, profile, battery start/end, EPP), bucket percentages summing to 100 ± 0.5, mean/p95 frame time and JS update time matching the fixture, and that `tools/privacy/scan.py --paths` passes on it (no host name, user name or absolute path, even when the inputs contain them).
- [ ] The tool flags, in the report, a gap of more than 15% between the attributed JS time and GameLoopStats' JS time, and a test covers both the within-15% and the flagged case.
- [ ] `docs/perf/m0-map92-profile-recipe.md` lists the exact commands for the run (machine prep, run, attribute, report, then `tools/share/make_bundle.py`), so the owner can repeat it; a test checks every command it names exists in the tree.
- [ ] No real profile numbers are invented: the card does not commit `docs/perf/m0-map92-profile.md`. Nothing is posted anywhere by the card.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §6.1, §9 M0 exit ("profile report written"). Forge workers have no game window or game copy, so implement and test everything with synthetic inputs. The real run (dev-laptop, battery, `power-saver`, unplugged ≥ 2 min, battery ≥ 50%, test copy in `SMZUGGLER_GAME_DIR`, never the Steam folder) and the committed report are the owner's after-merge checkpoint in docs/FORGE.md.
