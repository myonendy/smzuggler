+++
id = "M1-T007b"
title = "jsb runtime on the engine's QuickJS context, plus the seam calls binding ports need"
milestone = 1
deps = ["M1-T004", "M1-T005", "M1-T006", "M1-T007"]
agent = "any"
owns = ["src/jsb/jsb.h", "src/jsb/jsb_adopt.h", "src/jsb/jsb_quickjs.c", "src/jsb/jsb_v8.cc", "src/jsb/README.md", "src/engine/js_engine.c", "src/engine/js_engine.h", "CMakeLists.txt", "tests/smz/test_jsb_adopt.c", "tests/smz/jsb_adopt.cmake", "tests/smz/test_jsb_values.c", "tests/smz/jsb_values.cmake", "tests/smz/test_js_engine_v8.c"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true  # OWNER 2026-11-04: only for the get_jsb assertion in tests/smz/test_js_engine_v8.c
+++
## Goal
Give the binding ports (M1-T014..T025) what DEC-M1-T014-JSB-ADOPT says they lack, without porting any binding:

1. **Adoption.** A private, transitional header `src/jsb/jsb_adopt.h` (QuickJS builds only; removed by M1-T027) declares `jsb_runtime *jsb_runtime_adopt(struct JSContext *js)`, which wraps an existing context without owning it, and `jsb_runtime *jsb_runtime_of(struct JSContext *js)`, which returns that runtime or NULL. `jsb_runtime_destroy` on an adopted runtime frees only jsb's own state and must run before the context is freed. `jsb_quickjs.c` finds its runtime through the QuickJS runtime opaque, not the context opaque, which `js_engine.c` owns.
2. **Engine.** `js_engine.c` adopts its context in `js_engine_init`, frees the runtime in `js_engine_shutdown` before the context, and `js_engine_get_jsb()` returns it. Update the comments in `js_engine.h` and `js_engine.c` that say it is NULL.
3. **Build.** A QuickJS jsb library (built from `src/jsb/jsb_quickjs.c` in both modes while `js_engine.c` exists) is linked by `outsider` and by every test target that builds `src/engine/js_engine.c`. Do this from the root `CMakeLists.txt` or a new `tests/smz/*.cmake` fragment; `tests/CMakeLists.txt` and existing test sources stay untouched. That library adds the PUBLIC compile definition `SMZ_JSB_ADOPT`, which guards every use of `jsb_adopt.h` outside `src/jsb/` (port cards' wrappers check it; the name has no `JS_` so their grep criterion stays clean). In v8 mode `smz_jsb` stays the V8 backend for `smz_js_engine_v8` users, which never see `SMZ_JSB_ADOPT`, and no target links both backends.
4. **Seam calls**, in `jsb.h` with a §15.1 comment each and implemented on V8 and QuickJS: `jsb_new_i32`, `jsb_new_f64`, `jsb_new_bool`, `jsb_new_null`, `jsb_new_str`, `jsb_new_str_len` (embedded NULs), `jsb_new_array` and `jsb_set_index`; `jsb_new_buffer_copy`; coercing reads `jsb_arg_truthy` (ToBoolean, never throws), `jsb_arg_to_i32` (ToInt32 with valueOf, as `JS_ToInt32`) and `jsb_arg_to_str` (String(), as `JS_ToCStringLen`, into memory the caller frees with `jsb_free_str`); `jsb_arg_to_u32` and `jsb_arg_to_f64` (ToUint32 / ToNumber with valueOf, as `JS_ToUint32` / `JS_ToFloat64`); `jsb_new_u32`; `jsb_is_array`, `jsb_get_index` and `jsb_val_to_f64` for reading plain-Array arguments element by element; `jsb_clear_exception` (drops a pending error, for the bindings that try one argument form and fall back to another or to a null/0 result); `jsb_set_fn_n` (sets the function's `length`); `jsb_throw_error` and `jsb_throw_internal` (QuickJS's InternalError, including its "out of memory" message; on V8 an Error whose `name` is "InternalError"). Before adding them, list every distinct `JS_*` / `JS_CFUNC_DEF` use in `src/bindings/` and make sure each has a jsb equivalent; add any missing one the same way. Names may change if `docs/js-binding.md` records why; the behaviours may not.

## Acceptance criteria
- [ ] New ctest `jsb_adopt` (both modes, QuickJS engine): after `js_engine_init`, `js_engine_get_jsb()` is non-NULL; a module defined through it with `jsb_runtime_setup` + `jsb_define_module` is visible to `js_engine_eval` scripts and its functions run; console output, rejection tracking and the watchdog behave as before; shutdown frees the adopted runtime before the context.
- [ ] New ctest `jsb_values` (both modes, both backends where built): each new seam call, including `length`, coercion of numbers/strings/objects with valueOf/Symbols, embedded NULs and error names.
- [ ] `test_jsb_core` (both modes), `jsb_buffers`, `jsb_callbacks`, `jsb_quickjs` and the whole ctest suite pass in both modes, unchanged.
- [ ] `git grep -n 'jsb_adopt.h' -- src ':!src/jsb' ':!src/engine/js_engine.c'` lists nothing yet (the port cards are its first users).
- [ ] `docs/js-binding.md` has a table mapping every distinct QuickJS call or macro used in `src/bindings/` (from `git grep -oh 'JS_[A-Za-z0-9_]*' -- src/bindings | sort -u`) to its jsb replacement, with none left unmapped.
- [ ] `docs/js-binding.md` documents adoption (replacing "has no way to adopt the engine's"), the new calls and their removal card (M1-T027 for adoption).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).
- [ ] The only change to an existing test is in `tests/smz/test_js_engine_v8.c`, `test_get_jsb_quickjs`: its NULL assertion becomes non-NULL, with its note, comment and the file's `Modified` header line updated. Every other line of that file and every other existing test file is unchanged (`git diff main --stat -- tests` lists only that file plus the new test files). Reviewers fail any other edit to an existing test.

## Notes
Spec §4.1; DEC-M1-T014-JSB-ADOPT, DEC-SETUP-008 / SI-11, DEC-SETUP-009. Shares `CMakeLists.txt` with M1-T012 and M1-T013, so forge never runs it alongside them.

Owner ruling 2026-11-04: `test_js_engine_v8.c` asserted the pre-adoption behaviour (`js_engine_get_jsb()` is NULL in QuickJS mode) that this card's goal changes, so this card owns that one assertion. `may_edit_tests` is card-wide in forge, so the acceptance criterion above is what limits it.
