+++
id = "M1-T026b"
title = "Share arg_is_array_buffer in jsb and hide jsb_set_fn_n functions from enumeration"
milestone = 1
deps = []
agent = "or-deepseek"  # OWNER 2026-11-05: small cleanup for DeepSeek; Claude/Codex review
owns = ["src/jsb/jsb.h", "src/jsb/jsb_quickjs.c", "src/jsb/jsb_v8.cc", "src/jsb/README.md", "src/bindings/**"]
docs = ["docs/js-binding.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Two small cleanups in the jsb seam, with no behaviour change seen from JS:

1. **Shared ArrayBuffer check.** `bind_image.c`, `bind_canvas2d.c`, `bind_renderer.c` and `bind_tilemap.c` each carry the same `static bool arg_is_array_buffer(jsb_ctx *ctx, int index, bool *out)` (an argument is an ArrayBuffer, not a TypedArray or DataView, because it has no `byteOffset`). Move it into the seam as one `jsb.h` call (e.g. `jsb_arg_is_array_buffer`) implemented in both backends (`jsb_quickjs.c` and `jsb_v8.cc`; a backend may use its engine's own ArrayBuffer test if the result is the same for every input, including a getter that throws), and make the four binders call it. Delete the four static copies.
2. **Non-enumerable functions.** `jsb_set_fn_n` (and `jsb_set_fn`, which calls it) currently assigns, so the functions come out enumerable, unlike `JS_CFUNC_DEF` and JS built-ins. Define them writable, configurable and non-enumerable in both backends: QuickJS with `JS_DefinePropertyValueStr(..., JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE)`, V8 with `DefineOwnProperty(..., v8::DontEnum)`. Then remove the per-binder setup scripts that only existed to restore that (the `Object.defineProperty(m, k, { enumerable: false })` snippets in `src/bindings/bind_*.c`). Where a snippet also changes non-function properties (e.g. one that walks `Object.getOwnPropertyNames(m)`), keep every module's resulting property descriptors exactly as they are today: narrow the snippet rather than change what JS sees.

## Acceptance criteria
- [ ] `git grep -n 'static bool arg_is_array_buffer' -- src` is empty and the four binders use the new jsb call, which has a §15.1 doc comment in `jsb.h`.
- [ ] New ctest (a new `tests/smz/test_jsb_enum.c` registered by a new `tests/smz/jsb_enum.cmake`, on every backend that is built, in the style of `tests/smz/jsb_values.cmake`): an object given functions with `jsb_set_fn` and `jsb_set_fn_n` has an empty `Object.keys`, each function's descriptor is `{writable: true, enumerable: false, configurable: true}`, `length` and `name` are unchanged, and the function can still be called, reassigned and deleted. It also covers the new ArrayBuffer call: true for an ArrayBuffer, false for a TypedArray, a DataView, a plain object and a number.
- [ ] Every existing test passes unchanged in both modes (no existing test file is edited), including the `Object.keys(__native_*).length` and descriptor checks in `tests/smz/test_bind_*_jsb.c`.
- [ ] No other behaviour changes: each `__native_*` module's own property names, values and descriptors are the same before and after (state how you checked in the report).
- [ ] `docs/js-binding.md` says `jsb_set_fn_n` defines non-enumerable functions and lists the new call; the notes that say bindings restore non-enumerability with a script are updated. Grep for them; never open the file whole.
- [ ] Every changed file has its §12 `Modified` header line and every new or changed function has a §15.1 doc comment.
- [ ] CHANGELOG.md has this card's one line (AGENTS.md section 6).
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.1, §15.1, §15.8. Follow-up to M1-T007b and the binding ports M1-T014..T025. It touches the same seam as M1-T027, so M1-T027 waits for it. V8 mode may not be buildable in your worktree yet (the prebuilt V8 is still being built); write the V8 side to match the QuickJS side and say in the report whether it was compiled.
