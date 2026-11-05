+++
id = "M1-T017"
title = "Port bind_image.c to jsb"
milestone = 1
deps = ["M1-T005", "M1-T006", "M1-T007", "M1-T007b"]
agent = "any"
owns = ["src/bindings/bind_image.c", "src/bindings/bind_image.h", "tests/smz/test_bind_image_jsb.c", "tests/smz/bind_image_jsb.cmake"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Rewrite `src/bindings/bind_image.c` on jsb (core, buffers and callbacks as needed), keeping its JS-visible API byte-for-byte (names, argument order, return shapes, errors). Near-mechanical: keep the structure so upstream diffs re-apply (§8.3).

Per DEC-M1-T014-JSB-ADOPT: define `__native_image` with `jsb_define_module` and the M1-T007b seam calls (`jsb_set_fn_n` with the old `JS_CFUNC_DEF` arity; the coercing `jsb_arg_to_*` reads wherever the old code used `JS_To*`). Add `bind_image_define(jsb_runtime *rt, ...)` (same other parameters) that defines the module, and keep `bind_image_register` with its current parameters, `JSContext *` as an opaque `struct JSContext *`, as a one-line transitional wrapper calling `bind_image_define(jsb_runtime_of(ctx), ...)` (from `src/jsb/jsb_adopt.h`), compiled only under `#ifdef SMZ_JSB_ADOPT`; M1-T026 removes it. Do not edit callers or existing tests. `loadImage`'s "path outside the game root returns null" fallback clears the pending error with the M1-T007b exception-clearing call, as now.

## Acceptance criteria
- [ ] tests/test_image_loader.c passes in both engine modes.
- [ ] New ctest `bind_image_jsb` (tests/smz, DEC-SETUP-009) calls `bind_image_define(js_engine_get_jsb(engine), ...)` in both modes (on the QuickJS engine in quickjs mode, on `smz_js_engine_v8` in v8 mode, so the port really runs on V8) and checks `__native_image`'s own property names, every function's `typeof` and `length`, the result shape of each function that runs without a device, and the error type and message for wrong arguments. Its expected values are the pre-port binary's, listed in the card report.
- [ ] tests/test_canvas2d.c and tests/integration/test_acceptance.c, test_gameplay.c and test_title_screen.c pass unchanged in both modes.
- [ ] `git grep -n 'JS_\|quickjs' -- src/bindings/bind_image.c src/bindings/bind_image.h` is empty.
- [ ] `--dump-surface` output for the `image` module is identical before and after (diff in the card report).
- [ ] `docs/js-binding.md` has a row for bind_image (functions, buffers, callbacks used).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] While M1 is in progress both engine modes build (`SMZ_JS_ENGINE=quickjs|v8`, from M1-T002) and the ctest suite passes in both; see DEC-SETUP-008 / SI-11 for the transitional QuickJS jsb backend.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1, §9 M1 ("all 12 bindings ported"). DEC-M1-T014-JSB-ADOPT: `--dump-surface` lists no native module rows and no `length` (DEC-M0-T009b), so the new ctest is what shows the API is unchanged; a `struct JSContext *` parameter is not a QuickJS call.
