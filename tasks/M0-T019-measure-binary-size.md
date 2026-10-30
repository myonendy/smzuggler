+++
id = "M0-T019"
title = "Measure binary size"
milestone = 0
deps = []
agent = "any"
owns = ["tools/perf/binsize.py", "tests/tools/test_binsize.py", "docs/perf/binary-size.md"]
docs = ["docs/perf.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `tools/perf/binsize.py BUILD_DIR`: reports the stripped size of `smzuggler-rt`, its shared libraries and the installed shims, and a per-dependency breakdown from the static archives (QuickJS, SDL3, SoLoud, Effekseer, shader tools). Write the result to `docs/perf/binary-size.md` as the M0 baseline.

## Acceptance criteria
- [ ] `tests/tools/test_binsize.py` runs the tool on a tiny fake build dir (generated object files) and asserts the totals.
- [ ] `docs/perf/binary-size.md` lists the release-build numbers with the build flags used.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §9 M0 ("Measure binary size"), §11 Q5/Q8 context.
