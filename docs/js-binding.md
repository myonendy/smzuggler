<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-11-05: complete the jsb API reference, the §15.8 lifetime
rules with examples, and the binding, error and threading guides (M1-T030).
Modified 2026-11-05: make V8 the sole engine and retire the adoption bridge.
Modified 2026-11-04: document the Canvas2D binding port and baseline.
Modified 2026-11-01: add persistent callbacks.
Modified 2026-11-01: document the V8 script watchdog.
Modified 2026-11-01: document how the game loop calls into JS.
Modified 2026-11-01: document the error handler's js_engine.h path.
Modified 2026-11-01: document microtask checkpoints and nextTick order.
Modified 2026-11-04: document the V8 code cache.
Modified 2026-11-04: document the bind_io port and parity checks.
Modified 2026-11-04: document main thread stack sizing.
Modified 2026-11-04: document the font and audio binding ports and buffer validation.
Modified 2026-11-04: document the image binding and its buffer ownership.
Modified 2026-11-04: document the font binding port and buffer validation.
Modified 2026-11-04: document the video binding port and parity checks.
Modified 2026-11-05: document that M1-T026 removed the bind_<x>_register wrappers
and that tests reach the engine through js_engine.h and jsb.h.
Modified 2026-11-04: document the Effekseer binding port and parity capture.
Modified 2026-11-05: document the filters binding port and parity capture.
Modified 2026-11-05: document non-enumerable jsb functions and the shared
ArrayBuffer brand check (M1-T026b).
Modified 2026-11-04: document the renderer binding port and parity checks.
How native C bindings talk to the JS engine through jsb.h, and the handle rules.
-->
# JS binding: the `jsb` seam

`src/jsb/jsb.h` is the C API that native bindings (`src/bindings/`) use to expose
`__native_*` modules to the shims. It replaces direct QuickJS calls so that Outsider's
bindings move to V8 by near-mechanical rewriting (spec §4.1). It is **not** an engine
abstraction: its one real implementation is `src/jsb/jsb_v8.cc`.

Status: core API (M1-T003), zero-copy buffers (M1-T005), persistent callbacks
(M1-T006) and the complete API guide with the §15.8 rules and examples (M1-T030).
The library target is `smz_jsb`; `jsb.h` itself is plain C11 and includes no C++ or V8
header (checked by `tests/smz/test_jsb_header.c` on V8). Every function in `src/jsb/`,
`src/bindings/` and `src/engine/` carries a §15.1 doc comment, checked by
`tools/doclint/doclint.py`.

V8 is the sole engine (M1-T027); the former backend and adoption bridge
have been removed, ending DEC-SETUP-003. CMake imports the prebuilt monolith
from the `SMZ_V8_DIR` CMake setting, then `$SMZ_V8_DIR`, then
`$SMZ_DEPS_DIR/v8/<pin>` (using `SMZ_V8_VERSION_PIN` in `cmake/V8.cmake`),
with the standard offline dependency cache as the fallback; the surface inventory
observer follows the host-owned `process` accessor and leaves other getters
uncalled to avoid browser/device side effects.

The platform test follows `RMMZ_RENDER_BACKEND` unchanged.

## Seam calls for binding ports (M1-T007b)

In `jsb.h` on V8 (test `jsb_values`); they stay after M1-T027. Coercing reads
apply the ECMAScript conversions (valueOf runs; a Symbol is a TypeError).
`jsb_clear_exception` never drops a watchdog termination; on V8 each native call runs
in a `TryCatch` that it resets. On V8 `jsb_throw_internal` makes an `Error` whose own
`name` is `"InternalError"`. Added beyond the card's list: `jsb_val_to_i32/u32`
(bind_tilemap.c coerces elements), `jsb_val_type` and `jsb_engine_version`.

| QuickJS in `src/bindings/` | jsb |
| --- | --- |
| `JS_CFUNC_DEF`, `JS_SetPropertyFunctionList`, `JS_NewCFunction` | `jsb_set_fn_n` (arity = `length`, writable/configurable/non-enumerable) |
| `JS_IsArrayBuffer` | `jsb_arg_is_array_buffer` (intrinsic brand check, no getters) |
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

## Ported asset and media bindings

Each port defines its module in `bind_<x>_define(jsb_runtime *rt, ...)`. M1-T026 removed
the transitional `bind_<x>_register(struct JSContext *, ...)` wrappers; every caller now
passes `js_engine_get_jsb()`. A `tests/smz/test_bind_<x>_jsb.c` test pins the JS API to
the pre-port binary's API while running on V8.

| Binding | Module and functions (`length`) | jsb calls used | Buffers | Callbacks |
| --- | --- | --- | --- | --- |
| `bind_io.c` (M1-T016) | `__native_io`: `readFileSync` (1), `readFileBinary` (1), `writeFileSync` (2), `writeFileBinary` (2), `mkdirSync` (1), `existsSync` (1), `isDirSync` (1), `readdirSync` (1), `unlinkSync` (1), `renameSync` (2), `getGameRoot` (0), `resolvePath` (1) | `jsb_define_module`, `jsb_set_fn_n`, `jsb_arg_count/to_str/val/bytes`, `jsb_free_str`, `jsb_get_prop`, `jsb_val_type`, `jsb_clear_exception`, `jsb_new_array/str/str_len/buffer_copy`, `jsb_set_index`, `jsb_ret_val/bool/str/undefined`, `jsb_throw_type/internal`, `jsb_runtime_setup` | Reads copy native bytes into engine-owned ArrayBuffers; writes borrow bytes only during the call and accept ArrayBuffers only | none |
| `bind_input.c` (M1-T015) | `__native_input`: `pollEvents` (0), `getGamepad` (1), `getGamepadCount` (0) | `jsb_define_module`, `jsb_set_fn_n`, `jsb_new_object/array/str/i32/f64/bool/null`, `jsb_set_prop`, `jsb_set_index`, `jsb_arg_count`, `jsb_arg_to_i32` (`getGamepad` coerces its index), `jsb_ret_val/i32`, `jsb_runtime_setup` | none | none |
| `bind_font.c` (M1-T018) | `__native_font`: `loadFont` (2), `loadFontFile` (2), `hasFont` (1), `getMetrics` (2), `measureText` (3), `fontCount` (0), `fontNames` (0) | `jsb_define_module`, `jsb_runtime_setup/eval`, `jsb_set_fn_n`, `jsb_arg_count`, `jsb_arg_to_str/f64`, `jsb_free_str`, `jsb_arg_bytes`, `jsb_clear_exception`, `jsb_throw_type`, `jsb_new_object/array/str/f64/null`, `jsb_set_prop/index`, `jsb_ret_val/bool/i32/f64` | Borrowed ArrayBuffer bytes during `loadFont`; both font caches copy them before return. File bytes are owned and freed by the binding. | none |
| `bind_audio.c` (M1-T019) | `__native_audio`: `init`, `shutdown`, `isInitialized`, `stopAll`, `getActiveVoiceCount` (0); `load`, `loadStream`, `loadFromMemory`, `freeSource`, `stop`, `stopBus`, `pause`, `resume`, `pauseBus`, `resumeBus`, `isPlaying`, `isPaused`, `getVolume`, `getPosition`, `getDuration` (1); `setVolume`, `setPitch`, `setPan`, `setBusVolume`, `seek` (2); `fadeVolume`, `fadeBusVolume` (3); `play` (6) | `jsb_define_module`, `jsb_runtime_setup/eval`, `jsb_set_fn_n`, `jsb_arg_count`, `jsb_arg_to_str/u32/i32/f64`, `jsb_arg_truthy`, `jsb_free_str`, `jsb_arg_bytes`, `jsb_clear_exception`, `jsb_throw_type/range`, `jsb_new_u32/i32`, `jsb_set_prop`, `jsb_ret_val/bool/i32/f64` | ArrayBuffer-only `loadFromMemory`; borrowed bytes are copied by SoLoud before return. | none |
| `bind_video.c` (M1-T020) | `__native_video`: `open`, `close`, `play`, `pause`, `getState` (1); `setVolume`, `setLoop`, `setVisible` (2) | `jsb_define_module`, `jsb_runtime_setup`, `jsb_set_fn_n`, `jsb_arg_count`, `jsb_arg_to_str/i32/f64`, `jsb_arg_truthy`, `jsb_free_str`, `jsb_new_object/bool/f64/i32/null`, `jsb_set_prop`, `jsb_ret_val/i32/undefined`, `jsb_throw_type` | none | none |
| `bind_canvas2d.c` (M1-T021) | `__native_canvas2d`: 27 methods, with upstream names and arities (baseline below) | `jsb_define_module`, `jsb_runtime_setup`, `jsb_set_fn_n`, `jsb_arg_count`, `jsb_arg_to_u32/i32/f64/str`, `jsb_free_str`, `jsb_arg_is_array_buffer`, `jsb_arg_bytes`, `jsb_clear_exception`, `jsb_throw_type/range/internal`, `jsb_new_object/i32/u32/null/buffer_copy`, `jsb_set_prop`, `jsb_ret_val/bool/f64` | Input ArrayBuffer bytes are borrowed only after coercions finish; rendering/font services copy them. Pixel results are engine-owned copies. File bytes and coerced strings are freed by the binding. | none |
| `bind_effekseer.c` (M1-T022) | `__native_effekseer`: `init` (3); `shutdown`, `isInitialized`, `stopAll`, `beginDraw`, `endDraw`, `resetBackground`, `getLoadedCount`, `getPlayingCount` (0); `load`, `setSpeed` (2); `play`, `setPosition`, `setRotation`, `setScale` (4); `release`, `isLoaded`, `stop`, `stopRoot`, `exists`, `setProjectionMatrix`, `setCameraMatrix`, `update`, `drawHandle`, `setRestorationOfStatesFlag`, `setBackground` (1) | `jsb_define_module`, `jsb_runtime_setup`, `jsb_set_fn_n`, `jsb_arg_count/val/to_str/to_i32/to_u32/to_f64/truthy`, `jsb_free_str`, `jsb_is_array`, `jsb_get_index`, `jsb_val_to_f64`, `jsb_clear_exception`, `jsb_new_u32`, `jsb_ret_val/bool/i32`, `jsb_throw_type` | none; matrices are plain Arrays read with Number coercion, copied to 16 native floats during the call | none |
| `bind_filters.c` (M1-T024) | `__native_filters`: `init`, `shutdown`, `drawQuad`, `endFilter` (0); `deleteShader` (1); `compileShader` (2); `setUniform1f`, `setUniformMat4` (3); `beginFilter`, `setUniform2f`, `setUniformTexture` (4); `setUniform4f` (6); plus the shader-ID values `colorMatrixShader`, `maskShader`, `blurShader`, `alphaShader`, `colorFilterShader` | `jsb_define_module`, `jsb_runtime_setup/eval`, `jsb_set_fn_n`, `jsb_set_prop`, `jsb_arg_count/val/type/to_str/to_u32/to_i32/to_f64`, `jsb_arg_bytes`, `jsb_get_prop`, `jsb_is_array`, `jsb_get_index`, `jsb_val_to_f64`, `jsb_clear_exception`, `jsb_free_str`, `jsb_new_u32`, `jsb_ret_val/bool/undefined`, `jsb_throw_type` | `setUniformMat4` borrows an ArrayBuffer or TypedArray's bytes only for the call; a plain (or short) Array is read with Number coercion into 16 native floats during the call. A DataView or other value stays ignored, as upstream's two probes did | none |



`jsb_set_fn_n` defines functions writable, configurable and non-enumerable, as
`JS_CFUNC_DEF` did, so `Object.keys(__native_input)` is empty without a
post-definition script. On V8 the functions are also non-constructable
(`ConstructorBehavior::kThrow`), so their only own properties are `length`
and `name`, as with `JS_CFUNC_DEF`; a plain V8 function would also expose
`prototype`, `arguments` and `caller`.

`bind_io_define` gets the same property attributes from `jsb_set_fn_n`; it needs
no setup script either.
The `bind_io_jsb` ctest checks all twelve names and arities above, string and
ArrayBuffer round trips (including embedded NULs and empty buffers), directory
arrays, boolean filesystem results, undefined write results, coercion, and exact
binding error names/messages through `smz_js_engine_v8`. Engine-generated Symbol
conversion messages use V8's spelling. No JS handle or borrowed buffer survives a native call.

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

`bind_font_define` preserves the native ArrayBuffer brand check with a setup
script that captures the intrinsic ArrayBuffer byte-length getter and wraps
`loadFont` with a non-constructible method of the same name and arity. The getter checks the actual
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


`bind_audio_define` preserves the native ArrayBuffer brand check using the same
captured intrinsic technique as the font port. Only the four bus constants are
enumerable. The wrapper's method syntax
preserves `loadFromMemory.length === 1` and its lack of a constructor. The C
binding uses coercing argument readers in the original order; all strings are
freed and no borrowed bytes or JS handles survive the call. Audio ending events
remain polled by the shim, so this port needs no persistent callbacks.

### M1-T019 parity report

Before the port, `bind_audio_jsb` was run against `bind_audio_register` on the
original QuickJS binary. It passed the following expectations; the final test
uses `bind_audio_define(js_engine_get_jsb(engine))`, selecting the real V8 engine
on the V8 engine.

| Pre-port check | Captured expected value |
| --- | --- |
| Own names, in order | `init,shutdown,isInitialized,load,loadStream,loadFromMemory,freeSource,play,stop,stopBus,stopAll,pause,resume,pauseBus,resumeBus,setVolume,setPitch,setPan,setBusVolume,fadeVolume,fadeBusVolume,isPlaying,isPaused,getVolume,getPosition,seek,getDuration,getActiveVoiceCount,BUS_BGM,BUS_BGS,BUS_ME,BUS_SE` |
| Function lengths, in the same order | `0,0,0,1,1,1,1,6,1,1,0,1,1,1,1,2,2,2,2,3,3,1,1,1,1,2,1,0` |
| Function properties | All `typeof` values are `function`; names match keys; properties are writable/configurable and non-enumerable. Function own names are `length,name`. |
| Bus constants | Enumerable `BUS_BGM,BUS_BGS,BUS_ME,BUS_SE`, values `0,1,2,3` |
| Uninitialized results | `isInitialized`, `isPlaying`, `isPaused`: `false`; `load`, `loadStream`, `loadFromMemory` (including an empty buffer), `play`, `getVolume`, `getPosition`, `getDuration`, `getActiveVoiceCount`: Number `0`; all other calls except `init`: `undefined` |
| Device-free init | With the engine disabled, `init()` returns `true` and `isInitialized()` stays `false` |
| Missing arguments: TypeError messages | `load requires a path argument`; `loadStream requires a path argument`; `loadFromMemory requires an ArrayBuffer`; `play requires 6 arguments` |
| Wrong buffer: TypeError message | `argument must be an ArrayBuffer` for null, objects, strings, typed arrays, DataView and spoofed ArrayBuffer tags |
| Invalid bus: RangeError message | `invalid audio bus: -1` or `invalid audio bus: 4` for play, stop/pause/resume bus, set/fade bus volume |
| Outside-root path: TypeError message | `audio path resolution failed: ../outside.ogg` for load and loadStream |
| Coercion | String and number conversions run in the original order; user-thrown errors keep their type/message. Symbol coercion is TypeError (engine wording differs). |

The pre/post binary `--dump-surface` files compare equal: `diff -u` emits no
lines (exit 0), for the complete inventory and therefore for audio. As specified
by DEC-M0-T009b, the inventory has no native audio module rows or function lengths;
the new parity test supplies that coverage.



## API

`jsb.h` is the complete interface; this section is the guide to it. Unless an entry says
otherwise, every call runs on the main thread. *Borrowed* arguments are read during the
call and never kept; *call-scoped* handles (`jsb_val *`, `jsb_ctx *`) die when the native
call or setup callback that received the context returns.

### Runtime (host side)

| Call | Contract |
| --- | --- |
| `jsb_set_flags(flags)` | Passes space-separated V8 flags such as `--expose-gc` for every runtime of the process. `flags` is borrowed. Only before the first runtime (V8 freezes its flags); false afterwards, with a message. Main thread. |
| `jsb_runtime_create(argv0, icu_data_path)` | Creates one isolate and one context; the first call starts V8 for the process. `argv0` and `icu_data_path` are borrowed. Returns NULL and prints the reason on stderr on failure. Free with `jsb_runtime_destroy`. Main thread. |
| `jsb_runtime_destroy(rt)` | Destroys `rt` and every JS object it owns, releasing live callbacks and external buffers; NULL is ignored. Native pointers that bindings stored in JS objects are not freed here. Main thread. |
| `jsb_runtime_setup(rt, fn, user)` | Runs `fn(ctx, user)` inside a fresh handle scope so host code can define modules. `user` is passed through untouched. Returns false, after printing a pending exception on stderr, when `fn` failed. Main thread. |
| `jsb_runtime_eval(rt, src, file, out, cap)` | Compiles and runs `src` (UTF-8, borrowed) as a classic script named `file` (borrowed). Writes `String(result)`, or on a throw the thrown value's `String()`, as UTF-8 into `out` (`cap` bytes, truncated, NUL-terminated) when `out` is non-NULL. Returns true when the script completed. Main thread. |
| `jsb_engine_version()` | Returns the engine's version string in static storage (never NULL), for `process.versions`-style reports. Any thread. |
| `jsb_set_error_handler(rt, fn, user)` | Routes exceptions thrown by `jsb_callback_call` to `fn(message, stack, user)`; NULL restores the stderr default. `user` is borrowed. Main thread. |

### Arguments

Every typed getter takes a 0-based `index`; a missing argument counts as `undefined`.
On a wrong type the getter throws a `TypeError: argument N must be ...` (N counts from 1)
and returns false or NULL. **When that happens, return at once.** Getters never coerce
(DEC-M1-T003).

| Call | Contract |
| --- | --- |
| `jsb_arg_count(ctx)` | Number of arguments the caller passed (0 outside a call). |
| `jsb_arg_i32(ctx, i, &out)` | Needs a Number, converted like ToInt32 (wraps modulo 2^32, NaN becomes 0). |
| `jsb_arg_f64(ctx, i, &out)` | Needs a Number. |
| `jsb_arg_bool(ctx, i, &out)` | Needs a Boolean; there is no truthiness. |
| `jsb_arg_str(ctx, i, buf, cap, &len)` | Needs a String; copies UTF-8 into `buf`, NUL-terminates, and writes the byte length (embedded NULs kept) to `len` when non-NULL. RangeError, writing nothing, when the text needs more than `cap - 1` bytes. `buf` is caller-owned. |
| `jsb_arg_obj(ctx, i)` | Needs an Object (functions and arrays count; `null` does not). Returns NULL after a TypeError otherwise. |
| `jsb_arg_val(ctx, i)` | Any value, `undefined` when missing; never throws. |
| `jsb_arg_bytes(ctx, i, &data, &len)` | Needs an ArrayBuffer, TypedArray or DataView; returns its bytes in place (byte offset applied, length in bytes). `data` may be NULL when `len` is 0. The memory stays JS-owned and is valid only for this call, and only while no JS runs (see "Buffers"). TypeError for another type, a detached buffer or a view on one; SharedArrayBuffer is refused. |
| `jsb_arg_is_array_buffer(ctx, i)` | True only for a plain ArrayBuffer, not a TypedArray, DataView or SharedArrayBuffer. An intrinsic brand check that runs no getters; never throws. |
| `jsb_arg_truthy(ctx, i)` | Returns ToBoolean(`arg[i]`) (JS_ToBool). Never throws. |
| `jsb_arg_to_i32(ctx, i, &out)` | Coerces with ToInt32, running `valueOf`/`toString`/`Symbol.toPrimitive`; a Symbol is a TypeError. False with the exception pending on a throw. |
| `jsb_arg_to_u32(ctx, i, &out)` | Coerces with ToUint32; otherwise as `jsb_arg_to_i32`. |
| `jsb_arg_to_f64(ctx, i, &out)` | Coerces with ToNumber; otherwise as `jsb_arg_to_i32`. |
| `jsb_arg_to_str(ctx, i, &len)` | Coerces with ToString and returns NUL-terminated UTF-8 in new memory; write the byte length (embedded NULs kept) to `len` when non-NULL. The caller frees it with `jsb_free_str`. NULL after a conversion throw or an InternalError "out of memory". |
| `jsb_free_str(str)` | Frees a string from `jsb_arg_to_str`; NULL is ignored and it is safe after the call has returned. |

### Return values

| Call | Contract |
| --- | --- |
| `jsb_ret_undefined`, `jsb_ret_bool`, `jsb_ret_i32`, `jsb_ret_f64`, `jsb_ret_str`, `jsb_ret_val` | Sets the function's result; the last call wins and calls after a throw are ignored. `jsb_ret_str` copies NUL-terminated UTF-8 (`NULL` throws a TypeError); `jsb_ret_val` copies the handle (`NULL` means `undefined`). Nothing set means `undefined`. |
| `jsb_new_i32/_u32/_f64/_bool/_null` | Builds a call-scoped Number, Boolean or `null` handle; never NULL. |
| `jsb_new_str(ctx, utf8)` | Builds a String copied from NUL-terminated UTF-8 (borrowed); `NULL` throws a TypeError and returns NULL. |
| `jsb_new_str_len(ctx, utf8, len)` | Builds a String from `len` UTF-8 bytes that may contain embedded NULs and need not be NUL-terminated. `utf8` may be NULL only when `len` is 0; more than `INT32_MAX` bytes is a RangeError. |
| `jsb_new_array(ctx)` | Builds a new empty Array; never NULL. |
| `jsb_new_object(ctx)` | Builds a new empty plain object; never NULL. |
| `jsb_new_buffer_copy(ctx, data, len)` | Builds an ArrayBuffer holding a copy of `len` bytes; the engine owns the copy and the caller keeps `data` (borrowed). `data` may be NULL only when `len` is 0; more than `INT32_MAX` bytes is a RangeError. |
| `jsb_set_index(ctx, obj, i, val)` | Ordinary `obj[i] = val`, so an array grows; a non-object `obj` or NULL `val` is a TypeError. False with the exception pending when the assignment threw. |
| `jsb_get_index(ctx, obj, i)` | Returns `obj[i]` (`undefined` if absent), running getters. A non-object `obj` is a TypeError; NULL is also returned when a getter threw (exception pending). |
| `jsb_is_array(ctx, val)` | `Array.isArray`; NULL is false and it never throws. |
| `jsb_val_to_f64/_i32/_u32(ctx, val, &out)` | ToNumber/ToInt32/ToUint32 of an element; NULL `val` is a TypeError and throws as the `jsb_arg_to_*` calls do. |
| `jsb_val_type(ctx, val)` | Kind of `val` (JS_IsNull/JS_IsUndefined and friends); NULL counts as undefined, and objects, functions, symbols and BigInts are all `JSB_TYPE_OBJECT`. Never throws. |

### Objects, functions and modules

| Call | Contract |
| --- | --- |
| `jsb_set_fn(ctx, obj, name, fn)` | `JS_CFUNC_DEF`: defines `obj[name]` as a function calling `fn`, with `name` as its `name`. The property is writable, configurable and non-enumerable. `fn` must stay valid for the runtime's lifetime. False with the exception pending when the set failed. |
| `jsb_set_fn_n(ctx, obj, name, fn, length)` | As `jsb_set_fn`, and sets the function's `length` to `length` (0..255), like `JS_CFUNC_DEF`'s arity; out of range is a RangeError. The function is non-constructable on V8. |
| `jsb_set_prop(ctx, obj, name, val)` | Ordinary `obj[name] = val`, so setters run. False with the exception pending when it threw. |
| `jsb_get_prop(ctx, obj, name)` | Returns `obj[name]` (`undefined` if absent), running getters. NULL when a getter threw (exception pending). |
| `jsb_define_module(ctx, name, init)` | Makes a fresh object, fills it with `init`, then sets the global `name` (for example `__native_audio`). The global appears only after `init` succeeds, so a failed init leaves no half-built module. False when `init` failed or the global could not be set. |

### Errors

| Call | Contract |
| --- | --- |
| `jsb_throw_type/_range(ctx, fmt, ...)` | Throws a TypeError or RangeError with a printf-style message (truncated to 255 bytes). Only the first throw of a call counts. |
| `jsb_throw_error(ctx, fmt, ...)` | Throws a plain Error. |
| `jsb_throw_internal(ctx, fmt, ...)` | Throws an InternalError; on V8, which has no InternalError, it is an Error whose own `name` is `"InternalError"`, so `String(e)` and `e.name` match and `e instanceof Error` holds. `jsb_throw_internal(ctx, "out of memory")` replaces JS_ThrowOutOfMemory. |
| `jsb_clear_exception(ctx)` | Drops the call's pending exception so the binding can continue (for example to try a fallback argument form); `jsb_ret_*` and `jsb_throw_*` work again. A watchdog termination is never dropped. Does nothing when nothing is pending. |

### Native memory

| Call | Contract |
| --- | --- |
| `jsb_new_external_buffer(ctx, data, len, free_fn, user)` | Wraps `len` bytes of native memory as an ArrayBuffer without copying; on success JS owns the memory and `free_fn(data, len, user)` runs exactly once (after collection or detach, or at `jsb_runtime_destroy`). NULL `data`/`free_fn` is a TypeError, an oversize `len` a RangeError; on failure the caller still owns `data`. See "Buffers". |
| `jsb_adjust_external_memory(ctx, change)` | Adds `change` bytes to the runtime's external-memory accounting (V8's `AdjustAmountOfExternalAllocatedMemory`) and returns the new total. Use it for native memory reachable through a small JS wrapper, adding when made and subtracting when freed. A negative total is refused with -1 and a message. |

### Persistent callbacks

| Call | Contract |
| --- | --- |
| `jsb_callback_new(ctx, fn, &cb)` | Keeps a callable `fn` in a strong `Global`; the handle survives the call. TypeError (false, `cb` zeroed) when `fn` is NULL or not callable. Main thread. |
| `jsb_callback_call(cb, argc, argv, &ret)` | Calls the function with `this` undefined in its own handle scope, using 0..64 `jsb_cval` arguments (`argv` may be NULL at 0). Returns `JSB_OK`, `JSB_ERR_THREW` (reported to the error handler; `ret` is `undefined`), `JSB_ERR_HANDLE` or `JSB_ERR_ARG`. Main thread. |
| `jsb_callback_free(&cb)` | Releases the function and zeroes `*cb`; false for a zero, freed or stale handle. Never from a `jsb_free_fn`. Main thread. |

Types used by callbacks: `jsb_cval` (undefined, null, Boolean, Number or borrowed String
argument), `jsb_result` (type plus Boolean/Number or a caller `str`/`str_cap` buffer with
`str_len`), `jsb_status`, `jsb_type` and `jsb_error_fn`. No `jsb_val` crosses a callback
boundary as an argument or result.

### A complete module

```c
static void fn_add(jsb_ctx *ctx) {
    double a, b;
    if (!jsb_arg_f64(ctx, 0, &a) || !jsb_arg_f64(ctx, 1, &b)) return;
    jsb_ret_f64(ctx, a + b);
}
static bool init_math(jsb_ctx *ctx, jsb_val *m) {
    return jsb_set_fn_n(ctx, m, "add", fn_add, 2);
}
/* in a jsb_runtime_setup callback: */
jsb_define_module(ctx, "__native_math", init_math);
```

## Error conventions

Every native function follows the same pattern, so a binding reads as one shape:

1. **A getter that fails throws and returns false or NULL.** The pending exception is
   already the best message (`TypeError: argument N must be ...`, a conversion error from
   user code, or a RangeError). Return immediately; do not overwrite it with a second,
   vaguer error.
2. **Throw once.** Only the first `jsb_throw_*` of a call counts; later throws are ignored
   so the most specific error reaches JS. After a throw, `jsb_ret_*` is ignored too.
3. **Catching and continuing** is explicit: `jsb_clear_exception` drops a pending
   exception when a binding has a fallback. It never drops a watchdog termination.
4. **Error types keep the pre-port contract.** `TypeError` for a wrong argument or a
   failed brand check, `RangeError` for an out-of-range size or index, `Error` for a
   general failure, and `InternalError` for allocation and internal failures. On V8 the
   last is an `Error` whose own `name` is `"InternalError"`, so `e.name`, `String(e)` and
   `e instanceof Error` all match what the QuickJS binding produced.
5. **Messages name the function and the argument**, for example
   `TypeError: loadFromMemory requires an ArrayBuffer` or
   `RangeError: invalid audio bus: 4`. They are ASCII UTF-8 and are truncated to 255
   bytes by the seam.
6. **Exceptions never cross into C.** A `jsb_callback_call` catches the throw, passes
   `String(exception)` and its `stack` to the runtime's error handler, and returns
   `JSB_ERR_THREW`. It does not reach the enclosing JS caller either, because the
   callback has no JS stack frame of its own.
7. **C never unwinds past a JS call.** A native function that cannot continue returns
   after a throw; it does not `longjmp`, call JS again, or free engine-owned handles.

```c
static void fn_load(jsb_ctx *ctx) {
    char path[4096];
    size_t len = 0;
    if (!jsb_arg_str(ctx, 0, path, sizeof(path), &len)) return;  /* TypeError */
    if (!audio_load(path)) {
        jsb_throw_error(ctx, "load: %s", path);                  /* one throw */
        return;
    }
    jsb_ret_bool(ctx, true);
}
```

`jsb_runtime_setup` and `jsb_runtime_eval` report a pending exception on stderr and
return false; the caller decides whether that is fatal. The engine's error handler
(`src/engine/error_handler.c`) turns exceptions that reach C from game code into a log
line, a `window.onerror` dispatch and optionally a dialog; jsb callback errors are logged
only, because a jsb error handler must not run JS.

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
  total negative is refused with -1 and a message on stderr.

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
  (released at a later V8 GC).
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
DEC-M1-T011). It holds no JS value: it reaches JS only through
`js_engine.h`, so it needs no handle rules of its own.

| Call | Behaviour |
| --- | --- |
| `error_handler_report_js_error(engine, message, stack, fatal)` | Logs `JS Exception: <message>` and `Stack trace:` (scrubbed), runs the shim's `__error_handler_dispatch(message, error)`, then a dialog if `fatal`. `engine` NULL skips dispatch. |
| `error_handler_on_jsb_error(message, stack, user)` | A `jsb_error_fn`: `jsb_set_error_handler(rt, error_handler_on_jsb_error, NULL)` sends jsb callback exceptions to this log instead of stderr. Declared without `jsb.h`, so error_handler has no seam dependency. Whoever owns the runtime installs it; `js_engine_v8.cc` does not yet (not this card's file). |
| `error_handler_attempt_auto_save(engine)` | Runs the shim's `__error_handler_autoSave`; true if it returned non-zero. |

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

`tests/smz/test_error_handler_jsb.c` (ctest `error_handler_jsb`, V8) checks the
log lines, exact delivery of quotes, backslashes, newlines and UTF-8 to window.onerror,
that a message cannot break out of its literal, missing and throwing dispatchers, every
auto-save outcome, and that a throwing jsb callback is logged with its stack.
The older `tests/test_error_handler.c` now reports through the engine-neutral
`error_handler_report_js_error` on V8 without exposing engine handles.

## Ported platform and rendering bindings
Each port keeps its JS API byte-for-byte (names, order, `length`, results, errors) and
adds `bind_<x>_define(jsb_runtime *rt, ...)`; M1-T026 removed the old
`bind_<x>_register(struct JSContext *, ...)` wrappers, and callers pass
`js_engine_get_jsb()` (DEC-M1-T014-JSB-ADOPT). Each has a
`tests/smz/test_bind_<x>_jsb.c` ctest run in V8.

| Binding | Globals | Functions (`length`) | jsb calls used | Buffers | Callbacks |
| --- | --- | --- | --- | --- | --- |
| `bind_platform.c` (M1-T014) | `__native_platform`, `__native_process` | `setFullscreen` (1), `isFullscreen`, `getDisplaySize`, `getDisplayScale`, `getWindowSize`, `setWindowTitle` (1), `quit` (0 for the rest); process: `platform`, `arch`, `engine` strings, `environ`, `hrtime` (0) | `jsb_define_module`, `jsb_runtime_setup`, `jsb_set_fn_n`, `jsb_arg_count`, `jsb_arg_truthy`, `jsb_arg_to_str`/`jsb_free_str`, `jsb_new_object`/`array`/`i32`/`f64`/`str`, `jsb_set_prop`, `jsb_set_index`, `jsb_ret_*`, `jsb_throw_type`, `jsb_throw_internal`, `jsb_engine_version` | none | none |
| `bind_image.c` (M1-T017) | `__native_image` | `decodeApng` (1), `loadImage` (1), `loadImageFromMemory` (2), `getImageInfo` (1), `getImagePixels` (1), `freeImage` (1), `cacheCount` (0), `clearCache` (0) | `jsb_define_module`, `jsb_runtime_setup`, `jsb_set_fn_n`, `jsb_arg_count/val/to_str/to_u32`, `jsb_free_str`, `jsb_new_object/array/i32/u32/null`, `jsb_set_prop/index`, `jsb_get_prop`, `jsb_val_type`, `jsb_ret_val/undefined`, `jsb_throw_type/internal`, `jsb_arg_is_array_buffer`, `jsb_clear_exception` | `jsb_arg_bytes` borrows input bytes during the call; `jsb_new_buffer_copy` gives JS its own RGBA copy | none |
| `bind_renderer.c` (M1-T023) | `__native_renderer` | `init` (2), `shutdown` (0), `resize` (2), `getSize` (0), `beginFrame` (0), `beginFrameTransparent` (0), `endFrame` (0), `createTexture` (2), `updateTexture` (4), `deleteTexture` (1), `setTextureFilter` (2), `createRenderTexture` (2), `deleteRenderTexture` (2), `bindRenderTexture` (3), `unbindRenderTexture` (0), `drawQuad` (11), `drawQuadVertices` (15), `drawQuadVerticesUV` (19), `flush` (0), `setBlendMode` (1), `getDrawCalls` (0), `getQuadCount` (0), `readPixels` (3), `rebindBatch` (0), `setScissor` (4), `clearScissor` (0), `setScreenViewport` (4) | `jsb_define_module`, `jsb_runtime_setup`, `jsb_set_fn_n`, `jsb_arg_count/val/to_i32/to_u32/to_f64/truthy`, `jsb_arg_is_array_buffer`, `jsb_arg_bytes`, `jsb_get_prop`, `jsb_val_type`, `jsb_clear_exception`, `jsb_new_object/i32/u32/null/buffer_copy`, `jsb_set_prop`, `jsb_ret_val/bool/i32/undefined`, `jsb_throw_type/range` | `updateTexture` borrows an ArrayBuffer's bytes only during the call (views are rejected, as upstream did); `readPixels` copies the native RGBA buffer into an engine-owned ArrayBuffer and frees the native one | none |
| `bind_tilemap.c` (M1-T025) | `__native_tilemap` | `drawTiles` (8) | `jsb_define_module`, `jsb_runtime_setup`, `jsb_set_fn_n`, `jsb_arg_count/val/to_i32/to_f64`, `jsb_get_index`, `jsb_val_to_u32/i32`, `jsb_get_prop`, `jsb_val_type`, `jsb_arg_is_array_buffer`, `jsb_arg_bytes`, `jsb_clear_exception`, `jsb_ret_undefined`, `jsb_throw_type/range/internal` | `drawTiles` borrows its element ArrayBuffer only during the call, after every coercion (views are rejected, as upstream's buffer read did) | none |

* **bind_image.** Defines the eight functions in upstream order, with their original
  arities and non-enumerable, writable, configurable properties. Strings and handles
  use coercing reads. `loadImage` clears conversion/path errors and returns null,
  including paths outside the game root. `loadImageFromMemory` accepts an ArrayBuffer;
  `decodeApng` also accepts TypedArray views with their byte offsets. Cache entries
  remain owned by the image loader; APNG frame textures belong to the caller, which
  deletes them through the renderer. All handles are call-scoped and the binding runs
  on the main thread. The `bind_image_jsb` test covers shape, arities, explicit errors,
  headless PNG and APNG results, cache operations and copied pixel storage on the
  V8 engine.
  Pre-port QuickJS measurements are the literal expectations in
  `tests/smz/test_bind_image_jsb.c`: lengths `1,1,2,1,1,1,0,0`; image fields
  `width,height,handle,glTexture`; info fields `width,height,glTexture`; pixel
  buffers have 64 bytes for the synthetic 4×4 PNG. The generated single-frame
  APNG gives `{width:4,height:4,numPlays:3,frames:[{delay:70,glTexture:0}]}`.
  Missing arguments and invalid buffer kinds raise the exact TypeErrors recorded
  in `check_errors`; unknown handles and failed decoding return null, freeing and
  clearing return undefined, and cache counts are numbers. Linking the production
  `dump_surface` inventory path against the pre-port and ported image binding
  produced identical inventories (2,048 lines); `diff -u` produced no output,
  including for image-related rows. Native module rows and lengths are absent
  from that inventory, so the contract test checks them separately.

  **ArrayBuffer checks:** `jsb_arg_is_array_buffer` gives all four buffer
  bindings the engine's intrinsic brand test, so a script cannot shadow
  `byteOffset` to change the answer or make the check throw. `decodeApng`
  still accepts a TypedArray by its `BYTES_PER_ELEMENT` property, which a
  script can shadow, and `jsb_arg_bytes` then accepts any real view; a
  DataView with that property spoofed is therefore still accepted. Property
  reads precede byte borrowing so a getter cannot detach already-borrowed
  storage.

* **bind_renderer.** Defines the 27 functions in upstream order, with their original
  arities and non-enumerable, writable, configurable properties. Every numeric
  argument uses the coercing `jsb_arg_to_*` reads the old `JS_To*` calls used, and
  `setTextureFilter`'s flag uses `jsb_arg_truthy` (`JS_ToBool`). `updateTexture`
  keeps upstream's ArrayBuffer-only rule: TypedArray and DataView arguments are
  rejected with `TypeError: updateTexture: fourth arg must be ArrayBuffer`, and the
  borrowed bytes are used only until the call returns. `readPixels` copies the
  native RGBA buffer into an engine-owned ArrayBuffer and frees the native one.
  The renderer itself is a single C-owned global created lazily by `init` and
  released by `shutdown`; no JS handle or buffer survives a call. The
  `bind_renderer_jsb` test covers names, order, arities, descriptors, every
  no-device result, the device-side size/readPixels path, and the exact TypeError
  and RangeError messages. Pre-port QuickJS measurements are the literal
  expectations in `tests/smz/test_bind_renderer_jsb.c`: lengths
  `2,0,2,0,0,0,0,2,4,1,2,2,2,3,0,11,15,19,0,1,0,0,3,0,4,0,4`; `getSize` is
  `null` without a renderer and `{width,height}` with one; `createRenderTexture`
  is `{fbo:0,texture:0}` without GL; `readPixels(0,2,2)` is a 16-byte
  ArrayBuffer; `getDrawCalls` and `getQuadCount` are `0`. `--dump-surface` lists
  no native module rows (DEC-M0-T009b), so its renderer output is unchanged by
  the port; the contract test is what pins the API.

* **bind_tilemap.** Defines `drawTiles` with its original arity and
  non-enumerable, writable, configurable property. The count, tileset count and
  offsets use the coercing `jsb_arg_to_i32`/`jsb_arg_to_f64` reads, and the
  per-tileset texture id, width and height use `jsb_get_index` plus
  `jsb_val_to_u32`/`jsb_val_to_i32`, in upstream's order. The element buffer
  keeps upstream's ArrayBuffer-only rule (TypedArray and DataView arguments get
  `TypeError: drawTiles: arg 0 must be an ArrayBuffer`) and is read last,
  because earlier coercions can run user JS that detaches it. With no renderer,
  with `count <= 0` or with `numTilesets <= 0`, the result is `undefined`;
  allocation failure keeps `InternalError: out of memory`. Heap tileset storage
  is freed on every exit and no JS handle outlives the call. The
  `bind_tilemap_jsb` test covers the name, arity and descriptor, the no-device
  result, buffer-only rejection, range and conversion errors, and coerced
  arguments with a headless renderer. Pre-port QuickJS measurements are the
  literal expectations in `tests/smz/test_bind_tilemap_jsb.c`; lengths are
  `8`, `Object.getOwnPropertyNames` is `drawTiles`, the descriptor is
  `true,false,true`, and the binding-authored errors are the strings above.
  `--dump-surface` lists no native module rows (DEC-M0-T009b), so its tilemap
  output is unchanged by the port.

* **bind_platform.** The `__native_process` helpers stay inline in `bind_platform.h`
  (`bind_platform_define_process(rt)`), so window-less tests need no SDL; callers pass
  `js_engine_get_jsb()` directly now that the transitional wrapper is gone.
  `bind_platform_define` also defines `__native_process`, as the old register did. The
  `Platform *` is kept in a file static (jsb modules have no user pointer); no jsb handle
  is stored. `engine` is `jsb_engine_version()`, the running engine's version.

## How to add a binding

A binding is one C file pair plus one test. The goal is to keep the JS API identical to
the one Outsider's QuickJS binding exposed, so the shim above it does not change.

1. **Header.** Declare one entry point in `src/bindings/bind_<name>.h`:
   `bool bind_<name>_define(jsb_runtime *rt, ...)`. Document that `rt` is borrowed, the
   function is main-thread only, and what the extra borrowed services are (a `Platform *`
   or `Renderer *`, for example). Include only `jsb/jsb.h`, not a V8 or engine header.
2. **Module init.** Write `static bool <name>_module_init(jsb_ctx *ctx, jsb_val *module)`
   that defines every function with `jsb_set_fn_n`, in the original order and with the
   original `length`. Use `jsb_set_prop` for constants. Return `false` at the first
   failure; `jsb_define_module` then leaves no global behind.
3. **Setup callback.** A `jsb_setup_fn` calls
   `jsb_define_module(ctx, "__native_<name>", <name>_module_init)`. The public entry
   wraps it in `jsb_runtime_setup(rt, <name>_setup, NULL)`.
4. **Arguments.** Read in the same order the QuickJS binding did. Use the typed getters
   (`jsb_arg_i32`, `jsb_arg_f64`, `jsb_arg_bool`, `jsb_arg_str`, `jsb_arg_obj`) when the
   old code required that type, and the coercing readers (`jsb_arg_to_i32/u32/f64/str`,
   `jsb_arg_truthy`) when it called `JS_To*`. Read slice buffers last, after every
   coercion that could run game JS and detach them.
5. **Results.** Set one result with `jsb_ret_*` or build an object/array with the
   `jsb_new_*` and `jsb_set_*` calls. A function that sets nothing returns `undefined`.
6. **Errors.** Throw with `jsb_throw_type/_range/_error/_internal` and return; follow
   "Error conventions" above. Never keep a `jsb_val`.
7. **Register.** Call `bind_<name>_define(js_engine_get_jsb(engine), ...)` where the other
   modules are defined, on the main thread after the engine exists.
8. **Test.** Add `tests/smz/test_bind_<name>_jsb.c` and a
   `tests/smz/<name>.cmake` fragment. Pin names, order, `length`, descriptors, results,
   every binding-authored message and one real service round trip where possible; keep
   the pre-port capture as the literal expectation.
9. **Document.** Add the row to the ported-bindings table, record the parity evidence,
   and add one `CHANGELOG.md` line under "Unreleased". No new dependency is needed.

```c
/* src/bindings/bind_math.c (sketch) */
static void js_add(jsb_ctx *ctx) {
    double a = 0, b = 0;
    if (!jsb_arg_f64(ctx, 0, &a) || !jsb_arg_f64(ctx, 1, &b)) return;
    jsb_ret_f64(ctx, a + b);
}

static bool math_module_init(jsb_ctx *ctx, jsb_val *module) {
    return jsb_set_fn_n(ctx, module, "add", js_add, 2);
}

static bool math_setup(jsb_ctx *ctx, void *user) {
    (void)user;
    return jsb_define_module(ctx, "__native_math", math_module_init);
}

bool bind_math_define(jsb_runtime *rt) {
    if (!rt) return false;
    return jsb_runtime_setup(rt, math_setup, NULL);
}
```

## Threading rules

The runtime is single-threaded. The main thread creates the SDL window, the isolate and
the context, evaluates scripts, drives the frame loop and owns every GL object. Only that
thread may:

* enter the isolate and create, use or destroy a `Local`, a `Global`, a `HandleScope`, a
  `jsb_ctx` or a `jsb_val`;
* call any jsb function except `jsb_engine_version`;
* run JS callbacks (`jsb_callback_call`), timers, rAF, audio `onended` or video events;
* touch a GL name, or call `glDelete*` for a queued name.

Other threads only queue work; the main loop consumes it:

* The audio mixer thread in `src/audio/audio_engine.cpp` consumes PCM under its mutex and
  never calls JS or GL.
* The watchdog thread in `src/engine/js_watchdog.cc` may call
  `Isolate::RequestInterrupt`, which runs its handler on the main thread.
* The optional control reader in `src/engine/game_loop.c` reads stdin and appends lines
  under a mutex; the main loop evaluates them.
* `game_loop_request_stop` only writes a `bool`; it is the one game-loop call documented
  as safe from another thread.
* A `jsb_free_fn` is the only jsb callback that may run off the main thread: V8 may invoke
  it on a GC helper thread during collection, so it releases memory only and calls no jsb,
  V8 or GL function and takes no lock the main thread may hold.

```c
/* A worker thread produces commands; it never touches the engine. */
void audio_thread(void) {
    QueueItem item = {.kind = AUDIO_ENDED, .voice = id};
    queue_push(&audio_events, &item);
}

/* The main loop drains the queue and is the only thread that runs the callback. */
void main_loop_drain_audio(void) {
    QueueItem item;
    while (queue_pop(&audio_events, &item)) {
        jsb_cval arg = {JSB_TYPE_NUMBER, false, (double)item.voice, NULL};
        jsb_callback_call(g_onended, 1, &arg, NULL);
    }
}
```

<a id="handle-rules"></a>
## Lifetime and handle rules (§15.8)

These rules make handle lifetime deterministic in a C runtime. They apply to `jsb`,
`js_engine_v8.cc` and every binding; the GL half applies to the renderer and canvas
bindings and is expanded in `webgl.md` when M2 adds real WebGL. A short comment in every
file that touches handles or GL objects points here (§15.8).

### A `Local<T>` lives only inside its `HandleScope`

A `Local<T>` is a temporary handle, valid only until the `HandleScope` it was created in
ends. **Never store a `Local` in a C struct, a global or a static.**

```cpp
// Wrong: `cached` outlives the scope that made it, so the slot is reused.
static v8::Local<v8::Object> cached;

// Right: keep a Global (next rule), or copy the C data you actually need.
static v8::Global<v8::Object> cached_global;
```

### Open a `HandleScope` per call and per loop iteration

Every native function called from JS, and every callback we drive (timers, rAF, audio
and video events), opens its own `HandleScope`. A loop that creates many handles opens
one scope per iteration so handles do not pile up.

```cpp
void on_timer(v8::Isolate *isolate) {
    v8::HandleScope call_scope(isolate);   // one per callback
    // ...
}

for (int i = 0; i < count; i++) {
    v8::HandleScope iteration(isolate);    // many values, one scope each
    v8::Local<v8::Value> v = make_value(isolate, i);
    consume(isolate, v);
}
```

### Leave a scope only through `EscapableHandleScope::Escape`

Return a `Local` out of a scope with `EscapableHandleScope::Escape`. Nothing else is
safe: returning a plain `Local` from a helper whose scope has ended hands back a dead
slot.

```cpp
v8::Local<v8::Value> make_object(v8::Isolate *isolate) {
    v8::EscapableHandleScope scope(isolate);
    v8::Local<v8::Object> object = v8::Object::New(isolate);
    return scope.Escape(object);           // the only safe way out
}
```

### Keep a JS object in a `v8::Global<T>` with one owner

To keep a JS object beyond the current call, use `v8::Global<T>`. Each Global has exactly
one owner, and the owner calls `Reset()` when done. Leaked Globals are leaked memory, and
they also keep the object (and anything it references) alive.

```cpp
struct AudioVoice {
    v8::Global<v8::Function> onended;      // owned by this voice
};

voice->onended.Reset(isolate, local_fn);   // take ownership
// ... frames later, when the voice can no longer end:
voice->onended.Reset();                    // release exactly once
```

jsb wraps this rule in `jsb_callback`: `jsb_callback_new` takes the Global,
`jsb_callback_call` uses it, and `jsb_callback_free` resets it. `jsb_runtime_destroy`
resets any that remain.

### Weak Globals with finalizers only release native resources

Weak Globals with finalizers are used only to release native resources. The finalizer
does not run JS and does not touch GL, because it may run during a GC, on a GC helper
thread, while the main thread holds locks.

```cpp
static void on_texture_collected(const v8::WeakCallbackInfo<Texture> &info) {
    delete info.GetParameter();            // release C memory only
}

v8::Global<v8::Object> wrapper;
wrapper.SetWeak(texture, on_texture_collected,
                v8::WeakCallbackType::kParameter);
```

For GL names the finalizer must not call GL either; it queues the name (see "A GC
finalizer queues GL names" below).

### Say who owns every `ArrayBuffer` backing store

Every `ArrayBuffer` backing store states who owns its memory: V8, or our allocator. Report
native memory that JS keeps alive through another object with
`AdjustAmountOfExternalAllocatedMemory`, so GC timing stays honest. jsb exposes both
forms: `jsb_new_buffer_copy` gives V8 the copy, `jsb_new_external_buffer` hands native
memory to JS with a `free_fn`, and `jsb_adjust_external_memory` reports memory behind a
small wrapper.

```c
/* V8 owns this copy; the caller keeps `data`. */
jsb_val *copy = jsb_new_buffer_copy(ctx, data, len);

/* JS owns this native block; free_pixels runs exactly once. */
jsb_val *external = jsb_new_external_buffer(ctx, pixels, bytes,
                                            free_pixels, NULL);

/* A texture behind a tiny JS wrapper: report the real cost. */
jsb_adjust_external_memory(ctx, (int64_t)bytes);   /* when created */
/* ... when the texture is freed ... */
jsb_adjust_external_memory(ctx, -(int64_t)bytes);
```

Do not report an external buffer twice: V8 already counts `jsb_new_external_buffer`
memory.

### Only the main thread enters the isolate

Only the main thread enters the isolate. Other threads post work to it; they never call
V8, never touch a `Local`, a `Global` or a `jsb_ctx`. This is why the audio mixer thread
and the watchdog timer call host services or `Isolate::RequestInterrupt`, not JS.

```c
/* Wrong: a worker thread entering the isolate. */
void audio_thread(void) {
    v8::Isolate *isolate = run->isolate;
    v8::HandleScope scope(isolate);        // never on this thread
}

/* Right: the worker queues work; the main loop runs it on its next turn. */
void audio_thread(void) {
    queue_event(AUDIO_ENDED, voice_id);
    /* game_loop_step calls jsb_callback_call on the main thread. */
}
```

### Every GL object belongs to one context and the main thread

Every GL object (texture, buffer, framebuffer, renderbuffer, program, shader, VAO,
query, sampler) belongs to one context and is only touched on the main thread.

```c
/* The renderer binding runs inside a JS call on the main thread, so it may
   create the texture directly; no other thread may use `name`. */
GLuint name = 0;
glGenTextures(1, &name);
```

### A JS WebGL object wraps exactly one GL name and owns it

Each JS WebGL object (`WebGLTexture` and so on) wraps exactly one GL name and **owns**
it. Nothing else deletes that name.

```c
typedef struct {
    GLuint name;      /* owned by this wrapper */
    bool alive;
} WebGLTexture;
```

### A GC finalizer queues GL names; the frame drains the queue

The GL name is freed when JS calls `delete*()`, or when the wrapper is garbage-collected.
A GC finalizer never calls GL directly: it **queues** the name, and the queue is drained
at the end of the frame on the main thread.

```c
static void texture_finalizer(const v8::WeakCallbackInfo<WebGLTexture> &info) {
    queue_gl_delete(GL_TEXTURE, info.GetParameter()->name);   /* no GL here */
    delete info.GetParameter();
}

/* end of frame, main thread: */
drain_gl_delete_queue();                                     /* calls glDelete* */
```

### After `delete*()` the wrapper is dead and reports Chrome's error

After `delete*()`, the wrapper is marked dead, and later use reports the same WebGL
error that Chrome does.

```js
gl.deleteTexture(texture);
gl.bindTexture(gl.TEXTURE_2D, texture);  // INVALID_OPERATION, as in Chrome
```

### On context loss, wrappers are invalidated but names are not deleted

On context loss, every wrapper is marked invalid and GL names are forgotten, not
deleted.

```c
void on_context_lost(Renderer *renderer) {
    for (size_t i = 0; i < renderer->texture_count; i++) {
        renderer->textures[i].alive = false;
        renderer->textures[i].name = 0;      /* forget; do not glDelete */
    }
}
```

### Shader errors log the shader source line

Shader compilation errors are logged with the shader source line, so plugin authors can
see them.

```c
if (!compiled) {
    log_error("%s:%d: shader compile failed: %s",
              shader_name, source_line, info_log);
}
```

### How jsb applies the V8 rules

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

```c
/* jsb_ctx and jsb_val are call-scoped: use them, never store them. */
static void fn_identity(jsb_ctx *ctx) {
    jsb_val *arg = jsb_arg_val(ctx, 0);   /* valid until fn_identity returns */
    jsb_ret_val(ctx, arg);                /* the same ctx also owns the result */
}

/* To keep a function for a later event, make a Global-backed handle. */
jsb_callback cb = {0};
if (jsb_callback_new(ctx, jsb_arg_val(ctx, 0), &cb)) {
    /* the owner later calls jsb_callback_call(cb, ...) and jsb_callback_free(&cb) */
}
```

## Runtime stack

V8 engine initialization measures the available stack below its current main
thread frame before creating the first isolate. Linux uses pthread stack bounds,
macOS uses pthread stack address/size, and Windows uses the virtual allocation
base. It passes `--stack-size` in KiB, subtracting 25% of available space or
256 KiB, whichever is larger, for native calls and exception reporting. The
selected value is printed at startup. An unavailable or too-small stack fails
initialization loudly. JS, SDL and the event loop stay on the process main thread;
no runtime thread is created. Embedders must initialize the engine on that thread
before creating other jsb runtimes.
`stack_size` checks repeated catchable recursion overflow and engine recovery.

## js_engine on V8
`src/engine/js_engine_v8.cc` implements `src/engine/js_engine.h` on V8 (§4.1 item 3,
card M1-T007). CMake builds `smz_js_engine_v8` for the runtime and every engine
consumer. Callers use `js_engine.h` and `jsb.h`; no raw engine context is exposed.

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
- **Errors** are reported as engine-neutral strings: the exception's `String()` form, then its
  `stack`, both at level `error` through the console callback (or stderr).
- **Microtasks**: `MicrotasksPolicy::kExplicit`; `js_engine_execute_pending_jobs` runs one
  checkpoint, then reports rejections still unhandled (through
  `__error_handler_dispatchRejection` when the shim is loaded). Its count is the number
  of promise reaction jobs, taken from a promise hook; `queueMicrotask` callbacks are
  not counted. It returns -1 if a microtask threw uncaught.
- **Code cache**: V8 code caches under `$XDG_CACHE_HOME/smzuggler-rt/v8cache/`, never
  a `.jsc` beside the script; see "Code cache" below.
- **Watchdog**: see "Watchdog" below.
- **Handles** (§15.8): every entry point opens its own `HandleScope`; the engine owns
  exactly two kinds of `Global` (its context and pending rejections) and Resets them in
  `js_engine_shutdown` before the isolate is disposed of.

### Code cache
Card M1-T012 (§4.1 item 4, §5.6). `js_engine_eval_file_cached`, and `js_engine_eval_file`
once a cache dir is set, keep V8's compiled code in `<dir>/<sha256>.bin`.
The `js_engine_on_v8` variant of `tests/test_js_engine.c` asserts that layout:
it looks for `.bin` files under the configured (and the default XDG) cache
directory, never a `.jsc` beside a script including a `..` script path, and
expects a source edit to take effect even when the mtime is preserved.

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
  so key, directory and file handling are independent of V8 handles.
- **Handles**: no `Local` outlives `compile_and_run`'s `HandleScope`; the `CachedData`
  belongs to the `ScriptCompiler::Source` and is deleted with it.

### Watchdog
`js_engine_set_watchdog(engine, ms)` (from `--watchdog <ms>`) stops scripts that run
longer than `ms` since the last arm (§4.1 item 3, card M1-T008, DEC-M1-T008). The
outermost eval entry point and each microtask checkpoint arm it, and the game loop arms
it once per frame.

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

`tests/smz/test_js_watchdog.c` (ctest `js_watchdog`) checks that `while(true){}`
under 200 ms stops within 400 ms naming `spin.js:2`, that a `catch` cannot swallow it,
that the next eval succeeds, that re-arming per eval and idle time do not fire, that a
looping promise job is stopped too, and that no thread exists while the limit is off.

`tests/smz/test_js_engine_v8.c` (ctest `js_engine_stack`, V8) checks that a
throw in `foo.js` line 3 reports `foo.js:3` from a file, a string and a SyntaxError;
also the game-relative name and that `js_engine_get_jsb()` sees the engine's
globals. The unchanged `tests/test_js_engine.c` also runs on V8 as ctest
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
- **Cost.** Each frame evaluates five short scripts. V8 reuses its compilation cache
  except for the rAF call, whose timestamp changes the source. Each eval also
  re-arms the watchdog, so its
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

The exact task and microtask orders are recorded in
[DEC-M1-T010](decisions/DEC-M1-T010.md). `tests/smz/test_microtasks.c` runs the real
loop and DOM input dispatcher, registering startup timers and rAF callbacks before
any frame. It also checks promises queued before ticks and ticks queued by promises.

`tests/smz/test_game_loop_js.c` (ctest `game_loop_js`, V8) runs the real
`game_loop.c` with the platform, renderer, audio, video and log calls stubbed. It checks
the phase order and arguments, both job drains, a growing Number timestamp, the
first-frame snapshot, exception logging and dispatch, the `__nw_hostClose` outcomes,
exit-probe values, errors and escaping, and control-mode replies (synchronous, promise,
unknown command) through a stdin pipe.

## Tests
`tests/smz/test_jsb_core.c` (ctest `jsb_core`, V8) defines a module from C, calls
every getter, return and throw from JS, and checks values, wrong-type and
missing-argument TypeErrors, the string-size RangeError and module failure.

`tests/smz/test_jsb_buffers.c` (ctest `jsb_buffers`, V8) checks
`jsb_arg_bytes` on ArrayBuffers, TypedArrays and DataViews with byte offsets, the
TypeError for non-buffers and detached buffers, C writes seen by JS and JS writes seen by
C through an external buffer, that its free function runs exactly once after `gc()`
(from `jsb_set_flags("--expose-gc")`) and once more at destroy for a buffer still alive,
and the `jsb_adjust_external_memory` totals.

`tests/smz/test_jsb_callbacks.c` (ctest `jsb_callbacks`, V8) keeps a
function in a native call and calls it from C after that scope ended, checks every
argument and result type and string truncation, that exceptions (Errors and thrown
primitives, from the host and from inside a native call) reach the error handler with
message and stack and return `JSB_ERR_THREW`, that a freed function is collected (an
external buffer hung on it is freed only after `jsb_callback_free` and `gc()`), that
freed and stale handles and bad arguments are refused, and that a callback may free its
own handle.


### M1-T020 video parity report

`bind_video_define(js_engine_get_jsb(engine))` defines the module on the
engine's V8 context through `smz_js_engine_v8`.
All values are call-scoped, the coerced path is freed after `video_open`, and
video events remain polled by the DOM shim; no persistent callback is needed.
The methods come from `jsb_set_fn_n`, so they are non-enumerable. Missing required
arguments throw a `TypeError` naming the function and the missing value. That
is a deliberate, documented deviation: the pre-port QuickJS binding returned
its bare internal uninitialized exception sentinel (`typeof` unknown,
`String(e) === "[uninitialized]"`), which V8 cannot represent. Throwing a
`TypeError` is the nearest safe behaviour and keeps the failure explicit for
callers; the parity test pins each message.

The original binding was compiled separately against the gate-built QuickJS
library, using the same API test with registration switched to the original
`bind_video_register`. The initial GCC and Clang builds failed on the cached
QuickJS const assignment at `quickjs.c:26366`; the standard GCC gate build
supplied its existing toolchain settings and produced the dependency library.
No dependency source or warning policy was changed by this card.

| Pre-port binary check | Captured expected value |
| --- | --- |
| Own names, in order | `open,close,play,pause,setVolume,setLoop,setVisible,getState` |
| Function lengths, same order | `1,1,1,1,2,2,2,1` |
| Function descriptors | All have type `function`, matching names, writable/configurable and non-enumerable; own names `length,name` |
| Failed open | Number `0` for missing files, null and undefined paths |
| Invalid handle | `getState` returns null; close/play/pause/setVolume/setLoop/setVisible return undefined |
| Synthetic fixture state | Own names `playing,paused,ended,time,duration,width,height`; initial values false/false/false/0, numeric duration between 0.8 and 1.2 seconds, dimensions 64 by 48 |
| Playback and cleanup | play sets playing; pause sets paused and clears playing; close makes state null; setters return undefined |
| Missing open path | `TypeError: open requires a path` |
| Other missing arguments | Deliberate deviation: each throws `TypeError` (`close requires a handle`, `play requires a handle`, `pause requires a handle`, `setVolume requires a handle and a volume`, `setLoop requires a handle and a flag`, `setVisible requires a handle and a flag`, `getState requires a handle`) instead of the unrepresentable sentinel |
| Coercion errors | User-thrown `Error: path` and `RangeError: number` propagate; Symbol conversion throws TypeError (engine-specific wording) |
| Coercion order | Handle before volume; looping and visibility use truthiness without numeric conversion |

The pre-port runtime was relinked with the original binding object, and both
binaries ran `--dump-surface`. The complete pre/post inventories compare equal:
`diff -u` emits no lines and exits 0, including the video inventory. As specified
by DEC-M0-T009b, native video rows and function lengths are excluded from that
inventory; the ctest supplies those assertions.

### Canvas2D port baseline (M1-T021)

`bind_canvas2d_define` defines the module on the supplied engine runtime. All calls run
on the main thread; no jsb handle or borrowed buffer survives a native call.
The ArrayBuffer-only check is the shared `jsb_arg_is_array_buffer` brand test:
a TypedArray, DataView, plain or spoofed object is rejected, and the binding
throws upstream's own TypeError. Numeric/string coercions run in upstream order
before buffer validation, so user code cannot detach an already borrowed pointer.
`jsb_set_fn_n` makes all 27 methods non-enumerable.

The new `bind_canvas2d_jsb` ctest uses real CPU Canvas2D services without a graphics
device and calls `bind_canvas2d_define(js_engine_get_jsb(engine))` on V8. Its literal expected
errors and values are the pre-port binary baseline, including every missing-arg
TypeError; see `tests/smz/test_bind_canvas2d_jsb.c` for the full byte strings.

| Check | Pre-port expected value |
| --- | --- |
| Own names, in order | `create,destroy,resize,getSize,setFillColor,setStrokeColor,setGlobalAlpha,setCompositeOp,setLineWidth,setTextAlign,setTextBaseline,save,restore,fillRect,clearRect,strokeRect,drawImage,drawImageHandle,getImageData,putImageData,getPixels,loadFont,loadFontFile,setFont,fillText,strokeText,measureText` |
| Function lengths, same order | `2,1,3,1,5,5,2,2,2,2,2,1,1,5,5,5,12,10,5,6,1,2,2,3,4,4,2` |
| Types and descriptors | All `function`; writable/configurable, non-enumerable; function own names `length,name` |
| Create / size / pixel copies | Numeric handle; `{"width":2,"height":3}`; ArrayBuffer of 24 bytes; getImageData returns an ArrayBuffer |
| Missing handle | getSize/getPixels: `null`; measureText: `0`; getImageData: zeroed RGBA bytes |
| Empty rectangle / missing font | getImageData: `null`; loadFont/loadFontFile: `false` |
| Other functions | `undefined`; putImageData followed by getImageData preserves `1,2,3,4` |
| Missing arguments | `TypeError` with the original per-method `requires` message (all 27 in the test) |
| Wrong buffer | `TypeError: <method>: second arg must be ArrayBuffer` for drawImage, putImageData and loadFont |
| Invalid dimensions | Original RangeError dimensions/buffer-mismatch/64-MiB-limit messages, including dimensions and byte count |
| Throwing conversions | Original user exception type/message, before buffer validation |

`--dump-surface` has no native canvas2d rows and does not report function lengths;
the ctest supplies that coverage. The pre/post inventory comparison is recorded
with the validation results for this card.
### Effekseer parity capture (M1-T022)

The pre-port QuickJS binding has 26 own methods, in the order pinned by
`test_bind_effekseer_jsb.c`, with lengths
`3,0,0,2,1,1,4,1,1,0,4,4,4,2,1,1,1,1,0,1,0,1,1,0,0,0`.
Every method is a function with a writable, configurable, non-enumerable property.
Without initialization, `isInitialized`, `isLoaded` and `exists` return false;
`load`, `getLoadedCount` and `getPlayingCount` return 0; `play` returns -1.
Other operations return undefined. Tracking-backend `init` returns true and
`shutdown` returns undefined. No test opens a graphics device.

Missing `load` arguments throw `TypeError: load requires a path argument`.
Non-Array matrices throw `TypeError: setProjectionMatrix expects a 16-element array`
or `TypeError: setCameraMatrix expects a 16-element array`. Arrays of any length
are accepted; missing entries coerce to NaN, and all 16 entries are read in order.
Numeric conversion and element getter exceptions retain their type and message.
Path resolution and path string conversion exceptions are cleared and return 0;
scale conversion errors still propagate. Paths are allocated by jsb and freed
before backend loading; no JS handles or callbacks survive a call.

The new test calls `bind_effekseer_define(js_engine_get_jsb(engine))` on V8 and pins the capture above without opening a device.
The `--dump-surface` effekseer row is unchanged (empty diff); native method arities
are absent from that inventory, so the test above supplies the parity evidence.

### Filters parity capture (M1-T024)

`bind_filters_define` defines `__native_filters` on the supplied V8 runtime. All
handles are call-scoped; the mat4 bytes are borrowed only during the call, and
no callback is kept.

Upstream exposed the compiled shader IDs as ordinary properties of the module.
`init()` still publishes them, through a short assignment script evaluated on
the runtime kept from `bind_filters_define`, because jsb reaches objects only
through the call or setup context. Without a graphics device every compile
returns 0, so `init()` returns false and the five IDs stay 0.

The pre-port binary was run with the same script as `tests/smz/test_bind_filters_jsb.c`
(registration switched back to the original `bind_filters_register`), and its
values are pinned there:

| Check | Pre-port expected value |
| --- | --- |
| Own names, in order | `init,shutdown,compileShader,deleteShader,beginFilter,setUniform1f,setUniform2f,setUniformTexture,setUniform4f,setUniformMat4,drawQuad,endFilter,colorMatrixShader,maskShader,blurShader,alphaShader,colorFilterShader` |
| Function lengths | `0,0,2,1,4,3,4,4,6,3,0,0` |
| Attributes | Functions are writable, configurable and non-enumerable with own names `length,name`; the five shader IDs are writable, enumerable Numbers starting at 0 |
| No device | `init()` false; `compileShader` 0; `shutdown`, `deleteShader`, `beginFilter` and every uniform/draw call return undefined |
| Mat4 forms | Float32Array, ArrayBuffer, a plain 16-element Array, `null`, `{}` and a DataView all return undefined; only the first three reach the renderer |
| Missing arguments | `TypeError: compileShader requires vertSrc and fragSrc`; `TypeError: beginFilter requires shaderId, inputTexture, w, h` |
| Unconvertible sources | A Symbol vertex source is ignored and returns 0; an unconvertible fragment source throws `TypeError: compileShader: fragSrc required` |
| Coercion failures | Symbol and throwing `valueOf`/`toString`/element getter errors keep their original type and message |

`--dump-surface` lists no native filters rows, so pre/post inventories compare
equal (empty diff); this ctest supplies the API evidence, including function
lengths, which DEC-M0-T009b leaves out of the inventory.
