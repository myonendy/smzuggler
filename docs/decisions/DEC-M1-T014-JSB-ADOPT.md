<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Ruling for the blocked binding-port cards M1-T014..T025 (spec section 4.1).
-->
## DEC-M1-T014-JSB-ADOPT — binding ports get a jsb runtime on the engine's own context first

**Context.** M1-T014 (bind_platform.c) blocked after four attempts with no code changed, and
M1-T015 (bind_input.c) stopped at the same point. The workers were right: the port cannot be
done inside the card. Three things are missing, none of them in the port cards' `owns`:

1. No jsb runtime shares the game's globals. The `outsider` binary and the 23 test targets
   that build `src/engine/js_engine.c` run on QuickJS in *both* `SMZ_JS_ENGINE` modes until
   M1-T027, but `js_engine_get_jsb()` returns NULL there: `jsb_quickjs.c` only makes its own
   `JSContext` (`jsb_runtime_create`), and both it and `js_engine.c` claim the context's
   opaque pointer. In v8 mode `smz_jsb` is the V8 backend, which `outsider` does not link
   and could not use with a QuickJS engine anyway.
2. The seam lacks calls the bindings use to keep their JS API byte-for-byte: primitive
   values and arrays to put in result objects (`JS_NewInt32`/`NewFloat64`/`NewBool`/
   `NewString(Len)`/`NewArray`/`SetPropertyUint32`, `JS_NULL`), JS coercion of arguments
   (`JS_ToBool`, `JS_ToInt32`, `JS_ToCString(Len)`; jsb's getters throw on a wrong type
   instead), a function `length` (`JS_CFUNC_DEF`'s arity; `jsb_set_fn` has none), a plain
   / internal error (`JS_ThrowInternalError`), and a copied ArrayBuffer.
3. Callers and tests pass a `JSContext *` to `bind_*_register` (`src/main.c`,
   `src/smzuggler/dump_surface.c`, `tests/integration/test_acceptance.c`,
   `tests/test_display.c`, `tests/test_storage.c`, `tests/smz/test_process_shim.c`), and port
   cards may not edit tests. M1-T026 ports those tests later.

M1-T009 and M1-T011 did not hit this because they only call *into* JS through `js_engine.h`;
bindings must *define* native functions in the game's global object.

**Decision.**
- New card **M1-T007b** adds the missing pieces before any binding port: a transitional,
  QuickJS-only `jsb_runtime_adopt()` / `jsb_runtime_of()` in a private header
  `src/jsb/jsb_adopt.h`; `js_engine.c` adopts its context at init so `js_engine_get_jsb()`
  is non-NULL in every build that runs QuickJS; a QuickJS jsb library linked by everything
  that builds `js_engine.c`, in both modes; and the seam calls in item 2, on both backends,
  each tested in both modes. jsb finds its runtime through the QuickJS *runtime* opaque (unused
  by `js_engine.c`), never the context opaque.
- Port cards M1-T014..T025 depend on M1-T007b. Each defines its module in a new
  `bind_<x>_define(jsb_runtime *rt, ...)` and keeps its old
  `bind_<x>_register(struct JSContext *ctx, ...)` as a one-line transitional wrapper,
  compiled only under `#ifdef SMZ_JSB_ADOPT` (the macro name keeps the grep criterion clean), that calls it with `jsb_runtime_of(ctx)`. That parameter is an opaque
  pointer, not a QuickJS call; the card's grep criterion stays as written. No caller or test
  changes in port cards.
- Port cards add one new `tests/smz/` test (DEC-SETUP-009) that calls `bind_<x>_define` on
  `js_engine_get_jsb()` (QuickJS engine in quickjs mode, `smz_js_engine_v8` in v8 mode) and
  checks the module's shape, every function's `length` and its errors, because `--dump-surface` does not
  list native modules or `length` (DEC-M0-T009b) and cannot show the API is unchanged by itself.
- M1-T026 switches `src/main.c` and `src/smzuggler/dump_surface.c` to `js_engine_get_jsb()`
  and deletes the wrappers when it ports the tests.

**Why.** It is the smallest change that lets each binding port stay one near-mechanical card
(spec section 8.3) with both modes green and no test edited, and it keeps all QuickJS knowledge in
`src/jsb/` and `src/engine/js_engine.c`, which M1-T027 deletes. It depends on DEC-SETUP-008
(the transitional QuickJS backend, SI-11), so approving this ruling also approves that one.

**Rejected alternatives.** Widening each port card's `owns` to the seam, engine, callers and
tests: twelve cards would edit the same files, overlap-block each other and each re-solve
adoption. A single big-bang port of all 12 bindings (M1-T004's fallback): no green tests
between bindings. Relaxing the grep or "byte-for-byte" criteria: that weakens a gate.

Supersedes the worker-written `DEC-M1-T014.md` and the SI-16 row on the unmerged
`forge/M1-T014` branch. Owner approved (2026-11-04), together with DEC-SETUP-008. No gate is relaxed.
