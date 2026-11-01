/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * TRANSITIONAL QuickJS implementation of the jsb seam (src/jsb/jsb.h).
 * Removed by card M1-T027, together with QuickJS itself.
 *
 * Why: spec section 4.1 gives jsb one implementation, V8 (jsb_v8.cc). While
 * M1 ports Outsider's bindings to jsb one at a time, the game still runs on
 * QuickJS, so a ported binding must work in both engine modes for the tests
 * to stay green (docs/SPEC_ISSUES.md SI-11, DEC-SETUP-008). This file is that
 * stand-in and nothing more: it must behave exactly like jsb_v8.cc, and it
 * gains no API of its own. See docs/js-binding.md.
 *
 * Handle rules (spec section 15.8, docs/js-binding.md): a jsb_ctx lives on
 * the stack of one native call, setup or eval. Every jsb_val it hands out
 * owns one reference to its JSValue, kept in chunks that never move, and the
 * ctx frees them all when the call ends; so, as with V8 Locals, no jsb_val
 * outlives its call. Only jsb_runtime holds engine objects beyond a call (the
 * JSRuntime and JSContext), and jsb_runtime_destroy frees them. Main thread
 * only: QuickJS runtimes are not thread-safe.
 */
#include "jsb.h"

#include <quickjs.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* QuickJS keeps a C function's magic in an int16_t, which bounds how many
 * distinct jsb_fn a runtime can register. */
#define JSB_MAX_FNS 32767

/* Number of jsb_vals per chunk; a typical binding call needs a handful. */
#define JSB_CHUNK_VALS 16

/* A JS value handle; owns one reference to `value` until its ctx ends. */
struct jsb_val {
    JSValue value;
};

/* A fixed block of handles. Chunks are linked, never reallocated, so a
 * jsb_val pointer stays valid for the whole call. */
typedef struct jsb_chunk {
    struct jsb_chunk *next;
    int used;
    jsb_val vals[JSB_CHUNK_VALS];
} jsb_chunk;

/* Per-call state. `in_call` is false for setup callbacks and eval, which
 * have no arguments and no return slot. `ret` is owned by the ctx. */
struct jsb_ctx {
    JSContext *js;
    jsb_runtime *rt;
    int argc;
    JSValueConst *argv;
    bool in_call;
    bool threw;
    JSValue ret;
    jsb_chunk first;
    jsb_chunk *last;
};

/* The engine instance. `fns` maps a function's QuickJS magic number to the
 * jsb_fn it calls; it only grows and is freed with the runtime. `dv_get`
 * holds the intrinsic DataView.prototype getters for buffer, byteOffset and
 * byteLength (QuickJS has no C API for DataView), taken before any user
 * script runs so that script cannot replace them. `external_bytes` is the
 * jsb_adjust_external_memory total. */
struct jsb_runtime {
    JSRuntime *qrt;
    JSContext *js;
    jsb_fn *fns;
    int fn_count;
    int fn_cap;
    JSValue dv_get[3];
    int64_t external_bytes;
};

/* What jsb_new_external_buffer's free function needs; owned by QuickJS's
 * ArrayBuffer through its opaque pointer and freed with the memory. */
typedef struct ext_holder {
    jsb_free_fn free_fn;
    void *user;
    size_t len;
} ext_holder;

/* Set by jsb_set_flags("--expose-gc"): runtimes get a global gc(). */
static bool g_expose_gc;

/* Set by the first jsb_runtime_create; flags are refused afterwards, as on
 * V8, so both backends fail the same way. */
static bool g_started;

/* Prepares `ctx` for one call on `rt`. `argv` is borrowed from QuickJS for
 * the duration of the call (NULL with `argc` 0 outside a call). */
static void ctx_init(jsb_ctx *ctx, jsb_runtime *rt, int argc,
                     JSValueConst *argv, bool in_call) {
    ctx->js = rt->js;
    ctx->rt = rt;
    ctx->argc = argc;
    ctx->argv = argv;
    ctx->in_call = in_call;
    ctx->threw = false;
    ctx->ret = JS_UNDEFINED;
    ctx->first.next = NULL;
    ctx->first.used = 0;
    ctx->last = &ctx->first;
}

/* Frees every handle of `ctx` and its extra chunks. The result in
 * `ctx->ret` is not touched: the caller takes or frees it. */
static void ctx_finish(jsb_ctx *ctx) {
    jsb_chunk *chunk = &ctx->first;
    while (chunk) {
        jsb_chunk *next = chunk->next;
        for (int i = 0; i < chunk->used; i++)
            JS_FreeValue(ctx->js, chunk->vals[i].value);
        if (chunk != &ctx->first) free(chunk);
        chunk = next;
    }
}

/* Makes a call-scoped jsb_val that takes over the reference `value`. Out
 * of memory aborts the process loudly: callers rely on a non-NULL result
 * (jsb_new_object promises one), as the V8 backend's deque does. */
static jsb_val *wrap(jsb_ctx *ctx, JSValue value) {
    if (ctx->last->used == JSB_CHUNK_VALS) {
        jsb_chunk *chunk = malloc(sizeof *chunk);
        if (!chunk) {
            fprintf(stderr, "jsb: out of memory for a value handle\n");
            abort();
        }
        chunk->next = NULL;
        chunk->used = 0;
        ctx->last->next = chunk;
        ctx->last = chunk;
    }
    jsb_val *val = &ctx->last->vals[ctx->last->used++];
    val->value = value;
    return val;
}

/* Throws a new TypeError (RangeError when `range`) with a printf-style
 * message, unless the call already threw: the first error is the specific
 * one, later ones are usually fallout. */
static void throw_with(jsb_ctx *ctx, bool range, const char *fmt,
                       va_list ap) {
    if (ctx->threw) return;
    char message[256];
    vsnprintf(message, sizeof message, fmt, ap);
    ctx->threw = true;
    if (range)
        JS_ThrowRangeError(ctx->js, "%s", message);
    else
        JS_ThrowTypeError(ctx->js, "%s", message);
}

/* Returns argument `index` (borrowed), undefined when missing or outside a
 * call. */
static JSValueConst arg(jsb_ctx *ctx, int index) {
    if (!ctx->in_call || index < 0 || index >= ctx->argc) return JS_UNDEFINED;
    return ctx->argv[index];
}

/* Throws the getters' common TypeError for argument `index`. */
static void throw_arg_type(jsb_ctx *ctx, int index, const char *expected) {
    jsb_throw_type(ctx, "argument %d must be %s", index + 1, expected);
}

/* Replaces the call's result with `value`, taking over its reference, or
 * frees `value` when no result may be set (outside a call, or after a
 * throw), so a dropped result never leaks. */
static void set_ret(jsb_ctx *ctx, JSValue value) {
    if (!ctx->in_call || ctx->threw) {
        JS_FreeValue(ctx->js, value);
        return;
    }
    JS_FreeValue(ctx->js, ctx->ret);
    ctx->ret = value;
}

/* Returns whether `val` is a usable object handle, so callers can throw a
 * TypeError instead of crashing on NULL or a primitive. */
static bool is_object(jsb_val *val) { return val && JS_IsObject(val->value); }

/* Takes and discards the pending exception, if any; used where an error
 * has already been reported another way. */
static void clear_exception(JSContext *js) {
    JS_FreeValue(js, JS_GetException(js));
}

/* Writes String(value) as UTF-8 into `out` (truncated, NUL-terminated);
 * does nothing if `out` is NULL or `cap` is 0. A value whose String()
 * throws is written as "(unconvertible value)" and its error is dropped. */
static void to_buffer(JSContext *js, JSValueConst value, char *out,
                      size_t cap) {
    if (!out || cap == 0) return;
    size_t n = 0;
    const char *text = JS_ToCStringLen(js, &n, value);
    const char *s = text;
    if (!text) {
        clear_exception(js);
        s = "(unconvertible value)";
        n = strlen(s);
    }
    if (n > cap - 1) n = cap - 1;
    memcpy(out, s, n);
    out[n] = '\0';
    if (text) JS_FreeCString(js, text);
}

/* Entry point QuickJS calls for every jsb_set_fn function; `magic` indexes
 * the runtime's jsb_fn table. Builds the call's jsb_ctx, runs the C
 * function, and returns its result (ownership passes to QuickJS), or
 * JS_EXCEPTION when it threw or left a getter's exception pending. */
static JSValue trampoline(JSContext *js, JSValueConst this_val, int argc,
                          JSValueConst *argv, int magic) {
    (void)this_val;
    jsb_runtime *rt = JS_GetContextOpaque(js);
    jsb_ctx ctx;
    ctx_init(&ctx, rt, argc, argv, true);
    rt->fns[magic](&ctx);
    JSValue ret = ctx.ret;
    bool failed = ctx.threw || JS_HasException(js);
    ctx_finish(&ctx);
    if (failed) {
        JS_FreeValue(js, ret);
        return JS_EXCEPTION;
    }
    return ret;
}

/* Returns the magic number for `fn`, adding it to the runtime's table if
 * new, or -1 when the table is full or cannot grow. */
static int fn_index(jsb_runtime *rt, jsb_fn fn) {
    for (int i = 0; i < rt->fn_count; i++)
        if (rt->fns[i] == fn) return i;
    if (rt->fn_count == JSB_MAX_FNS) return -1;
    if (rt->fn_count == rt->fn_cap) {
        int cap = rt->fn_cap ? rt->fn_cap * 2 : 64;
        if (cap > JSB_MAX_FNS) cap = JSB_MAX_FNS;
        jsb_fn *grown = realloc(rt->fns, (size_t)cap * sizeof *grown);
        if (!grown) return -1;
        rt->fns = grown;
        rt->fn_cap = cap;
    }
    rt->fns[rt->fn_count] = fn;
    return rt->fn_count++;
}

/* Implements the global gc() that V8 installs for "--expose-gc": runs a
 * full cycle collection. Takes no arguments and returns undefined. */
static JSValue js_gc(JSContext *js, JSValueConst this_val, int argc,
                     JSValueConst *argv) {
    (void)this_val;
    (void)argc;
    (void)argv;
    JS_RunGC(JS_GetRuntime(js));
    return JS_UNDEFINED;
}

/* Stores the intrinsic DataView.prototype getters in `rt->dv_get` and, if
 * "--expose-gc" was set, defines the global gc(). Runs before any user
 * script. Returns false, with the error printed, if either fails. */
static bool install_intrinsics(jsb_runtime *rt) {
    static const char src[] =
        "(function () { var p = DataView.prototype,"
        " d = Object.getOwnPropertyDescriptor; return [d(p, 'buffer').get,"
        " d(p, 'byteOffset').get, d(p, 'byteLength').get]; })()";
    JSValue list = JS_Eval(rt->js, src, sizeof src - 1, "<jsb>",
                           JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(list)) {
        clear_exception(rt->js);
        fprintf(stderr, "jsb: cannot read the DataView getters\n");
        return false;
    }
    for (uint32_t i = 0; i < 3; i++)
        rt->dv_get[i] = JS_GetPropertyUint32(rt->js, list, i);
    JS_FreeValue(rt->js, list);
    if (!g_expose_gc) return true;
    JSValue global = JS_GetGlobalObject(rt->js);
    int rc = JS_SetPropertyStr(rt->js, global, "gc",
                               JS_NewCFunction(rt->js, js_gc, "gc", 0));
    JS_FreeValue(rt->js, global);
    if (rc < 0) fprintf(stderr, "jsb: cannot define gc()\n");
    return rc >= 0;
}

/* See jsb.h: jsb_set_flags. Only "--expose-gc" (or "--expose_gc", which V8
 * also accepts) means anything here; other flags tune V8 and are ignored. */
bool jsb_set_flags(const char *flags) {
    if (g_started) {
        fprintf(stderr, "jsb: engine flags must be set before the first "
                        "runtime is created\n");
        return false;
    }
    const char *p = flags;
    while (*p) {
        while (*p == ' ') p++;
        size_t n = strcspn(p, " ");
        if (n == 11 && (strncmp(p, "--expose-gc", n) == 0 ||
                        strncmp(p, "--expose_gc", n) == 0))
            g_expose_gc = true;
        p += n;
    }
    return true;
}

/* See jsb.h. `argv0` and `icu_data_path` are unused: QuickJS needs no
 * process-wide setup and has no ICU. */
jsb_runtime *jsb_runtime_create(const char *argv0, const char *icu_data_path) {
    (void)argv0;
    (void)icu_data_path;
    g_started = true;
    jsb_runtime *rt = calloc(1, sizeof *rt);
    if (!rt) {
        fprintf(stderr, "jsb: out of memory for a runtime\n");
        return NULL;
    }
    for (int i = 0; i < 3; i++) rt->dv_get[i] = JS_UNDEFINED;
    rt->qrt = JS_NewRuntime();
    rt->js = rt->qrt ? JS_NewContext(rt->qrt) : NULL;
    if (!rt->js) {
        fprintf(stderr, "jsb: cannot create a QuickJS context\n");
        jsb_runtime_destroy(rt);
        return NULL;
    }
    JS_SetContextOpaque(rt->js, rt);
    if (!install_intrinsics(rt)) {
        jsb_runtime_destroy(rt);
        return NULL;
    }
    return rt;
}

/* See jsb.h. Frees the context before the runtime, as QuickJS requires;
 * that runs the free function of every live external buffer. */
void jsb_runtime_destroy(jsb_runtime *rt) {
    if (!rt) return;
    for (int i = 0; i < 3 && rt->js; i++) JS_FreeValue(rt->js, rt->dv_get[i]);
    if (rt->js) JS_FreeContext(rt->js);
    if (rt->qrt) JS_FreeRuntime(rt->qrt);
    free(rt->fns);
    free(rt);
}

/* See jsb.h: jsb_runtime_setup. */
bool jsb_runtime_setup(jsb_runtime *rt, jsb_setup_fn fn, void *user) {
    jsb_ctx ctx;
    ctx_init(&ctx, rt, 0, NULL, false);
    bool ok = fn(&ctx, user);
    ctx_finish(&ctx);
    if (JS_HasException(rt->js)) {
        char message[512];
        JSValue exc = JS_GetException(rt->js);
        to_buffer(rt->js, exc, message, sizeof message);
        JS_FreeValue(rt->js, exc);
        fprintf(stderr, "jsb: setup threw: %s\n", message);
        return false;
    }
    return ok;
}

/* See jsb.h: jsb_runtime_eval. */
bool jsb_runtime_eval(jsb_runtime *rt, const char *source,
                      const char *file_name, char *out, size_t out_cap) {
    JSValue result = JS_Eval(rt->js, source, strlen(source),
                             file_name ? file_name : "<eval>",
                             JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(result)) {
        JSValue exc = JS_GetException(rt->js);
        to_buffer(rt->js, exc, out, out_cap);
        JS_FreeValue(rt->js, exc);
        return false;
    }
    to_buffer(rt->js, result, out, out_cap);
    JS_FreeValue(rt->js, result);
    return true;
}

/* See jsb.h: jsb_arg_count. */
int jsb_arg_count(jsb_ctx *ctx) { return ctx->in_call ? ctx->argc : 0; }

/* See jsb.h: jsb_arg_i32. */
bool jsb_arg_i32(jsb_ctx *ctx, int index, int32_t *out) {
    JSValueConst v = arg(ctx, index);
    if (!JS_IsNumber(v)) {
        throw_arg_type(ctx, index, "a number");
        return false;
    }
    /* ToInt32 of a Number cannot throw. */
    JS_ToInt32(ctx->js, out, v);
    return true;
}

/* See jsb.h: jsb_arg_f64. */
bool jsb_arg_f64(jsb_ctx *ctx, int index, double *out) {
    JSValueConst v = arg(ctx, index);
    if (!JS_IsNumber(v)) {
        throw_arg_type(ctx, index, "a number");
        return false;
    }
    JS_ToFloat64(ctx->js, out, v);
    return true;
}

/* See jsb.h: jsb_arg_bool. */
bool jsb_arg_bool(jsb_ctx *ctx, int index, bool *out) {
    JSValueConst v = arg(ctx, index);
    if (!JS_IsBool(v)) {
        throw_arg_type(ctx, index, "a boolean");
        return false;
    }
    *out = JS_ToBool(ctx->js, v) != 0;
    return true;
}

/* See jsb.h: jsb_arg_str. Lone surrogates come out as QuickJS encodes
 * them (CESU-8 style), where V8 writes U+FFFD; no binding relies on it. */
bool jsb_arg_str(jsb_ctx *ctx, int index, char *buf, size_t cap,
                 size_t *len) {
    JSValueConst v = arg(ctx, index);
    if (!JS_IsString(v)) {
        throw_arg_type(ctx, index, "a string");
        return false;
    }
    size_t n = 0;
    const char *text = JS_ToCStringLen(ctx->js, &n, v);
    if (!text) { /* out of memory: QuickJS left the error pending */
        ctx->threw = true;
        return false;
    }
    bool fits = cap > 0 && n <= cap - 1;
    if (fits) {
        memcpy(buf, text, n);
        buf[n] = '\0';
        if (len) *len = n;
    }
    JS_FreeCString(ctx->js, text);
    if (!fits)
        jsb_throw_range(ctx, "argument %d is %zu bytes, the limit is %zu",
                        index + 1, n, cap ? cap - 1 : 0);
    return fits;
}

/* See jsb.h: jsb_arg_obj. */
jsb_val *jsb_arg_obj(jsb_ctx *ctx, int index) {
    JSValueConst v = arg(ctx, index);
    if (!JS_IsObject(v)) {
        throw_arg_type(ctx, index, "an object");
        return NULL;
    }
    return wrap(ctx, JS_DupValue(ctx->js, v));
}

/* See jsb.h: jsb_arg_val. */
jsb_val *jsb_arg_val(jsb_ctx *ctx, int index) {
    return wrap(ctx, JS_DupValue(ctx->js, arg(ctx, index)));
}

/* Calls DataView getter `which` (0 buffer, 1 byteOffset, 2 byteLength) on
 * `v`. Returns the result (owned by the caller), or JS_EXCEPTION with the
 * error cleared when `v` is not a DataView or, for 1 and 2, is detached. */
static JSValue dv_call(jsb_ctx *ctx, int which, JSValueConst v) {
    JSValue r = JS_Call(ctx->js, ctx->rt->dv_get[which], v, 0, NULL);
    if (JS_IsException(r)) clear_exception(ctx->js);
    return r;
}

/* Reads a DataView's buffer (into `*buffer`, owned by the caller), byte
 * offset and byte length. Returns 1 on success, 0 when `v` is not a
 * DataView and -1 when its buffer is detached. */
static int dataview_parts(jsb_ctx *ctx, JSValueConst v, JSValue *buffer,
                          size_t *offset, size_t *length) {
    JSValue buf = dv_call(ctx, 0, v);
    if (JS_IsException(buf)) return 0;
    JSValue off = dv_call(ctx, 1, v);
    JSValue len = dv_call(ctx, 2, v);
    int64_t o = 0, l = 0;
    bool ok = !JS_IsException(off) && !JS_IsException(len) &&
              JS_ToInt64(ctx->js, &o, off) == 0 &&
              JS_ToInt64(ctx->js, &l, len) == 0;
    JS_FreeValue(ctx->js, off);
    JS_FreeValue(ctx->js, len);
    if (!ok) {
        JS_FreeValue(ctx->js, buf);
        return -1;
    }
    *buffer = buf;
    *offset = (size_t)o;
    *length = (size_t)l;
    return 1;
}

/* Finds the ArrayBuffer behind `v` (into `*buffer`, owned by the caller)
 * and the view's byte range; `*whole` is set for a bare ArrayBuffer, whose
 * length is read later. Returns 1 on success, 0 when `v` is not an
 * ArrayBuffer, TypedArray or DataView, and -1 when it is detached. */
static int buffer_parts(jsb_ctx *ctx, JSValueConst v, JSValue *buffer,
                        size_t *offset, size_t *length, bool *whole) {
    *offset = 0;
    *length = 0;
    *whole = false;
    if (!JS_IsObject(v)) return 0;
    if (JS_IsArrayBuffer(v)) {
        *buffer = JS_DupValue(ctx->js, v);
        *whole = true;
        return 1;
    }
    if (JS_GetTypedArrayType(v) >= 0) {
        size_t bytes_per_element = 0;
        *buffer = JS_GetTypedArrayBuffer(ctx->js, v, offset, length,
                                         &bytes_per_element);
        if (!JS_IsException(*buffer)) return 1;
        clear_exception(ctx->js); /* detached or out of bounds */
        return -1;
    }
    return dataview_parts(ctx, v, buffer, offset, length);
}

/* See jsb.h: jsb_arg_bytes. QuickJS reports a detached buffer by failing
 * JS_GetArrayBuffer; a zero-length one may have no data pointer. */
bool jsb_arg_bytes(jsb_ctx *ctx, int index, uint8_t **data, size_t *len) {
    JSValue buffer;
    size_t offset, length, size = 0;
    bool whole;
    int kind = buffer_parts(ctx, arg(ctx, index), &buffer, &offset, &length,
                            &whole);
    uint8_t *base = NULL;
    if (kind > 0) {
        base = JS_GetArrayBuffer(ctx->js, &size, buffer);
        JS_FreeValue(ctx->js, buffer);
        if (!base && JS_HasException(ctx->js)) {
            clear_exception(ctx->js);
            kind = -1;
        }
    }
    if (kind == 0) {
        throw_arg_type(ctx, index, "an ArrayBuffer, TypedArray or DataView");
        return false;
    }
    if (kind < 0 || (!whole && offset + length > size)) {
        jsb_throw_type(ctx, "argument %d is a detached ArrayBuffer or a view "
                            "on one", index + 1);
        return false;
    }
    if (whole) length = size;
    *data = base ? base + offset : NULL;
    *len = length;
    return true;
}

/* See jsb.h: jsb_ret_undefined. */
void jsb_ret_undefined(jsb_ctx *ctx) { set_ret(ctx, JS_UNDEFINED); }

/* See jsb.h: jsb_ret_bool. */
void jsb_ret_bool(jsb_ctx *ctx, bool value) {
    set_ret(ctx, JS_NewBool(ctx->js, value));
}

/* See jsb.h: jsb_ret_i32. */
void jsb_ret_i32(jsb_ctx *ctx, int32_t value) {
    set_ret(ctx, JS_NewInt32(ctx->js, value));
}

/* See jsb.h: jsb_ret_f64. */
void jsb_ret_f64(jsb_ctx *ctx, double value) {
    set_ret(ctx, JS_NewFloat64(ctx->js, value));
}

/* See jsb.h: jsb_ret_str. Built only when it can be returned, so an
 * ignored result costs no allocation. */
void jsb_ret_str(jsb_ctx *ctx, const char *utf8) {
    if (!utf8) {
        jsb_throw_type(ctx, "jsb_ret_str: NULL string");
        return;
    }
    if (!ctx->in_call || ctx->threw) return;
    JSValue s = JS_NewString(ctx->js, utf8);
    if (JS_IsException(s)) { /* out of memory, error pending */
        ctx->threw = true;
        return;
    }
    set_ret(ctx, s);
}

/* See jsb.h: jsb_ret_val. */
void jsb_ret_val(jsb_ctx *ctx, jsb_val *val) {
    set_ret(ctx, val ? JS_DupValue(ctx->js, val->value) : JS_UNDEFINED);
}

/* See jsb.h: jsb_throw_type. */
void jsb_throw_type(jsb_ctx *ctx, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    throw_with(ctx, false, fmt, ap);
    va_end(ap);
}

/* See jsb.h: jsb_throw_range. */
void jsb_throw_range(jsb_ctx *ctx, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    throw_with(ctx, true, fmt, ap);
    va_end(ap);
}

/* See jsb.h: jsb_new_object. */
jsb_val *jsb_new_object(jsb_ctx *ctx) {
    JSValue obj = JS_NewObject(ctx->js);
    if (JS_IsException(obj)) {
        fprintf(stderr, "jsb: out of memory for an object\n");
        abort();
    }
    return wrap(ctx, obj);
}

/* See jsb.h: jsb_set_fn. The function's `length` is 0, as in jsb_v8.cc. */
bool jsb_set_fn(jsb_ctx *ctx, jsb_val *obj, const char *name, jsb_fn fn) {
    if (!is_object(obj) || !fn) {
        jsb_throw_type(ctx, "jsb_set_fn(%s): bad object or function", name);
        return false;
    }
    int magic = fn_index(ctx->rt, fn);
    if (magic < 0) {
        jsb_throw_range(ctx, "jsb_set_fn(%s): more than %d native functions",
                        name, JSB_MAX_FNS);
        return false;
    }
    JSValue func = JS_NewCFunctionMagic(ctx->js, trampoline, name, 0,
                                        JS_CFUNC_generic_magic, magic);
    if (JS_IsException(func)) return false;
    /* JS_SetPropertyStr takes over `func`, also on failure. */
    return JS_SetPropertyStr(ctx->js, obj->value, name, func) >= 0;
}

/* See jsb.h: jsb_set_prop. */
bool jsb_set_prop(jsb_ctx *ctx, jsb_val *obj, const char *name,
                  jsb_val *val) {
    if (!is_object(obj) || !val) {
        jsb_throw_type(ctx, "jsb_set_prop(%s): bad object or value", name);
        return false;
    }
    return JS_SetPropertyStr(ctx->js, obj->value, name,
                             JS_DupValue(ctx->js, val->value)) >= 0;
}

/* See jsb.h: jsb_get_prop. */
jsb_val *jsb_get_prop(jsb_ctx *ctx, jsb_val *obj, const char *name) {
    if (!is_object(obj)) {
        jsb_throw_type(ctx, "jsb_get_prop(%s): not an object", name);
        return NULL;
    }
    JSValue v = JS_GetPropertyStr(ctx->js, obj->value, name);
    if (JS_IsException(v)) return NULL;
    return wrap(ctx, v);
}

/* See jsb.h: jsb_define_module. */
bool jsb_define_module(jsb_ctx *ctx, const char *name, jsb_module_init init) {
    jsb_val *module = jsb_new_object(ctx);
    if (!init(ctx, module)) return false;
    JSValue global = JS_GetGlobalObject(ctx->js);
    int rc = JS_SetPropertyStr(ctx->js, global, name,
                               JS_DupValue(ctx->js, module->value));
    JS_FreeValue(ctx->js, global);
    return rc >= 0;
}

/* Frees an external buffer's memory through its jsb_free_fn; QuickJS calls
 * it on the main thread when the ArrayBuffer is freed or detached. */
static void ext_release(JSRuntime *qrt, void *opaque, void *ptr) {
    (void)qrt;
    ext_holder *h = opaque;
    h->free_fn(ptr, h->len, h->user);
    free(h);
}

/* See jsb.h: jsb_new_external_buffer. The bytes stay in the caller's
 * allocation and are released through `free_fn`; QuickJS has no
 * external-memory accounting, so nothing is reported. */
jsb_val *jsb_new_external_buffer(jsb_ctx *ctx, void *data, size_t len,
                                 jsb_free_fn free_fn, void *user) {
    if (!data || !free_fn) {
        jsb_throw_type(ctx, "jsb_new_external_buffer: NULL data or free "
                            "function");
        return NULL;
    }
    if (len > INT32_MAX) {
        jsb_throw_range(ctx, "jsb_new_external_buffer: %zu bytes is too "
                             "large for an ArrayBuffer", len);
        return NULL;
    }
    ext_holder *h = malloc(sizeof *h);
    if (!h) {
        fprintf(stderr, "jsb: out of memory for an external buffer\n");
        abort();
    }
    h->free_fn = free_fn;
    h->user = user;
    h->len = len;
    JSValue ab = JS_NewArrayBuffer(ctx->js, data, len, ext_release, h, false);
    if (JS_IsException(ab)) { /* error pending; the caller keeps `data` */
        free(h);
        ctx->threw = true;
        return NULL;
    }
    return wrap(ctx, ab);
}

/* See jsb.h: jsb_adjust_external_memory. QuickJS has no GC pressure hook,
 * so only the total is kept, to give the V8 backend's results. */
int64_t jsb_adjust_external_memory(jsb_ctx *ctx, int64_t change) {
    jsb_runtime *rt = ctx->rt;
    if (change < -rt->external_bytes) {
        fprintf(stderr, "jsb: external memory change %lld would make the "
                        "total of %lld bytes negative\n",
                (long long)change, (long long)rt->external_bytes);
        return -1;
    }
    rt->external_bytes += change;
    return rt->external_bytes;
}
