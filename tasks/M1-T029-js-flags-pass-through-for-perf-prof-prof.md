+++
id = "M1-T029"
title = "--js-flags pass-through for --perf-prof, --prof and --trace-gc"
milestone = 1
deps = ["M1-T027"]
agent = "or-deepseek"
owns = ["src/main.c", "src/engine/js_engine_v8.cc", "tools/perf/**", "tests/smz/test_js_flags.c", "tests/smz/js_flags.cmake"]
docs = ["docs/perf.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
`--js-flags="..."` passes V8 flags (validated against an allow-list: `--perf-prof`, `--perf-basic-prof`, `--prof`, `--trace-gc`, `--jitless`, `--max-old-space-size`, `--stack-size`) before isolate creation; the flags hash feeds the code-cache key. `tools/perf/` gets a recipe script for `perf record` with `--perf-prof` and jitdump injection.

## Acceptance criteria
- [ ] Test: an allowed flag reaches V8 (`--trace-gc` output appears when GC is forced); an unknown flag is rejected with exit code 2; the code-cache key changes with the flags.
- [ ] docs/perf.md documents each flag.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1, §6.
