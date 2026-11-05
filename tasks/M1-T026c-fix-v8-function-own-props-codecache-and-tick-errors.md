+++
id = "M1-T026c"
title = "Fix V8 jsb_set_fn_n own properties, code-cache checks and nextTick errors"
milestone = 1
deps = []
agent = "or-deepseek"  # OWNER 2026-11-05: V8-mode follow-ups for DeepSeek; Claude/Codex review
owns = ["src/jsb/jsb_v8.cc", "src/jsb/README.md", "src/engine/js_engine_v8.cc", "tests/test_js_engine.c"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true  # OWNER 2026-11-05: only the .jsc / code-cache assertions inside tests/test_js_engine.c (test_jsc_cache_dir); no other test file, and do not loosen any bind_*_jsb or microtasks assertion
+++
## Goal
V8 13.6.233.8 is installed and a v8-mode build links, but ctest is 57/64. Close the three real gaps so V8 mode matches the QuickJS behaviour the existing tests already pin. Do **not** loosen any bind or microtasks assertion.

1. **`jsb_set_fn_n` own properties (bind_audio_jsb, bind_canvas2d_jsb, bind_filters_jsb, bind_video_jsb).** Those four fail because `Object.getOwnPropertyNames(fn)` on a V8 function from `jsb_set_fn_n` is `arguments,caller,length,name,prototype` instead of QuickJS/`JS_CFUNC_DEF`'s `length,name`. The root is `jsb_set_fn_n` in `src/jsb/jsb_v8.cc` (`v8::Function::New` without stripping the constructor prototype). Fix in the binding layer so every function installed by `jsb_set_fn` / `jsb_set_fn_n` has exactly the own properties `length` and `name` (writable/configurable/non-enumerable on the module slot stays as today). Prefer `v8::Function::New(..., v8::ConstructorBehavior::kThrow)` and/or a `FunctionTemplate` with `RemovePrototype`, or the equivalent that leaves no `prototype`/`arguments`/`caller` own properties. Do **not** edit any `tests/smz/test_bind_*_jsb.c`.

2. **`js_engine_on_v8` code-cache checks (`tests/test_js_engine.c`, `test_jsc_cache_dir`).** Four checks still look for QuickJS `.jsc` files (`…/A.jsc` under the cache dir, `…/__/….jsc`, and a default `.jsc` beside the script). V8 stores caches as `<cache_dir>/<sha256>.bin` via `js_codecache.h` (see M1-T012 / `js_engine_v8.cc`), keys by source text (not mtime / relative path), and **never** writes beside the script. Under `#ifdef SMZ_JS_ENGINE_V8` (already a PUBLIC define of `smz_js_engine_v8`, so `js_engine_on_v8` sees it), replace only those file-existence assertions with equal-strength V8 expectations: a `.bin` appears under the configured cache dir after a cached eval, nothing is written beside the script, a source edit is **not** masked by a preserved mtime (V8 keys by bytes), a `..` script path still lands inside the cache dir as a `.bin`, and a NULL cache dir never creates a sibling `.jsc`. QuickJS branches stay unchanged. Do **not** skip the tests. `git diff` on `tests/` must list only `tests/test_js_engine.c`.

3. **`microtasks` `test_throwing_tick_order`.** `assert(errors == 1)` fails on V8: an exception thrown inside a `process.nextTick` callback is not reaching the console-error callback that `capture_tick_error` registered (see `drain_ticks` / `report_exception` / `emit` in `js_engine_v8.cc`, from M1-T010). Fix the engine so a throwing tick still reports an `error`-level console line whose text contains the Error message (`tick failure regression`), `js_engine_execute_pending_jobs` still returns -1, and remaining ticks keep priority over promises. Do **not** edit `tests/smz/test_microtasks.c`.

Out of scope: `acceptance` / `build_pipeline_structure` (hard-coded `build/smzuggler-rt` path) is expected to fail in an out-of-tree build; leave it alone.

## Acceptance criteria
- [ ] Default QuickJS `forge check` (in-tree / the forge worktree build) stays green: every existing QuickJS ctest still passes, and no QuickJS-only source was weakened.
- [ ] A separate out-of-tree v8-mode configure/build/ctest passes every test except the known `acceptance` hard-coded-path failure. Configure with `-DSMZ_JS_ENGINE=v8 -DSMZ_V8_DIR=$HOME/.cache/smzuggler-dev/deps/v8/13.6.233.8` (links with `lld`; `cmake/V8.cmake` already requires `ld.lld`). State the build dir, the ctest summary, and the single remaining acceptance failure in the report.
- [ ] `Object.getOwnPropertyNames` on every function installed by `jsb_set_fn` / `jsb_set_fn_n` is exactly `length,name` on V8 (bind_audio/canvas2d/filters/video_jsb pass unchanged; `jsb_enum` still passes).
- [ ] The only edit under `tests/` is in `tests/test_js_engine.c`, and only inside `test_jsc_cache_dir`'s V8-specific assertions (`git diff main --stat -- tests` lists that file alone among tests). Reviewers fail any other test edit, any skipped test, or any loosened bind/microtasks assertion.
- [ ] `test_throwing_tick_order` passes on V8 without any change to `tests/smz/test_microtasks.c`.
- [ ] `docs/js-binding.md` notes (briefly) that `jsb_set_fn_n` on V8 installs non-constructable functions whose only own properties are `length` and `name`, and that the `js_engine_on_v8` code-cache checks assert the `.bin` layout. Grep; never open the file whole.
- [ ] Every changed file has its §12 `Modified` header line and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] CHANGELOG.md has this card's one line (AGENTS.md section 6).
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1, §15.1, §15.8. Follow-up to M1-T007 / T010 / T012 / T026b once the pinned V8 is installed. M1-T027 waits for it. V8 is at `$HOME/.cache/smzuggler-dev/deps/v8/13.6.233.8`; use that path for the out-of-tree v8 build in the acceptance criteria.
