+++
id = "M0-T017"
title = "perf attribution tool (§6.1)"
milestone = 0
deps = []
agent = "any"
owns = ["tools/perf/**", "tests/tools/test_attribute.py", "tests/tools/fixtures/perf/**"]
docs = ["docs/perf.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `tools/perf/attribute.py`: reads `perf script` output (with call graphs) and attributes each sample to the §6.1 buckets (QuickJS interpreter, GC, shim JS, game JS, Canvas2D, renderer/GPU submit, audio, video, libc/other) by DSO and symbol rules kept in `tools/perf/buckets.toml`. Writes JSON and a Markdown table with self and total percentages. Strips paths to basenames.

## Acceptance criteria
- [ ] `tests/tools/test_attribute.py` feeds synthetic `perf script` text (in `tests/tools/fixtures/perf/`, hand-written, no real profile) and asserts exact bucket totals, the handling of `[unknown]` frames and kernel samples, and that no absolute path appears in the output.
- [ ] `docs/perf.md` explains how to record (`perf record -g --call-graph dwarf`), how to attribute, and the bucket rules.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §6.1. The tool must not need root. Output is run through the privacy `--paths` scan before it is shared.
