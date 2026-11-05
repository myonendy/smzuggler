<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-11-01: add persistent callbacks.
Modified 2026-11-01: document the V8 script watchdog.
Modified 2026-11-01: document how the game loop calls into JS.
Modified 2026-11-01: document the error handler's js_engine.h path.
Modified 2026-11-01: document microtask checkpoints and nextTick order.
Modified 2026-11-04: document the V8 code cache.
Modified 2026-11-04: document the bind_io port and parity checks.
Modified 2026-11-04: document main thread stack sizing.
Modified 2026-11-04: document the font binding port and buffer validation.
How native C bindings talk to the JS engine through jsb.h, and the handle rules.
-->
# JS binding: the `jsb` seam

`src/jsb/jsb.h` is the C API that native bindings (`src/bindings/`) use to expose
`__native_*` modules to the shims. It replaces direct QuickJS calls so that Outsider's
bindings move to V8 by near-mechanical rewriting (spec §4.1). It is **not** an engine
abstraction: its one real implementation is `src/jsb/jsb_v8.cc`.

Status: core API (M1-T003), zero-copy buffers (M1-T005) and persistent callbacks
(M1-T006). The library target is `smz_jsb` in both engine
modes; `jsb.h` itself is plain C11 and includes no C++ or V8 header (checked by
`tests/smz/test_jsb_header.c` in both engine modes).

## Transitional QuickJS backend (removed by M1-T027)

`src/jsb/jsb_quickjs.c` implements `jsb.h` on QuickJS-ng, and `smz_jsb` is built from it
when `SMZ_JS_ENGINE=quickjs` (the default). It exists only so bindings can be ported to
`jsb` one at a time while the game still runs on QuickJS and both engine modes stay green
(docs/SPEC_ISSUES.md SI-11, DEC-SETUP-008). **Card M1-T027 deletes it** together with
QuickJS. Rules while it lives:

- It must behave exactly like `jsb_v8.cc`: the unchanged `tests/smz/test_jsb_core.c` runs
  against it as ctest `jsb_core` in quickjs mode (`tests/smz/jsb_core_quickjs.cmake`).
  Any new jsb call lands in both backends in the same change.
- It adds no API of its own; nothing outside `src/jsb/` may include `quickjs.h` through it.
- `jsb_runtime_create` ignores `argv0` and `icu_data_path` (QuickJS has no ICU).
- Handles: each `jsb_val` owns one QuickJS reference, kept in fixed chunks owned by the
  call's `jsb_ctx` and freed when the call returns, so the "valid only during the call"
  rule below holds exactly as with V8 `Local`s. A native function is registered through a
  per-runtime table indexed by the QuickJS function's 16-bit magic, so a runtime holds at
  most 32,767 distinct `jsb_fn`s.
- Known difference: a string argument holding a lone UTF-16 surrogate is read out in
  QuickJS's encoding of it, where V8 writes U+FFFD.

## Adoption (transitional, removed by M1-T027)

The game runs on `js_engine.c` (QuickJS) in both modes until M1-T027, so binding ports
need a jsb runtime on *its* context (DEC-M1-T014-JSB-ADOPT). The private header
`src/jsb/jsb_adopt.h` declares `jsb_runtime_adopt(JSContext *)`, which wraps a context
without owning it (`js_engine_init` calls it; `js_engine_get_jsb()` returns it), and
`jsb_runtime_of(JSContext *)`, which the port cards' `bind_<x>_register` wrappers use.
jsb keeps its runtime in the QuickJS **runtime** opaque; the context opaque stays
`js_engine.c`'s. `jsb_runtime_destroy` on an adopted runtime frees only jsb's own
references and must run before the context is freed (`js_engine_shutdown` does).

`smz_jsb_quickjs` is built in both modes and linked by `outsider` and by every test
target that builds `js_engine.c` (the root `CMakeLists.txt` scans the `tests` targets).
Its PUBLIC `SMZ_JSB_ADOPT` guards every use of `jsb_adopt.h` outside `src/jsb/`. In
quickjs mode `smz_jsb` is an alias of it; in v8 mode `smz_jsb` is the V8 backend
(via `smz_js_engine_v8`, which never sees `SMZ_JSB_ADOPT`). No target links both.

## Seam calls for binding ports (M1-T007b)

In `jsb.h` on both backends (test `jsb_values`); they stay after M1-T027. Coercing reads
apply the ECMAScript conversions (valueOf runs; a Symbol is a TypeError).
`jsb_clear_exception` never drops a watchdog termination; on V8 each native call runs
in a `TryCatch` that it resets. On V8 `jsb_throw_internal` makes an `Error` whose own
`name` is `"InternalError"`. Added beyond the card's list: `jsb_val_to_i32/u32`
(bind_tilemap.c coerces elements), `jsb_val_type` and `jsb_engine_version`.

| QuickJS in `src/bindings/` | jsb |
| --- | --- |
| `JS_CFUNC_DEF`, `JS_SetPropertyFunctionList`, `JS_NewCFunction` | `jsb_set_fn_n` (arity = `length`) |
| `JS_GetGlobalObject` | `jsb_define_module` |
| `JS_NewObject`, `JS_SetPropertyStr`, `JS_GetPropertyStr` | `jsb_new_object`, `jsb_set_prop`, `jsb_get_prop` |
| `JS_NewAtomLen`, `JS_SetProperty`, `JS_FreeAtom` | `jsb_set_prop` on a NUL-terminated key copy |
| `JS_NewArray`, `JS_IsArray` | `jsb_new_array`, `jsb_is_array` |
| `JS_SetPropertyUint32`, `JS_GetPropertyUint32` | `jsb_set_index`, `jsb_get_index` |
| `JS_NewInt32`, `JS_NewUint32`, `JS_NewFloat64` | `jsb_new_i32`, `jsb_new_u32`, `jsb_new_f64` (or `jsb_ret_*`) |
| `JS_NewBool`, `JS_TRUE`, `JS_FALSE` | `jsb_new_bool`, `jsb_ret_bool` |
| `JS_NULL`, `JS_UNDEFINED` | `jsb_new_null`, `jsb_ret_undefined` |
| `JS_NewString`, `JS_NewStringLen` | `jsb_new_str`, `jsb_new_str_len` |
| `JS_NewArrayBufferCopy` | `jsb_new_buffer_copy` |
| `JS_GetArrayBuffer`, `JS_GetTypedArrayBuffer` | `jsb_arg_bytes` |
| `JS_ToBool` | `jsb_arg_truthy` |
| `JS_ToInt32`, `JS_ToUint32`, `JS_ToFloat64` | `jsb_arg_to_i32/u32/f64`; elements `jsb_val_to_i32/u32/f64` |
| `JS_ToCString`, `JS_ToCStringLen`, `JS_FreeCString` | `jsb_arg_to_str`, `jsb_free_str` |
| `JS_IsNull`, `JS_IsUndefined` | `jsb_val_type` |
| `JS_IsException`, `JS_EXCEPTION` | false/NULL from the jsb call, then return |
| `JS_GetException` | `jsb_clear_exception` |
| `JS_FreeValue` | none: handles are call-scoped (§15.8) |
| `JS_ThrowTypeError`, `JS_ThrowRangeError` | `jsb_throw_type`, `jsb_throw_range` |
| `JS_ThrowInternalError`, `JS_ThrowOutOfMemory` | `jsb_throw_internal` (`"out of memory"`) |
| `JS_GetVersion` | `jsb_engine_version` |

## Ported bindings

Each port defines its module in `bind_<x>_define(jsb_runtime *rt, ...)`; the old
`bind_<x>_register(struct JSContext *, ...)` stays as a wrapper under `SMZ_JSB_ADOPT`
until M1-T026. A `tests/smz/test_bind_<x>_jsb.c` test pins the JS API to the pre-port
binary's in both engine modes.

| Binding | Module and functions (`length`) | jsb calls used | Buffers | Callbacks |
| --- | --- | --- | --- | --- |
| `bind_io.c` (M1-T016) | `__native_io`: `readFileSync` (1), `readFileBinary` (1), `writeFileSync` (2), `writeFileBinary` (2), `mkdirSync` (1), `existsSync` (1), `isDirSync` (1), `readdirSync` (1), `unlinkSync` (1), `renameSync` (2), `getGameRoot` (0), `resolvePath` (1) | `jsb_define_module`, `jsb_set_fn_n`, `jsb_arg_count/to_str/val/bytes`, `jsb_free_str`, `jsb_get_prop`, `jsb_val_type`, `jsb_clear_exception`, `jsb_new_array/str/str_len/buffer_copy`, `jsb_set_index`, `jsb_ret_val/bool/str/undefined`, `jsb_throw_type/internal`, `jsb_runtime_setup/eval` | Reads copy native bytes into engine-owned ArrayBuffers; writes borrow bytes only during the call and accept ArrayBuffers only | none |
| `bind_input.c` (M1-T015) | `__native_input`: `pollEvents` (0), `getGamepad` (1), `getGamepadCount` (0) | `jsb_define_module`, `jsb_set_fn_n`, `jsb_new_object/array/str/i32/f64/bool/null`, `jsb_set_prop`, `jsb_set_index`, `jsb_arg_count`, `jsb_arg_to_i32` (`getGamepad` coerces its index), `jsb_ret_val/i32`, `jsb_runtime_eval` | none | none |
| `bind_font.c` (M1-T018) | `__native_font`: `loadFont` (2), `loadFontFile` (2), `hasFont` (1), `getMetrics` (2), `measureText` (3), `fontCount` (0), `fontNames` (0) | `jsb_define_module`, `jsb_runtime_setup/eval`, `jsb_set_fn_n`, `jsb_arg_count`, `jsb_arg_to_str/f64`, `jsb_free_str`, `jsb_arg_bytes`, `jsb_clear_exception`, `jsb_throw_type`, `jsb_new_object/array/str/f64/null`, `jsb_set_prop/index`, `jsb_ret_val/bool/i32/f64` | Borrowed ArrayBuffer bytes during `loadFont`; both font caches copy them before return. File bytes are owned and freed by the binding. | none |

`jsb_set_fn_n` assigns functions, so they come out enumerable, while `JS_CFUNC_DEF` made
them non-enumerable. `bind_input_define` restores that with a one-line
`Object.defineProperty` script after the module exists, so `Object.keys(__native_input)`
stays empty as before.

`bind_io_define` restores the same property attributes for `__native_io`.
The `bind_io_jsb` ctest checks all twelve names and arities above, string and
ArrayBuffer round trips (including embedded NULs and empty buffers), directory
arrays, boolean filesystem results, undefined write results, coercion, and exact
binding error names/messages. It uses the adopted engine in quickjs mode and
`smz_js_engine_v8` in v8 mode. Engine-generated Symbol conversion messages retain
each engine's spelling. No JS handle or borrowed buffer survives a native call.

The retained I/O parity test records these pre-port expectations:

| Check | Expected value |
| --- | --- |
| Own names and arities | The twelve functions in the `bind_io.c` row, in that order; all have `typeof === "function"` |
| Function properties | Writable, configurable, non-enumerable; `Object.keys(__native_io)` is empty |
| Text round trip | `h\u00e9\u0000x` has length 4, with character codes 233 and 0 at indices 1 and 2 |
| Binary round trip | ArrayBuffer of length 3, bytes `1,2,255`; empty ArrayBuffers round-trip with length 0 |
| Writes and directory reads | Writes return `undefined`; `readdirSync` returns an array of entry names |
| Filesystem predicates and mutations | Boolean results, including `false` for missing paths in predicates, rename and unlink |
| Path results | `getGameRoot` returns the configured root; `resolvePath('a/b%20c.txt')` returns that root plus `/a/b c.txt` |
| Missing arguments (functions with nonzero arity) | TypeError: `<function> requires a path argument`, except writes require `path and data arguments` and rename requires `oldPath and newPath arguments` |
| Invalid binary data | `TypeError: writeFileBinary: second argument must be an ArrayBuffer` |
| Missing read targets | InternalError: `failed to read file: <resolved path>`, `failed to read binary file: <resolved path>`, or `failed to read directory: <resolved path>` |

`bind_font_define` also restores non-enumerable function properties. Its setup script
captures the intrinsic ArrayBuffer byte-length getter and wraps `loadFont` with a
non-constructible method of the same name and arity. The getter checks the actual
ArrayBuffer brand: typed arrays, DataView and spoofed objects keep the old binding's
TypeError instead of passing the seam's broader `jsb_arg_bytes` reader. Family
ToString runs first, preserving conversion exceptions and ordering. No JS runs
while the C callback borrows bytes; its strings are owned copies freed on every
exit. Result handles remain inside their native call scope.

The `bind_font_jsb` test uses an original synthetic sfnt with one empty glyph,
a 1000-unit em and fixed metrics. It checks successful loading, metrics, widths
and names without a graphics device or an installed font. Missing-font cases,
all seven function descriptors/arities and binding-authored error messages are
captured from the pre-port binding; `--dump-surface` has no native font rows.
The M1-T018 pre/post `--dump-surface` diff is empty for the complete inventory
(and therefore for `font`). The pre-port probe recorded:

| Check | Expected value |
| --- | --- |
| Own names, in order | `loadFont,loadFontFile,hasFont,getMetrics,measureText,fontCount,fontNames` |
| Function arities, in order | `2,2,1,2,3,0,0`; each has `typeof === "function"` |
| Function property descriptors | writable and configurable, non-enumerable |
| Missing font | `loadFont` / `loadFontFile` / `hasFont`: `false`; `getMetrics`: `null`; `fontCount`: `0`; `fontNames`: `[]` |
| Missing-family width, size `"10"`, text `12345` | `30.000001907348633` |
| Synthetic font | load / has: `true`; count `1`; names `["Synthetic"]`; width of `abc` at 1000 pixels: `1500` |
| Synthetic metrics at 1000 pixels | `{"ascent":800,"descent":-200,"lineGap":100,"lineHeight":1100}` |
| Missing arguments (TypeError messages) | `loadFont requires family and ArrayBuffer`; `loadFontFile requires family and path`; `hasFont requires family name`; `getMetrics requires family and pixelSize`; `measureText requires family, pixelSize, text` |
| Invalid buffer (TypeError message) | `loadFont: second arg must be ArrayBuffer` (including views and a spoofed ArrayBuffer tag) |

User-thrown conversion errors keep their original type and message. Strings
longer than 4096 bytes remain accepted, and an embedded NUL in a family still
terminates its native name. All expectations also run against the port.


## API

### Host side
| Call | Purpose |
| --- | --- |
| `jsb_set_flags(flags)` | Engine flags such as `--expose-gc`, process-wide. Only before the first runtime (V8 freezes its flags); false afterwards. The QuickJS backend honours `--expose-gc` and ignores V8-only flags. |
| `jsb_runtime_create(argv0, icu_data_path)` | One isolate and one context. The first call starts V8 for the process. NULL on failure, reason on stderr. |
| `jsb_runtime_destroy(rt)` | Frees the runtime. |
| `jsb_runtime_setup(rt, fn, user)` | Runs `fn(ctx, user)` in a fresh handle scope, for defining modules. |
| `jsb_runtime_eval(rt, src, file, out, cap)` | Runs a classic script; writes `String(result)` or the thrown value's string to `out`. True when it completed. |

### Inside a native function (`void fn(jsb_ctx *ctx)`)
| Call | Behaviour |
| --- | --- |
| `jsb_arg_count` | Number of arguments the caller passed. |
| `jsb_arg_i32(ctx, i, &out)` | Needs a Number; converted like ToInt32 (wraps, NaN is 0). |
| `jsb_arg_f64(ctx, i, &out)` | Needs a Number. |
| `jsb_arg_bool(ctx, i, &out)` | Needs a Boolean; no truthiness. |
| `jsb_arg_str(ctx, i, buf, cap, &len)` | Needs a String; UTF-8 into `buf`, NUL-terminated. RangeError if it needs more than `cap - 1` bytes. |
| `jsb_arg_obj(ctx, i)` | Needs an Object (functions and arrays count, `null` does not). |
| `jsb_arg_val(ctx, i)` | Any value; never throws. |
| `jsb_arg_bytes(ctx, i, &data, &len)` | Needs an ArrayBuffer, TypedArray or DataView; gives its bytes in place (see "Buffers"). TypeError if detached. |
| `jsb_new_external_buffer(ctx, data, len, free_fn, user)` | Wraps native memory as an ArrayBuffer without copying; JS then owns it. |
| `jsb_adjust_external_memory(ctx, change)` | Reports native memory held by JS wrappers to the GC; returns the runtime's total. |
| `jsb_ret_undefined/_bool/_i32/_f64/_str/_val` | Set the result; the last call wins; ignored after a throw. |
| `jsb_throw_type/_range(ctx, fmt, ...)` | Throw TypeError/RangeError with a printf-style message. Only the first throw of a call counts. |
| `jsb_new_object`, `jsb_set_fn`, `jsb_set_prop`, `jsb_get_prop` | Build and read objects. Set and get are ordinary `[[Set]]`/`[[Get]]`, so accessors run. |
| `jsb_define_module(ctx, name, init)` | Makes a fresh object, fills it with `init`, then sets the global `name`. A failed `init` leaves no global. |

**Missing arguments** count as `undefined`, so every typed getter throws
`TypeError: argument N must be ...` (N counts from 1) for them. Getters never coerce
(DEC-M1-T003). When a getter returns false or NULL, an exception is pending: return at
once. A function that sets no result returns `undefined`.

```c
static void fn_add(jsb_ctx *ctx) {
    double a, b;
    if (!jsb_arg_f64(ctx, 0, &a) || !jsb_arg_f64(ctx, 1, &b)) return;
    jsb_ret_f64(ctx, a + b);
}
static bool init_math(jsb_ctx *ctx, jsb_val *m) {
    return jsb_set_fn(ctx, m, "add", fn_add);
}
/* in a jsb_runtime_setup callback: */
jsb_define_module(ctx, "__native_math", init_math);
```

## Ownership of strings
* **Strings in** (`name`, `fmt`, `jsb_ret_str` text, script source) are borrowed for the
  call and copied into the engine. The caller keeps and frees them.
* **Strings out** go into a caller buffer (`jsb_arg_str`, `jsb_runtime_eval`). jsb never
  returns a pointer to engine-owned text, so there is nothing to free.
* `jsb_arg_str` refuses to truncate (RangeError); `jsb_runtime_eval` truncates its
  diagnostic output to fit. Both always NUL-terminate. A string may hold embedded NULs:
  use the `len` out-parameter, not `strlen`.
* Text is UTF-8. Invalid UTF-8 given to jsb becomes U+FFFD.

## Buffers (zero-copy)
Pixel, audio and file data cross the seam without copies (DEC-M1-T005).

* **`jsb_arg_bytes`** gives a pointer to the first byte of the view (its byte offset
  applied) and its length in **bytes**, not elements. The memory belongs to JS. The
  pointer is valid only until the native function returns, and only while it runs no JS
  (JS could detach the buffer). Writes through it are visible to JS at once. `data` may be
  NULL when `len` is 0. A detached ArrayBuffer, or a view on one, throws a TypeError.
  SharedArrayBuffer is not accepted.
* **`jsb_new_external_buffer`** returns an ArrayBuffer over native memory the caller
  allocated. On success JS owns the memory, and `free_fn(data, len, user)` runs
  **exactly once**: after the buffer is garbage-collected or detached, or at the latest
  in `jsb_runtime_destroy`. Until then C may read and write the bytes on the main thread,
  and JS sees the writes. NULL `data` or `free_fn` is a TypeError, an oversize `len` a
  RangeError; on failure the caller still owns `data`.
* **The free function may run on a V8 GC helper thread**, in the middle of a GC. It must
  only release memory: no jsb or V8 calls, no locks the main thread may hold.
* **External memory accounting.** V8 counts every ArrayBuffer backing store, including
  external ones, toward its GC heuristics, so `jsb_new_external_buffer` reports nothing.
  Native memory that JS keeps alive through some *other* object (a texture or decoder
  behind a small wrapper) is reported with `jsb_adjust_external_memory`: add the size when
  it is made and subtract the same amount when it is freed, on the main thread. On V8 this
  calls `Isolate::AdjustAmountOfExternalAllocatedMemory`. A change that would make the
  total negative is refused with -1 and a message on stderr. The QuickJS backend keeps
  the same total but has no GC hook to give it to.

```c
static void free_pixels(void *data, size_t len, void *user) { (void)len; (void)user; free(data); }
/* in a native function: JS owns `pixels` once this succeeds */
jsb_val *buf = jsb_new_external_buffer(ctx, pixels, w * h * 4, free_pixels, NULL);
if (!buf) { free(pixels); return; }
jsb_ret_val(ctx, buf);
```

## Persistent callbacks
Audio `onended` and video events keep a JS function past the native call that received
it and run it later from the host (DEC-M1-T006).

| Call | Behaviour |
| --- | --- |
| `jsb_callback_new(ctx, fn, &cb)` | Keeps the function `fn` alive in a handle. TypeError (false, `cb` zeroed) if `fn` is not callable. |
| `jsb_callback_call(cb, argc, argv, &ret)` | Calls it with `this` undefined and C-built arguments; returns a `jsb_status`. |
| `jsb_callback_free(&cb)` | Releases the function and zeroes `cb`. False for a zero, freed or stale handle. |
| `jsb_set_error_handler(rt, fn, user)` | Where callback exceptions go; NULL restores the stderr default. |

* **Handles are small values** (`jsb_callback`: runtime, slot, generation), not pointers.
  Copy them freely; `jsb_callback cb = {0};` means "none". Each handle still has **one
  owner**, who frees it once the event can no longer fire. After the free every copy is
  refused (`JSB_ERR_HANDLE`, message on stderr), also after jsb reuses the slot for a new
  callback, because the slot's generation changes. A double free returns false; it never
  corrupts anything. Handles die with their runtime: `jsb_runtime_destroy` releases the
  live ones, and a handle must not be used after that.
* **Arguments** are `jsb_cval`s: undefined, null, Boolean, Number or String (UTF-8,
  borrowed, copied). At most 64; `argv` may be NULL when `argc` is 0. Anything else,
  including `JSB_TYPE_OBJECT` or a NULL string, is `JSB_ERR_ARG` and nothing runs. No JS
  object travels through a C argument, so no `jsb_val` is ever kept.
* **The result** goes into an optional `jsb_result`: its type, Boolean or Number, and for
  a String a copy into the caller's `str`/`str_cap` buffer (truncated, NUL-terminated)
  with the full length in `str_len`. Objects, functions, symbols and BigInts report only
  `JSB_TYPE_OBJECT`. On any error the type is `JSB_TYPE_UNDEFINED`.
* **Exceptions never cross into C.** A throw is caught, `String(exception)` and its
  `stack` property (or `""`) go to the runtime's error handler before the call returns,
  and the call returns `JSB_ERR_THREW`. Called from inside a native function, the
  exception does not reach the enclosing JS caller either. The engine's logger,
  `error_handler_on_jsb_error`, is a handler for this slot (see "Error handler").
  The handler must not call jsb on that runtime.
* **Scopes.** Every call opens its own `HandleScope` (and enters the isolate and
  context), so it works from the host loop with no JS on the stack and inside a native
  call. The function is held in a Local for the call, so a callback may free its own
  handle. Microtask checkpoints stay with the host's loop (§4.1, item 3).
* **GC.** The handle is a strong `Global`: the function cannot be collected while it
  lives. After `jsb_callback_free` it is garbage as soon as JS holds no other reference
  (QuickJS frees it at once by reference counting; V8 at a later GC).
* Main thread only. Never call `jsb_callback_free` from a `jsb_free_fn`.

```c
static jsb_callback g_onended;               /* owned by the audio binding */
static void fn_set_onended(jsb_ctx *ctx) {
    jsb_callback cb;
    if (!jsb_callback_new(ctx, jsb_arg_val(ctx, 0), &cb)) return;
    if (g_onended.rt) jsb_callback_free(&g_onended);
    g_onended = cb;
}
/* later, in the host loop when the sound ends: */
jsb_cval id = {JSB_TYPE_NUMBER, false, 3, NULL};
jsb_callback_call(g_onended, 1, &id, NULL);  /* JSB_ERR_THREW was reported already */
```

## Error handler
`src/engine/error_handler.c` turns JS errors that reach C into a log line, a
window.onerror / `'error'` event and, when fatal, a dialog (§4.1 item 5, card M1-T011,
DEC-M1-T011). It holds no JS value and has no QuickJS calls: it reaches JS only through
`js_engine.h`, so it needs no handle rules of its own and works on both engines.

| Call | Behaviour |
| --- | --- |
| `error_handler_report_js_error(engine, message, stack, fatal)` | Logs `JS Exception: <message>` and `Stack trace:` (scrubbed), runs the shim's `__error_handler_dispatch(message, error)`, then a dialog if `fatal`. `engine` NULL skips dispatch. |
| `error_handler_on_jsb_error(message, stack, user)` | A `jsb_error_fn`: `jsb_set_error_handler(rt, error_handler_on_jsb_error, NULL)` sends jsb callback exceptions to this log instead of stderr. Declared without `jsb.h`, so it builds in both modes. Whoever owns the runtime installs it; `js_engine_v8.cc` does not yet (not this card's file). |
| `error_handler_attempt_auto_save(engine)` | Runs the shim's `__error_handler_autoSave`; true if it returned non-zero. |
| `error_handler_report_js_exception(engine, fatal)` | quickjs mode only, an inline in `error_handler.h` for the QuickJS-era tests: takes QuickJS's pending exception and reports it as above. Removed by M1-T027. |

* **Text in, script out.** Message and stack (each cut at 4096 bytes) are escaped into a
  short script run with `js_engine_eval`; quotes, backslashes and control characters
  cannot end the literal. No dispatcher (DOM shim not loaded) is a no-op; a dispatcher
  that throws is reported by the engine and logged as a warning.
* **Deviation:** `error` in the listener is a new `Error` whose `message` is the thrown
  value's `String()` form and whose `stack` is the original stack, not the thrown object
  itself, because exceptions never cross into C as JS values.
* **jsb callback errors** are logged only, not dispatched: a jsb error handler must not
  run JS on its runtime (see "Persistent callbacks").
* Main thread, outside any JS call.

`tests/smz/test_error_handler_jsb.c` (ctest `error_handler_jsb`, both modes) checks the
log lines, exact delivery of quotes, backslashes, newlines and UTF-8 to window.onerror,
that a message cannot break out of its literal, missing and throwing dispatchers, every
auto-save outcome, and in v8 mode that a throwing jsb callback is logged with its stack.
The older `tests/test_error_handler.c` still calls QuickJS directly and runs on QuickJS
in both modes until M1-T027 ports it.

## Ported bindings
Each port keeps its JS API byte-for-byte (names, order, `length`, results, errors) and
adds `bind_<x>_define(jsb_runtime *rt, ...)`; the old `bind_<x>_register(struct JSContext
*, ...)` stays as a one-line wrapper under `#ifdef SMZ_JSB_ADOPT` until M1-T026
(DEC-M1-T014-JSB-ADOPT). Each has a `tests/smz/test_bind_<x>_jsb.c` ctest run in both modes.

| Binding | Globals | Functions (`length`) | jsb calls used | Buffers | Callbacks |
| --- | --- | --- | --- | --- | --- |
| `bind_platform.c` (M1-T014) | `__native_platform`, `__native_process` | `setFullscreen` (1), `isFullscreen`, `getDisplaySize`, `getDisplayScale`, `getWindowSize`, `setWindowTitle` (1), `quit` (0 for the rest); process: `platform`, `arch`, `engine` strings, `environ`, `hrtime` (0) | `jsb_define_module`, `jsb_runtime_setup`, `jsb_set_fn_n`, `jsb_arg_count`, `jsb_arg_truthy`, `jsb_arg_to_str`/`jsb_free_str`, `jsb_new_object`/`array`/`i32`/`f64`/`str`, `jsb_set_prop`, `jsb_set_index`, `jsb_ret_*`, `jsb_throw_type`, `jsb_throw_internal`, `jsb_engine_version` | none | none |

* **bind_platform.** The `__native_process` helpers stay inline in `bind_platform.h`
  (`bind_platform_define_process(rt)`), so window-less tests need no SDL;
  `bind_platform_register_process(struct JSContext *)` is their transitional wrapper.
  `bind_platform_define` also defines `__native_process`, as the old register did. The
  `Platform *` is kept in a file static (jsb modules have no user pointer); no jsb handle
  is stored. `engine` is `jsb_engine_version()`, the running engine's version.

## Handle rules (§15.8)
These are the rules for code that touches V8 handles, in jsb and elsewhere (WebGL has
its own GL-object rules in `webgl.md`).

* A `Local<T>` is a temporary handle, valid only until the `HandleScope` it was created
  in ends. **Never store a `Local` in a C struct, a global or a static.**
* Every native function called from JS and every callback we drive (timers, rAF, audio
  and video events) opens its own `HandleScope`. A loop that creates many handles opens
  one scope per iteration.
* Return a `Local` out of a scope only with `EscapableHandleScope::Escape`.
* To keep a JS object beyond the current call, use `v8::Global<T>`. Each Global has
  exactly one owner, which calls `Reset()` when done; a leaked Global is leaked memory.
* Weak Globals with finalizers only release native resources; the finalizer never runs
  JS or touches GL.
* Every `ArrayBuffer` backing store says who owns its memory (V8 or our allocator), and
  external memory is reported with `AdjustAmountOfExternalAllocatedMemory`.
* Only the main thread enters the isolate. Other threads post work to it.

How jsb applies them:

* `jsb_ctx` and `jsb_val` are the C face of these rules. Both are valid only until the
  native call (or setup callback) that received the ctx returns. **Never keep a
  `jsb_ctx *` or `jsb_val *`** in a struct, global or static, and never pass one to
  another call. To keep a JS function, make a `jsb_callback` (built on `Global`).
* The trampoline in `jsb_v8.cc` opens a `HandleScope` per call before it builds the
  `jsb_ctx` on its stack. The ctx holds the call's Locals in a deque (so `jsb_val`
  pointers stay put) and is destroyed before the scope closes.
* `jsb_runtime_setup` and `jsb_runtime_eval` each open their own scope; a loop that
  creates many values should run in separate setup calls or JS calls.
* jsb owns Globals only for the runtime's context and for each live `jsb_callback`
  (one owner per Global, see "Persistent callbacks"). `jsb_runtime_destroy` resets them
  all before disposing of the isolate.
* jsb is main-thread only. The one exception is a `jsb_free_fn`, which V8 may call from
  a GC helper thread (see "Buffers").

## Runtime stack

V8 engine initialization measures the available stack below its current main
thread frame before creating the first isolate. Linux uses pthread stack bounds,
macOS uses pthread stack address/size, and Windows uses the virtual allocation
base. It passes `--stack-size` in KiB, subtracting 25% of available space or
256 KiB, whichever is larger, for native calls and exception reporting. The
selected value is printed at startup. An unavailable or too-small stack fails
initialization loudly. JS, SDL and the event loop stay on the process main thread;
no runtime thread is created. Embedders must initialize the engine on that thread
before creating other jsb runtimes. QuickJS retains its Windows linker reservation.
`stack_size` checks repeated catchable recursion overflow and engine recovery.

## js_engine on V8
`src/engine/js_engine_v8.cc` implements `src/engine/js_engine.h` on V8 (§4.1 item 3,
card M1-T007). In v8 mode CMake builds it as `smz_js_engine_v8`, which defines
`SMZ_JS_ENGINE_V8` for its users. Until `game_loop.c`, `error_handler.c` and the bindings
are ported (M1-T009, M1-T011, M1-T016..T025) the runtime executable still links
`js_engine.c`, because those files call QuickJS through `js_engine_get_context`; the
v8-mode library serves every caller that needs only `js_engine.h`, starting with tests.

- **Isolate and context** come from `jsb_runtime_create`; the engine picks them up inside
  `jsb_runtime_setup`, so `js_engine_get_jsb()` returns a runtime whose modules land in
  the same global object as the game's scripts. Isolate data slot 0 is jsb's, slot 1 the
  engine's. ICU data comes from `SMZ_V8_ICU_DATA_PATH` (the `icudtl.dat` of
  `SMZ_V8_DIR`).
- **Script names**: every compile gets a `ScriptOrigin` with line and column offset 0, so
  traces read `name:line:col` like NW.js DevTools. A file is named by its path relative
  to the game root when the root is known (`js_engine_set_cache_dir`), else by the path
  given. A SyntaxError's `stack` has no frame in V8, so the reported stack gets
  `    at name:line:col` from the exception message.
- **Errors** are reported as in QuickJS mode: the exception's `String()` form, then its
  `stack`, both at level `error` through the console callback (or stderr).
- **Microtasks**: `MicrotasksPolicy::kExplicit`; `js_engine_execute_pending_jobs` runs one
  checkpoint, then reports rejections still unhandled (through
  `__error_handler_dispatchRejection` when the shim is loaded). Its count is the number
  of promise reaction jobs, taken from a promise hook; `queueMicrotask` callbacks are
  not counted. It returns -1 if a microtask threw uncaught.
- **Code cache**: V8 code caches under `$XDG_CACHE_HOME/smzuggler-rt/v8cache/`, never
  a `.jsc` beside the script; see "Code cache" below.
- **Watchdog**: see "Watchdog" below.
- **`js_engine_get_context`** is declared only without `SMZ_JS_ENGINE_V8`. In quickjs
  mode `js_engine_get_jsb()` returns the engine's own `JSContext` adopted by the
  transitional backend (see "Adoption" above), so jsb modules share the game's globals.
- **Handles** (§15.8): every entry point opens its own `HandleScope`; the engine owns
  exactly two kinds of `Global` (its context and pending rejections) and Resets them in
  `js_engine_shutdown` before the isolate is disposed of.

### Code cache
Card M1-T012 (§4.1 item 4, §5.6). `js_engine_eval_file_cached`, and `js_engine_eval_file`
once a cache dir is set, keep V8's compiled code in `<dir>/<sha256>.bin`.

- **Directory**: the one given to `js_engine_set_cache_dir`, else
  `js_codecache_default_dir()` (`$XDG_CACHE_HOME/smzuggler-rt/v8cache`, falling back to
  `$HOME/.cache/...`; Linux only). With neither, scripts run uncached.
- **Key**: `js_codecache_key` hashes the source, a NUL, `V8::GetVersion()`, a NUL and
  `ScriptCompiler::CachedDataVersionTag()` (which already folds in the flag hash). The
  key comes from the source text, not the mtime, so an edit always recompiles.
- **Produce**: on a miss the script compiles normally, runs, and only then
  `CreateCodeCache` is called, so functions compiled lazily during the first run are in
  the cache. The file is written to a temporary name and renamed into place.
- **Consume**: the file is passed as `CachedData` (`BufferNotOwned`; the engine frees
  the buffer after the compile) with `kConsumeCodeCache`. If V8 sets `rejected`
  (another build, damaged data) the file is deleted and a fresh cache is written.
- **Observability**: `js_codecache_get_stats` counts produced, accepted and rejected
  caches for the process; `tests/smz/test_codecache.c` reads them, since acceptance is
  invisible from JS.
- `src/engine/js_codecache.cc` has no V8 dependency (CMake target `smz_js_codecache`),
  so the key, directory and file handling are tested in quickjs mode too.
- **Handles**: no `Local` outlives `compile_and_run`'s `HandleScope`; the `CachedData`
  belongs to the `ScriptCompiler::Source` and is deleted with it.

### Watchdog
`js_engine_set_watchdog(engine, ms)` (from `--watchdog <ms>`) stops scripts that run
longer than `ms` since the last arm (§4.1 item 3, card M1-T008, DEC-M1-T008). The
outermost eval entry point and each microtask checkpoint arm it, and the game loop arms
it once per frame, as in QuickJS mode.

- **Thread**: `src/engine/js_watchdog.cc` is a timer thread that knows nothing of V8.
  It exists only while the limit is non-zero: 0 (the default) stops and joins it, a new
  limit replaces it, and `js_engine_shutdown` joins it before the isolate goes.
  `js_watchdog_live_threads()` counts the live threads, so tests can check that none
  exists while the limit is off.
- **Termination**: when an armed generation passes its deadline, the thread calls
  `Isolate::RequestInterrupt` (the only isolate call it makes). The interrupt runs on
  the main thread inside the script, reports
  `js_engine: watchdog: script ran longer than N ms, terminated at file:line:col` at
  level `error`, and calls `Isolate::TerminateExecution`. `try`/`catch` cannot catch it.
  A request made stale by a later arm (the script finished in time) is dropped.
- **Recovery**: when the outermost entry point returns, the engine calls
  `CancelTerminateExecution`, so the next eval runs. The failed eval returns false and
  the checkpoint returns -1; rejections still pending are reported at the next
  checkpoint. `js_engine_watchdog_arm` outside JS also cancels a termination left by JS
  the game loop called directly, so the frame loop continues.
- **Limits**: a script blocked inside a native call is stopped only once it returns to
  JS, because V8 checks interrupts only while it runs JS.

`tests/smz/test_js_watchdog.c` (ctest `js_watchdog`, v8 mode) checks that `while(true){}`
under 200 ms stops within 400 ms naming `spin.js:2`, that a `catch` cannot swallow it,
that the next eval succeeds, that re-arming per eval and idle time do not fire, that a
looping promise job is stopped too, and that no thread exists while the limit is off.

`tests/smz/test_js_engine_v8.c` (ctest `js_engine_stack`, both modes) checks that a
throw in `foo.js` line 3 reports `foo.js:3` from a file, a string and a SyntaxError; in
v8 mode also the game-relative name and that `js_engine_get_jsb()` sees the engine's
globals. In v8 mode the unchanged `tests/test_js_engine.c` also runs on V8 as ctest
`js_engine_on_v8` (`tests/smz/js_engine_v8.cmake`).

## Game loop
`src/engine/game_loop.c` drives the browser event loop that RPG Maker MZ expects: SDL
events, then `__dom_setWindowSize` (on resize), `__dom_flushInputEvents` and
`__dom_updateGamepads`; `__dom_flushTimers`; a job drain; `__dom_flushAnimationFrames(ts)`
with the ms elapsed since `game_loop_create`; a second job drain; video, audio, present.
Card M1-T009 removed its QuickJS calls (§4.1 item 5) without changing that order or the
timing (DEC-M1-T009).

- **One entry point.** On first use the loop evaluates a small helper that defines the
  non-writable global `__smz_gameLoop(op, ...)`. Every call into JS (phases,
  `__nw_hostClose`, `__control_eval`, the exit probe) is `js_engine_eval_string` of a
  short call to it, so the loop uses only `js_engine.h` and runs unchanged on
  `js_engine.c` and on `js_engine_v8.cc`, which sits on jsb. The helper answers with a
  one-letter code (`n` missing, `t`/`f` truthy/falsy result, `x` threw, `v` value, `e`
  probe error) followed by any text.
- **No handles in C.** The helper snapshots the five `__dom_*` functions on the first
  frame and keeps them inside JS, as Outsider's cached `JSValue`s did, so the loop holds
  no `JSValue`, `jsb_val`, `jsb_callback` or `Global` and has nothing to release (§15.8).
  The engine must still outlive `game_loop_destroy`.
- **Errors.** A throwing phase is caught in JS, passed to `__error_handler_dispatch`
  (window.onerror) and returned; C logs `JS Exception: <String(e)>` and, for an Error
  with a stack, `Stack trace:`, the lines `error_handler_report_js_exception` writes. The
  rest of the frame still runs. A missing or throwing `__nw_hostClose` lets the window
  close.
- **Control mode.** `__native_control_send` is a JS function that queues reply lines; the
  loop prints them after each `eval` command, at the end of each frame (so replies sent
  by promise jobs appear in the frame they settle) and at shutdown.
- **Cost.** Each frame evaluates five short scripts. QuickJS compiles each one (a few
  microseconds); V8 reuses its compilation cache except for the rAF call, whose
  timestamp changes the source. In v8 mode each eval also re-arms the watchdog, so its
  limit applies per phase rather than per frame.

### Microtask checkpoints
V8 runs with `MicrotasksPolicy::kExplicit`: promise jobs run only at a checkpoint the
embedder asks for, never when a script returns. Chrome (and so NW.js) runs one after every
task, and Node runs the `process.nextTick` queue before promise jobs. Card M1-T010 makes
the loop do the same (DEC-M1-T010 has the exact order):

- **After each native input event (V8).** The bootstrap polls the input batch once,
  then invokes the original input dispatcher with one raw event at a time. Each
  invocation checkpoints after complete delivery, including consumed overlay events,
  bubbling and default actions. The borrowed poll function is restored on errors.
  Resize and gamepad phases temporarily wrap document/window dispatch with shared
  nesting depth. The video update phase also wraps video-element dispatch, so each
  native-backed `ended` event finishes all synchronous listeners before draining
  ticks and promises, ahead of the next video event or rAF callback. Temporary
  wrappers restore inherited dispatch methods as well as own methods on errors.
  Each phase also retains its engine-driven checkpoint.
- **After each timer and rAF callback (V8).** Scheduler accessors installed at context
  creation wrap callbacks as the DOM shim installs its schedulers. Startup registrations
  are covered before the first frame. Each callback checkpoints in a finally block.
- **nextTick first (V8).** The context owns a dedicated tick queue. Every native and
  engine-driven checkpoint exhausts ticks before draining promises, then repeats if
  promises queued more ticks. A throwing tick is reported, then remaining ticks
  drain before promises; watchdog termination stops the drain. The process shim
  keeps its other properties.

The exact orders and QuickJS phase-level differences are recorded in
[DEC-M1-T010](decisions/DEC-M1-T010.md). `tests/smz/test_microtasks.c` runs the real
loop and DOM input dispatcher, registering startup timers and rAF callbacks before
any frame. It also checks promises queued before ticks and ticks queued by promises.

`tests/smz/test_game_loop_js.c` (ctest `game_loop_js`, both engine modes) runs the real
`game_loop.c` with the platform, renderer, audio, video and log calls stubbed. It checks
the phase order and arguments, both job drains, a growing Number timestamp, the
first-frame snapshot, exception logging and dispatch, the `__nw_hostClose` outcomes,
exit-probe values, errors and escaping, and control-mode replies (synchronous, promise,
unknown command) through a stdin pipe.

## Tests
`tests/smz/test_jsb_core.c` (ctest `jsb_core`, both engine modes) defines a module from C, calls
every getter, return and throw from JS, and checks values, wrong-type and
missing-argument TypeErrors, the string-size RangeError and module failure.

`tests/smz/test_jsb_buffers.c` (ctest `jsb_buffers`, both engine modes) checks
`jsb_arg_bytes` on ArrayBuffers, TypedArrays and DataViews with byte offsets, the
TypeError for non-buffers and detached buffers, C writes seen by JS and JS writes seen by
C through an external buffer, that its free function runs exactly once after `gc()`
(from `jsb_set_flags("--expose-gc")`) and once more at destroy for a buffer still alive,
and the `jsb_adjust_external_memory` totals.

`tests/smz/test_jsb_callbacks.c` (ctest `jsb_callbacks`, both engine modes) keeps a
function in a native call and calls it from C after that scope ended, checks every
argument and result type and string truncation, that exceptions (Errors and thrown
primitives, from the host and from inside a native call) reach the error handler with
message and stack and return `JSB_ERR_THREW`, that a freed function is collected (an
external buffer hung on it is freed only after `jsb_callback_free` and `gc()`), that
freed and stale handles and bad arguments are refused, and that a callback may free its
own handle.
