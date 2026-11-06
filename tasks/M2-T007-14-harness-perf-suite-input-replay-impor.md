+++
id = "M2-T007"
title = "§14 harness: --perf-suite, input replay, --import-fixture, analyser"
milestone = 2
deps = ["M2-T006"]
agent = "or-deepseek"
owns = ["src/smzuggler/**", "src/main.c", "tools/perf/**", "tests/smz/test_perf_suite_args.c", "tests/smz/perf_suite_args.cmake", "tests/tools/test_perf_analyser.py"]
docs = ["docs/perf.md", "docs/testing-fixtures.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Build the §14 harness around `--frame-stats`: `--perf-suite` runs scripted scenes with **10 s warm-up / 120 s scored / 3 runs** (worst counts); control-mode input replay walks a fixed route; `--import-fixture` loads fixtures from `$XDG_DATA_HOME/smzuggler-rt/fixtures/` (read-only); the analyser emits reports with a `gate` field (`pass`/`fail`/`confirm-pass`/`confirm-fail`/`extra`/`proxy` per §14.7). Cover S1–S3 scene scripts and the fixture recipe (§14.10). Missing or wrong-map fixtures must stop with a clear message.

## Acceptance criteria
- [ ] `--import-fixture` refuses a missing fixture and a fixture whose map id is not the one the scene requires, with a clear non-zero exit and message (no game content is shipped).
- [ ] Analyser unit tests (`tests/tools/test_perf_analyser.py`): given a fixed JSON Lines input, the summary and `gate` field match expectations for pass and fail cases under §14.5 rules 1–4.
- [ ] New ctest or tool test `perf_suite_args`: CLI parsing for `--perf-suite`, `--import-fixture` and `--frame-stats` rejects bad combinations with a clear message.
- [ ] Reports pass `tools/privacy/scan.py --paths`.
- [ ] A dry harness run's outputs (logs, perf reports, frame-time CSVs, screenshots if any) are packaged with `tools/share/make_bundle.py` (M0-T023); the bundle is written without a privacy hit; the bundle path and its README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] `docs/testing-fixtures.md` has the fixture-save recipe (§14.10) with no personal paths.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §14.4–14.7, §14.10. Full S1–S3 gating runs on battery are M2-T009; this card delivers the harness and tests it without inventing gate=pass numbers. GUI steps use the test copy in `SMZUGGLER_GAME_DIR`, never the Steam folder; if a window cannot be opened, stop and report `BLOCKED: needs dev-laptop GUI run`.

Owner 2026-11-04/05 agent workflow: run `forge check` once when finished and once after each fix; mid-work run just the one relevant test; read files by line range instead of whole files unless under 100 lines.
