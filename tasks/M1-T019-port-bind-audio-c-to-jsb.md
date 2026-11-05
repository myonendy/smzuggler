+++
id = "M1-T019"
title = "Port bind_audio.c to jsb"
milestone = 1
deps = ["M1-T005", "M1-T006", "M1-T007", "M1-T007b"]
agent = "any"
owns = ["src/bindings/bind_audio.c", "src/bindings/bind_audio.h", "tests/smz/test_bind_audio_jsb.c", "tests/smz/bind_audio_jsb.cmake"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Rewrite `src/bindings/bind_audio.c` on jsb (core, buffers and callbacks as needed), keeping its JS-visible API byte-for-byte (names, argument order, return shapes, errors). Near-mechanical: keep the structure so upstream diffs re-apply (§8.3).

Per DEC-M1-T014-JSB-ADOPT: define `__native_audio` with `jsb_define_module` and the M1-T007b seam calls (`jsb_set_fn_n` with the old `JS_CFUNC_DEF` arity; the coercing `jsb_arg_to_*` reads wherever the old code used `JS_To*`). Add `bind_audio_define(jsb_runtime *rt, ...)` (same other parameters) that defines the module, and keep `bind_audio_register` with its current parameters, `JSContext *` as an opaque `struct JSContext *`, as a one-line transitional wrapper calling `bind_audio_define(jsb_runtime_of(ctx), ...)` (from `src/jsb/jsb_adopt.h`), compiled only under `#ifdef SMZ_JSB_ADOPT`; M1-T026 removes it. Do not edit callers or existing tests. Keep every RangeError message as is.

## Acceptance criteria
- [ ] tests/test_audio.c passes in both engine modes.
- [ ] New ctest `bind_audio_jsb` (tests/smz, DEC-SETUP-009) calls `bind_audio_define(js_engine_get_jsb(engine), ...)` in both modes (on the QuickJS engine in quickjs mode, on `smz_js_engine_v8` in v8 mode, so the port really runs on V8) and checks `__native_audio`'s own property names, every function's `typeof` and `length`, the result shape of each function that runs without a device, and the error type and message for wrong arguments. Its expected values are the pre-port binary's, listed in the card report.
- [ ] tests/test_webaudio.c, tests/integration/test_acceptance.c and test_gameplay.c pass unchanged in both modes.
- [ ] `git grep -n 'JS_\|quickjs' -- src/bindings/bind_audio.c src/bindings/bind_audio.h` is empty.
- [ ] `--dump-surface` output for the `audio` module is identical before and after (diff in the card report).
- [ ] `docs/js-binding.md` has a row for bind_audio (functions, buffers, callbacks used).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] While M1 is in progress both engine modes build (`SMZ_JS_ENGINE=quickjs|v8`, from M1-T002) and the ctest suite passes in both; see DEC-SETUP-008 / SI-11 for the transitional QuickJS jsb backend.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1, §9 M1 ("all 12 bindings ported"). DEC-M1-T014-JSB-ADOPT: `--dump-surface` lists no native module rows and no `length` (DEC-M0-T009b), so the new ctest is what shows the API is unchanged; a `struct JSContext *` parameter is not a QuickJS call.
