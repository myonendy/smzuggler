+++
id = "M1-T003"
title = "jsb seam core API on V8"
milestone = 1
deps = ["M1-T002"]
agent = "any"
owns = ["src/jsb/**", "CMakeLists.txt", "tests/smz/test_jsb_core.c", "tests/smz/jsb_core.cmake"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `src/jsb/jsb.h` and `src/jsb/jsb_v8.cc` with the §4.1 core: `jsb_ctx`, `jsb_val` (handle-scoped), argument getters (`jsb_arg_count`, `_i32`, `_f64`, `_bool`, `_str` into a caller buffer, `_obj`), returns (`jsb_ret_undefined/_bool/_i32/_f64/_str/_val`), `jsb_throw_type`/`jsb_throw_range`, `jsb_new_object`, `jsb_set_fn`, `jsb_set_prop`, `jsb_get_prop`, and `jsb_define_module(ctx, name, init_fn)`. No V8 type leaks into jsb.h.

## Acceptance criteria
- [ ] `tests/smz/test_jsb_core.c` (pure C, via `tests/smz/jsb_core.cmake`) defines a module with functions covering each getter, return and throw, calls them from JS, and asserts values, type errors for wrong argument types, and missing-argument behaviour.
- [ ] `jsb.h` compiles as C11 with no C++ or V8 include (checked by a C test translation unit).
- [ ] `docs/js-binding.md` started: the API, ownership of strings, and §15.8 handle rules.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; handle-lifetime rules follow §15.8.
- [ ] While M1 is in progress both engine modes build (`SMZ_JS_ENGINE=quickjs|v8`, from M1-T002) and the ctest suite passes in both; see DEC-SETUP-008 / SI-11 for the transitional QuickJS jsb backend.

## Notes
Spec §4.1, §15.8.
