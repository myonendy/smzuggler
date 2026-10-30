+++
id = "M1-T012"
title = "V8 code cache in the cache dir"
milestone = 1
deps = ["M1-T007"]
agent = "any"
owns = ["src/engine/js_codecache.cc", "src/engine/js_codecache.h", "src/engine/js_engine_v8.cc", "CMakeLists.txt", "tests/smz/test_codecache.c", "tests/smz/codecache.cmake"]
docs = ["docs/data-dirs.md", "docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Replace the `.jsc` cache in v8 mode with V8 code cache: `$XDG_CACHE_HOME/smzuggler-rt/v8cache/<sha256>.bin`, keyed by sha256 of the source + V8 version + the flag hash; produced after first run, consumed with `kConsumeCodeCache`, rejected caches deleted and regenerated.

## Acceptance criteria
- [ ] Test: first load creates the file; second load consumes it (`cachedData` accepted); changing the source, the flags or the version tag changes the key; a corrupted file is rejected and replaced; nothing is written next to the script.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.

## Notes
Spec §4.1, §5.6.
