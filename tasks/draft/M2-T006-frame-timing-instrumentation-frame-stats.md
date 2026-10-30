+++
id = "M2-T006"
title = "Frame-timing instrumentation (--frame-stats)"
milestone = 2
deps = []
agent = "any"
owns = ["src/smzuggler/frame_stats.c", "src/smzuggler/frame_stats.h"]
docs = ["docs/perf.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Always-compiled frame work time (CPU + GL timer queries), present interval, missed vblanks, markers; JSON Lines + summary (§14.3).

## Acceptance criteria
- [ ] Unit tests for the summary maths
- [ ] Overhead < 0.2 ms/frame
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/).

Spec §14.3.
