+++
id = "M2-T006"
title = "Frame-timing instrumentation (--frame-stats)"
milestone = 2
deps = []
agent = "or-deepseek"
owns = ["src/smzuggler/frame_stats.c", "src/smzuggler/frame_stats.h", "src/main.c", "src/engine/game_loop.c", "CMakeLists.txt", "tests/smz/test_frame_stats.c", "tests/smz/frame_stats.cmake"]
docs = ["docs/perf.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Always-compiled frame-timing instrumentation enabled with `--frame-stats=<file>` (§14.3): frame work time (CPU from frame start through swap, plus GPU time from `GL_TIME_ELAPSED` or `EXT_disjoint_timer_query` on GLES), present interval, missed vblanks (present interval > 1.5× refresh), and markers for scene changes / map transfer / save-load. Output JSON Lines (one record per frame) plus a summary of mean, p50, p95, p99, p99.9, max and frames-over-budget. Overhead target < 0.2 ms/frame.

## Acceptance criteria
- [ ] New ctest `frame_stats`: summary maths (percentiles, missed-vblank count, over-budget count) match a fixed hand-computed fixture of frame records.
- [ ] `--frame-stats=<file>` writes JSON Lines and a summary; without the flag the cost stays in the always-compiled stubs/no-op path (no file written).
- [ ] Marker API (or hook points) records scene-change / transfer / save-load windows so a later analyser can exclude them (§14.3, §14.5).
- [ ] Overhead measurement is documented in `docs/perf.md` and is < 0.2 ms/frame on a local microbench the test or tool runs (or the card reports the measured number and fails the criterion if ≥ 0.2 ms).
- [ ] Reports never include host name, user name or home paths (§16); paths are scrubbed to `~` where logged.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §14.3. Shares `src/main.c` with M2-T005 and M2-T007; forge serialises on overlapping owns. GL timer queries need a GL context: if none is available, CPU-only timing still works and the test notes GPU time as unavailable rather than inventing numbers.

Owner 2026-11-04/05 agent workflow: run `forge check` once when finished and once after each fix; mid-work run just the one relevant test (`ctest -R frame_stats`); read files by line range instead of whole files unless under 100 lines.
