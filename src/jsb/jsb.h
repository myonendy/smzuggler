/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * jsb: the small C seam through which native bindings talk to the JS engine.
 *
 * Why: Outsider's bindings (src/bindings/) are C written against QuickJS.
 * spec section 4.1 moves them to V8 by near-mechanical rewriting onto this
 * header, so upstream fixes to those bindings still re-apply with small
 * diffs. It is not an engine abstraction: its one real implementation is
 * src/jsb/jsb_v8.cc (the QuickJS backend of DEC-SETUP-008 is transitional).
 * See docs/js-binding.md for the full API guide.
 *
 * Rules a reader must not break (spec section 15.8, docs/js-binding.md):
 * - This header is plain C11 and never includes C++ or V8 headers.
 * - A jsb_ctx and every jsb_val obtained through it are valid only until the
 *   native call (or setup callback) that received the ctx returns. Never
 *   store either in a struct, a global or a static.
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

/* ---- Runtime (host side) ---------------------------------------------- */

/* Creates an engine instance with one global context.
 *
 * Why: bindings need a live engine to be defined in and tested against;
 * js_engine.c will create its runtime through this once it moves to V8.
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
 * a C function is. Returns false if the property could not be set (a JS
 * exception is then pending). */
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

#ifdef __cplusplus
}
#endif

#endif /* SMZ_JSB_H */
