/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: remove former engine migration contracts.
 * Modified 2026-11-04: value, coercion, array, error and length calls the
 * binding ports need (card M1-T007b).
 * Modified 2026-11-05: share the ArrayBuffer brand check and define native
 * functions non-enumerably (card M1-T026b).
 *
 * jsb: the small C seam through which native bindings talk to the JS engine.
 *
 * Why: The bindings (src/bindings/) are C and need opaque engine handles.
 * spec section 4.1 moves them to V8 by near-mechanical rewriting onto this
 * header, so upstream fixes to those bindings still re-apply with small
 * diffs. It is not an engine abstraction: its one real implementation is
 * src/jsb/jsb_v8.cc.
 * See docs/js-binding.md for the full API guide.
 *
 * Rules a reader must not break (spec section 15.8, docs/js-binding.md):
 * - This header is plain C11 and never includes C++ or V8 headers.
 * - A jsb_ctx and every jsb_val obtained through it are valid only until the
 *   native call (or setup callback) that received the ctx returns. Never
 *   store either in a struct, a global or a static. To keep a JS function,
 *   make a jsb_callback, which has one owner who frees it.
 * - Strings passed in are copied; strings read out go into caller buffers.
 * - Only the main thread, the one that created the runtime, calls jsb.
 */
#ifndef SMZ_JSB_H
#define SMZ_JSB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Context of one native call or setup callback; opaque, call-scoped. */
typedef struct jsb_ctx jsb_ctx;

/* A JS value handle; opaque, valid only while its jsb_ctx is. */
typedef struct jsb_val jsb_val;

/* An engine instance (V8 isolate plus one context); owned by the host. */
typedef struct jsb_runtime jsb_runtime;

/* A native function callable from JS. It reads arguments with jsb_arg_*,
 * sets its result with jsb_ret_* (undefined when it sets none) and reports
 * errors with jsb_throw_*. */
typedef void (*jsb_fn)(jsb_ctx *ctx);

/* Fills `module`, a fresh object, with functions and properties. Returns
 * false (normally after jsb_throw_*) to abort jsb_define_module. */
typedef bool (*jsb_module_init)(jsb_ctx *ctx, jsb_val *module);

/* Runs host setup (for example jsb_define_module calls) with a ctx. `user`
 * is passed through untouched. Returns false to report failure. */
typedef bool (*jsb_setup_fn)(jsb_ctx *ctx, void *user);

/* Releases the native memory behind a jsb_new_external_buffer buffer.
 *
 * `data` and `len` are what was passed to jsb_new_external_buffer, `user`
 * its pass-through pointer. Runs exactly once, when no JS object uses the
 * memory any more. On V8 that may be on a GC helper thread and during
 * garbage collection, so it must only release memory: no jsb or engine
 * calls, no locks the main thread may hold. */
typedef void (*jsb_free_fn)(void *data, size_t len, void *user);

/* ---- Runtime (host side) ---------------------------------------------- */

/* Passes command-line style engine flags (space-separated, for example
 * "--expose-gc") for every runtime of the process.
 *
 * Why: tests force garbage collection through `gc()`, and V8 freezes its
 * flags once it starts, so they must be set before it does. `flags` is
 * borrowed. Returns false, after printing why, when an engine has already
 * started (the flags would be ignored or crash V8).
 * Main thread only. */
bool jsb_set_flags(const char *flags);

/* Creates an engine instance with one global context.
 *
 * Why: bindings need a live engine to be defined in and tested against;
 * js_engine_v8.cc creates its runtime through this entry point.
 * `argv0` is the program path and `icu_data_path` the path of icudtl.dat
 * (both borrowed, not kept). The first call initialises the engine for the
 * whole process, which is never undone. Returns NULL and prints the reason
 * on stderr if the engine cannot start. Main thread only; free with
 * jsb_runtime_destroy. */
jsb_runtime *jsb_runtime_create(const char *argv0, const char *icu_data_path);

/* Destroys `rt` and every JS object it owns; NULL is ignored. Any native
 * pointer the bindings stashed in JS objects is not freed by this. */
void jsb_runtime_destroy(jsb_runtime *rt);

/* Calls `fn(ctx, user)` inside a fresh handle scope of `rt`, so host code
 * can define modules. Returns fn's result; returns false if fn left a JS
 * exception pending, after printing it on stderr. */
bool jsb_runtime_setup(jsb_runtime *rt, jsb_setup_fn fn, void *user);

/* Compiles and runs `source` (UTF-8, NUL-terminated) as a classic script.
 *
 * `file_name` names the script in stack traces. When `out` is not NULL the
 * result, converted with String(), or on an exception the thrown value's
 * String() form, is written to it as UTF-8, truncated to `out_cap` - 1
 * bytes and always NUL-terminated if `out_cap` > 0. Returns true when the
 * script completed, false when it threw. Arguments are borrowed. */
bool jsb_runtime_eval(jsb_runtime *rt, const char *source,
                      const char *file_name, char *out, size_t out_cap);

/* ---- Arguments --------------------------------------------------------- */
/* Every typed getter takes a 0-based `index`. A missing argument counts as
 * undefined. On a wrong type it throws a TypeError naming the argument and
 * returns false (or NULL); the native function must then return at once. */

/* Returns how many arguments the JS caller passed (0 outside a call). */
int jsb_arg_count(jsb_ctx *ctx);

/* Reads a Number argument converted like ToInt32 (wraps modulo 2^32, NaN
 * becomes 0). Anything but a Number, including undefined, is a TypeError. */
bool jsb_arg_i32(jsb_ctx *ctx, int index, int32_t *out);

/* Reads a Number argument as a double. Non-Numbers are a TypeError. */
bool jsb_arg_f64(jsb_ctx *ctx, int index, double *out);

/* Reads a Boolean argument. Non-Booleans (no truthiness) are a TypeError. */
bool jsb_arg_bool(jsb_ctx *ctx, int index, bool *out);

/* Copies a String argument as UTF-8 into `buf` and NUL-terminates it.
 *
 * `cap` is the size of `buf` in bytes. `len` (may be NULL) receives the
 * byte length without the NUL; the string may contain embedded NULs. A
 * non-String is a TypeError; a string needing more than `cap` - 1 bytes is
 * a RangeError, and nothing is written. The caller owns `buf`. */
bool jsb_arg_str(jsb_ctx *ctx, int index, char *buf, size_t cap,
                 size_t *len);

/* Returns an Object argument (functions and arrays count), or NULL after a
 * TypeError for anything else, null included. */
jsb_val *jsb_arg_obj(jsb_ctx *ctx, int index);

/* Returns an argument of any type, undefined when missing. Never throws. */
jsb_val *jsb_arg_val(jsb_ctx *ctx, int index);

/* Gives direct access to the bytes of an ArrayBuffer, TypedArray or
 * DataView argument, without copying.
 *
 * Why: Outsider's bindings hand pixel, audio and file data to C through
 * JS_GetArrayBuffer; copying those per call would cost frames. `*data`
 * points at the first byte of the view (byte offset applied) and `*len` is
 * the view's length in bytes, not elements; `*data` may be NULL when `*len`
 * is 0. The memory stays owned by JS: it is valid only until the native call
 * returns, and only while no JS code runs in between (JS could detach or
 * resize the buffer). Writes through `*data` are visible to JS. Any other
 * type is a TypeError; a detached buffer, or a view on one, is a TypeError
 * too. Both return false. SharedArrayBuffer is not accepted. */
bool jsb_arg_bytes(jsb_ctx *ctx, int index, uint8_t **data, size_t *len);

/* Returns whether argument `index` is a plain ArrayBuffer, not a TypedArray,
 * DataView or SharedArrayBuffer.
 *
 * Why: several ports must keep Outsider's ArrayBuffer-only rule even though
 * jsb_arg_bytes also accepts views. This is the engine's brand check, so it
 * never runs a `byteOffset` getter that game code could shadow or make
 * throw. Never throws; a missing or non-buffer argument is false. The bytes,
 * when needed, still come from jsb_arg_bytes. */
bool jsb_arg_is_array_buffer(jsb_ctx *ctx, int index);

/* ---- Returns ----------------------------------------------------------- */
/* Each sets the native function's result; the last call wins. They do
 * nothing once the call has thrown, and outside a native call. */

/* Sets the result to undefined, the default when nothing is set. */
void jsb_ret_undefined(jsb_ctx *ctx);
/* Sets a Boolean result. */
void jsb_ret_bool(jsb_ctx *ctx, bool value);
/* Sets an integer Number result. */
void jsb_ret_i32(jsb_ctx *ctx, int32_t value);
/* Sets a Number result. */
void jsb_ret_f64(jsb_ctx *ctx, double value);
/* Sets a String result copied from NUL-terminated UTF-8 `utf8` (borrowed).
 * Invalid UTF-8 becomes U+FFFD; NULL throws a TypeError. */
void jsb_ret_str(jsb_ctx *ctx, const char *utf8);
/* Sets `val` (from the same ctx) as the result; NULL means undefined. */
void jsb_ret_val(jsb_ctx *ctx, jsb_val *val);

/* ---- Errors ------------------------------------------------------------ */

/* Throws a TypeError with a printf-style message (truncated to 255 bytes).
 * Only the first throw of a call counts; later throws are ignored so the
 * first, most specific error reaches JS. Return right after throwing. */
void jsb_throw_type(jsb_ctx *ctx, const char *fmt, ...);

/* Throws a RangeError; otherwise as jsb_throw_type. */
void jsb_throw_range(jsb_ctx *ctx, const char *fmt, ...);

/* ---- Objects and modules ---------------------------------------------- */

/* Returns a new empty plain object (like `{}`). Never NULL. */
jsb_val *jsb_new_object(jsb_ctx *ctx);

/* Defines `obj[name]` as a JS function that calls `fn`.
 *
 * Why: this is the JS_CFUNC_DEF replacement. `name` (UTF-8, copied) is also
 * the function's `name`. `fn` must stay valid for the runtime's lifetime;
 * a C function is. Like JS_CFUNC_DEF, the property is writable,
 * configurable and non-enumerable. Returns false if the property could not
 * be set (a JS exception is then pending). */
bool jsb_set_fn(jsb_ctx *ctx, jsb_val *obj, const char *name, jsb_fn fn);

/* Sets `obj[name] = val` as an ordinary assignment, so setters run.
 * Returns false if that threw (the exception stays pending). */
bool jsb_set_prop(jsb_ctx *ctx, jsb_val *obj, const char *name,
                  jsb_val *val);

/* Returns `obj[name]` (undefined if absent), running getters. Returns NULL
 * if a getter threw (the exception stays pending). */
jsb_val *jsb_get_prop(jsb_ctx *ctx, jsb_val *obj, const char *name);

/* Creates the global object `name` (for example "__native_audio") and
 * fills it with `init`.
 *
 * Why: Outsider exposes each binding as one `__native_*` global; this keeps
 * that shape. The global is set only after `init` succeeds, so a failed
 * init leaves no half-built module behind. Returns false if `init` failed
 * or the global could not be set. */
bool jsb_define_module(jsb_ctx *ctx, const char *name, jsb_module_init init);

/* ---- Native memory ----------------------------------------------------- */

/* Returns a new ArrayBuffer whose bytes are `len` bytes of native memory
 * at `data`, without copying.
 *
 * Why: decoded images, audio and file contents are produced in C; exposing
 * them to JS in place avoids a copy of every asset. On success JS owns the
 * memory: `free_fn(data, len, user)` runs exactly once, after the buffer is
 * garbage-collected or detached, or at the latest in jsb_runtime_destroy
 * (see jsb_free_fn for where it may run). Until then C may read and write
 * the bytes on the main thread, and JS sees the writes. The engine already
 * counts these bytes as external memory; do not also report them with
 * jsb_adjust_external_memory. `data` and `free_fn` must not be NULL
 * (TypeError) and `len` must fit an ArrayBuffer (RangeError); on any
 * failure NULL is returned and the caller still owns `data`. */
jsb_val *jsb_new_external_buffer(jsb_ctx *ctx, void *data, size_t len,
                                 jsb_free_fn free_fn, void *user);

/* Reports `change` bytes of native memory kept alive by JS objects, so the
 * garbage collector runs as often as the real memory use needs.
 *
 * Why: a texture or decoder held by a small JS wrapper looks cheap to the
 * GC; without this the native memory grows long before a collection (spec
 * section 15.8). V8 receives it via AdjustAmountOfExternalAllocatedMemory.
 * Add the size when the native resource is made and subtract the same
 * amount when it is freed. Returns this runtime's reported total in bytes.
 * A change that would make the total negative is a bug in the caller: it
 * is ignored, reported on stderr, and -1 is returned. Main thread only;
 * never call it from a jsb_free_fn. */
int64_t jsb_adjust_external_memory(jsb_ctx *ctx, int64_t change);

/* ---- Persistent callbacks ---------------------------------------------- */

/* A persistent handle to a JS function, kept by C beyond the native call
 * that received the function (audio `onended`, video events).
 *
 * Why: a jsb_val dies with its call, but an event fires frames later. The
 * handle is a small value: copy it freely, and zero-initialise it
 * (`jsb_callback cb = {0};`) for "no callback". Its fields are private to
 * jsb. A handle is checked on every use: once freed, every copy of it is
 * refused, even after jsb reuses the slot. It belongs to one runtime and
 * must not be used after jsb_runtime_destroy, which releases them all. */
typedef struct jsb_callback {
    jsb_runtime *rt;
    uint32_t slot;
    uint32_t gen;
} jsb_callback;

/* Kinds of value a C caller passes to or gets back from a callback. */
typedef enum jsb_type {
    JSB_TYPE_UNDEFINED,
    JSB_TYPE_NULL,
    JSB_TYPE_BOOL,
    JSB_TYPE_NUMBER,
    JSB_TYPE_STRING,
    JSB_TYPE_OBJECT /* results only: any object, function or symbol */
} jsb_type;

/* One callback argument built in C. Only the field for `type` is read;
 * `str` is NUL-terminated UTF-8, borrowed for the call and copied. */
typedef struct jsb_cval {
    jsb_type type;
    bool b;
    double num;
    const char *str;
} jsb_cval;

/* A callback's return value as seen from C.
 *
 * The caller sets `str`/`str_cap` to a buffer (or NULL/0) before the call.
 * A String result is copied there as UTF-8, truncated to `str_cap` - 1
 * bytes and NUL-terminated; `str_len` gets its full byte length, so
 * `str_len >= str_cap` means it was truncated. Objects only report their
 * type: no JS object reaches C through a result. */
typedef struct jsb_result {
    jsb_type type;
    bool b;
    double num;
    char *str;
    size_t str_cap;
    size_t str_len;
} jsb_result;

/* What jsb_callback_call returns. */
typedef enum jsb_status {
    JSB_OK = 0,
    JSB_ERR_THREW = -1,  /* the function threw; the error handler got it */
    JSB_ERR_HANDLE = -2, /* zero, freed or stale handle; nothing ran */
    JSB_ERR_ARG = -3     /* bad argc/argv from C; nothing ran */
} jsb_status;

/* Receives every exception thrown by a function run through
 * jsb_callback_call. `message` is String(exception) and `stack` its
 * `stack` property, or "" when it has none (both UTF-8, valid only during
 * the call). `user` is jsb_set_error_handler's. Runs on the main thread
 * before jsb_callback_call returns; it must not call jsb on that runtime. */
typedef void (*jsb_error_fn)(const char *message, const char *stack,
                             void *user);

/* Routes callback exceptions of `rt` to `fn(message, stack, user)`.
 *
 * Why: an event callback has no JS caller to throw to, and an exception
 * must never cross into C (spec section 4.1). The engine's error handler
 * registers here, so a failing `onended` is reported like any uncaught
 * error. NULL `fn` restores the default, which prints both on stderr.
 * Main thread only. */
void jsb_set_error_handler(jsb_runtime *rt, jsb_error_fn fn, void *user);

/* Makes a persistent handle to the function `fn` (from `ctx`) in `*out`.
 *
 * Why: this is how a binding keeps a JS callback past its own call (the
 * JS_DupValue of Outsider's audio and video bindings). The handle keeps the
 * function alive until jsb_callback_free. Returns false after a TypeError
 * on `ctx` when `fn` is NULL or not callable; `*out` is then zeroed. Works
 * in native calls and setup callbacks. Main thread only. */
bool jsb_callback_new(jsb_ctx *ctx, jsb_val *fn, jsb_callback *out);

/* Calls the function behind `cb` with `argc` arguments from `argv` and
 * `this` undefined, in its own handle scope.
 *
 * Why: events driven by the host (audio end, video state) run JS from C
 * with no native call around them. Works both outside any call and inside
 * a native call. `argv` may be NULL when `argc` is 0; `argc` must be 0..64.
 * `ret` may be NULL; otherwise it receives the result (see jsb_result).
 * An exception thrown by the function is caught, passed to the runtime's
 * error handler, and JSB_ERR_THREW returned; it never reaches C or an
 * enclosing JS caller. A zero, freed or stale `cb` is JSB_ERR_HANDLE and a
 * bad `argc`/`argv` (or JSB_TYPE_OBJECT argument) JSB_ERR_ARG, both
 * printed on stderr. On any error `ret->type` is JSB_TYPE_UNDEFINED. The
 * handle stays valid even if the function frees it. Main thread only. */
jsb_status jsb_callback_call(jsb_callback cb, int argc, const jsb_cval *argv,
                             jsb_result *ret);

/* Releases the function behind `*cb` and zeroes `*cb`.
 *
 * Why: each handle has exactly one owner, which frees it when the event can
 * no longer fire (spec section 15.8); the function is then garbage once JS
 * drops it too. Every other copy of the handle becomes invalid. Returns
 * false, after a message on stderr, for a zero, freed or stale handle, so a
 * double free is reported rather than corrupting anything. Main thread
 * only; never from a jsb_free_fn. */
bool jsb_callback_free(jsb_callback *cb);

/* ---- Values for result objects (card M1-T007b) ------------------------ */
/* Why: Outsider's bindings build result objects and arrays from C values
 * (JS_NewInt32, JS_NewString, JS_NewArray, JS_NULL...); these keep that JS
 * API byte-for-byte. Each returns a call-scoped handle (valid like any
 * jsb_val until the ctx ends; nothing to free) and works in native calls
 * and setup callbacks. Main thread only. */

/* Returns an integer Number (JS_NewInt32). Never NULL. */
jsb_val *jsb_new_i32(jsb_ctx *ctx, int32_t value);
/* Returns a Number from an unsigned 32-bit integer (JS_NewUint32). */
jsb_val *jsb_new_u32(jsb_ctx *ctx, uint32_t value);
/* Returns a Number (JS_NewFloat64); NaN and infinities are kept. */
jsb_val *jsb_new_f64(jsb_ctx *ctx, double value);
/* Returns true or false (JS_NewBool, JS_TRUE, JS_FALSE). */
jsb_val *jsb_new_bool(jsb_ctx *ctx, bool value);
/* Returns null (JS_NULL). */
jsb_val *jsb_new_null(jsb_ctx *ctx);

/* Returns a String copied from NUL-terminated UTF-8 `utf8` (JS_NewString;
 * borrowed). NULL is a TypeError and returns NULL; invalid UTF-8 is not
 * defined to match between backends, so pass valid UTF-8. */
jsb_val *jsb_new_str(jsb_ctx *ctx, const char *utf8);

/* Returns a String from `len` bytes of UTF-8 at `utf8`, which may contain
 * embedded NULs and need not be NUL-terminated (JS_NewStringLen).
 * `utf8` may be NULL only when `len` is 0; otherwise NULL is a TypeError.
 * More than INT32_MAX bytes is a RangeError. Both return NULL. */
jsb_val *jsb_new_str_len(jsb_ctx *ctx, const char *utf8, size_t len);

/* Returns a new empty Array (JS_NewArray). Never NULL. */
jsb_val *jsb_new_array(jsb_ctx *ctx);

/* Sets `obj[index] = val` as an ordinary assignment (JS_SetPropertyUint32),
 * so an array grows. A non-object `obj` or NULL `val` is a TypeError.
 * Returns false if the assignment threw (the exception stays pending). */
bool jsb_set_index(jsb_ctx *ctx, jsb_val *obj, uint32_t index, jsb_val *val);

/* Returns a new ArrayBuffer holding a copy of `len` bytes at `data`
 * (JS_NewArrayBufferCopy). Unlike jsb_new_external_buffer the engine owns
 * the copy and the caller keeps `data` (borrowed). `data` may be NULL only
 * when `len` is 0 (TypeError); more than INT32_MAX bytes is a RangeError.
 * Both return NULL. */
jsb_val *jsb_new_buffer_copy(jsb_ctx *ctx, const void *data, size_t len);

/* ---- Coercing reads (card M1-T007b) ----------------------------------- */
/* Why: the typed getters above throw on a wrong type, but Outsider's
 * bindings coerce (JS_ToBool, JS_ToInt32, JS_ToCString...), so `"3"` or an
 * object with valueOf() is accepted; porting them must keep that. These
 * apply the ECMAScript conversions, which run valueOf/toString/
 * Symbol.toPrimitive and can therefore throw (a Symbol is a TypeError).
 * On a throw they return false (or NULL) with the exception pending; the
 * native function returns at once or calls jsb_clear_exception. A missing
 * argument is undefined. */

/* Returns ToBoolean(argument `index`) (JS_ToBool). Never throws. */
bool jsb_arg_truthy(jsb_ctx *ctx, int index);

/* Reads ToInt32(argument `index`) into `*out` (JS_ToInt32). */
bool jsb_arg_to_i32(jsb_ctx *ctx, int index, int32_t *out);

/* Reads ToUint32(argument `index`) into `*out` (JS_ToUint32). */
bool jsb_arg_to_u32(jsb_ctx *ctx, int index, uint32_t *out);

/* Reads ToNumber(argument `index`) into `*out` (JS_ToFloat64). */
bool jsb_arg_to_f64(jsb_ctx *ctx, int index, double *out);

/* Returns ToString(argument `index`) as NUL-terminated UTF-8 in new memory
 * (JS_ToCStringLen); the caller frees it with jsb_free_str. `len` (may be
 * NULL) gets the byte length without the NUL; embedded NULs are kept.
 * Returns NULL when the conversion threw (a Symbol is a TypeError) or, as
 * an InternalError "out of memory", when memory ran out. */
char *jsb_arg_to_str(jsb_ctx *ctx, int index, size_t *len);

/* Frees a string from jsb_arg_to_str (JS_FreeCString); NULL is ignored.
 * Safe at any time, also after the call that made it has returned. */
void jsb_free_str(char *str);

/* ---- Reading array elements (card M1-T007b) --------------------------- */

/* Returns whether `val` is an Array (Array.isArray, JS_IsArray); NULL is
 * false. Never throws. */
bool jsb_is_array(jsb_ctx *ctx, jsb_val *val);

/* Returns `obj[index]` (undefined if absent), running getters
 * (JS_GetPropertyUint32). A non-object `obj` is a TypeError; NULL is
 * returned then and when a getter threw (the exception stays pending). */
jsb_val *jsb_get_index(jsb_ctx *ctx, jsb_val *obj, uint32_t index);

/* Reads ToNumber(`val`) into `*out` (JS_ToFloat64 on an element); NULL
 * `val` is a TypeError. Throws and fails as the jsb_arg_to_* calls do. */
bool jsb_val_to_f64(jsb_ctx *ctx, jsb_val *val, double *out);

/* Reads ToInt32(`val`) (JS_ToInt32 on an element); as jsb_val_to_f64. */
bool jsb_val_to_i32(jsb_ctx *ctx, jsb_val *val, int32_t *out);

/* Reads ToUint32(`val`) (JS_ToUint32 on an element); as jsb_val_to_f64. */
bool jsb_val_to_u32(jsb_ctx *ctx, jsb_val *val, uint32_t *out);

/* Returns the kind of `val` (JS_IsNull, JS_IsUndefined and friends): NULL
 * counts as undefined; objects, functions, symbols and BigInts are all
 * JSB_TYPE_OBJECT. Never throws. */
jsb_type jsb_val_type(jsb_ctx *ctx, jsb_val *val);

/* ---- Errors and functions (card M1-T007b) ----------------------------- */

/* Drops the call's pending exception, if any, so the native function can
 * go on (JS_GetException + JS_FreeValue).
 *
 * Why: some bindings try one argument form, and on failure fall back to
 * another or return null/0. Afterwards jsb_ret_* and jsb_throw_* work
 * again. A script termination (the watchdog) is never dropped: it keeps
 * unwinding. Does nothing when nothing is pending. */
void jsb_clear_exception(jsb_ctx *ctx);

/* Throws a plain Error (JS_ThrowPlainError); otherwise as jsb_throw_type. */
void jsb_throw_error(jsb_ctx *ctx, const char *fmt, ...);

/* Throws an InternalError (JS_ThrowInternalError; JS_ThrowOutOfMemory is
 * jsb_throw_internal(ctx, "out of memory")); otherwise as jsb_throw_type.
 * V8 has no InternalError, so there it is an Error whose own `name` is
 * "InternalError": String(e) and e.name preserve the binding contract, e instanceof Error
 * holds on both. */
void jsb_throw_internal(jsb_ctx *ctx, const char *fmt, ...);

/* As jsb_set_fn, and sets the function's `length` to `length` (0..255),
 * as JS_CFUNC_DEF's arity does; out of range is a RangeError. The property
 * is writable, configurable and non-enumerable, so it does not appear in
 * Object.keys or for-in. */
bool jsb_set_fn_n(jsb_ctx *ctx, jsb_val *obj, const char *name, jsb_fn fn,
                  int length);

/* Returns the engine's version string (JS_GetVersion; V8::GetVersion on
 * V8), for process.versions-style reports. Static storage; never NULL.
 * Any thread. */
const char *jsb_engine_version(void);

#ifdef __cplusplus
}
#endif

#endif /* SMZ_JSB_H */
