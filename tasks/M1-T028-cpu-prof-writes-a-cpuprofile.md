+++
id = "M1-T028"
title = "--cpu-prof writes a .cpuprofile"
milestone = 1
deps = ["M1-T027"]
agent = "any"
owns = ["src/engine/js_profiler.cc", "src/engine/js_profiler.h", "src/main.c", "CMakeLists.txt", "tests/smz/test_cpu_prof.c", "tests/smz/cpu_prof.cmake"]
docs = ["docs/perf.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
`--cpu-prof[=FILE]` starts V8's CpuProfiler at boot and writes a Chrome DevTools `.cpuprofile` on exit (default under `$XDG_STATE_HOME/smzuggler-rt/profiles/`), with script URLs game-relative.

## Acceptance criteria
- [ ] Test runs a small script with the flag and asserts the file is valid JSON with `nodes`, `samples`, `timeDeltas`, a node for the test's function, and no absolute path.
- [ ] docs/perf.md documents it.
- [ ] `.cpuprofile` files are collected by `tools/share/make_bundle.py` (extend its file list in this card if needed).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.

## Notes
Spec §9 M1.
