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
 * jsb_fn it calls; it only grows and is freed with the runtime. */
struct jsb_runtime {
    JSRuntime *qrt;
    JSContext *js;
    jsb_fn *fns;
    int fn_count;
    int fn_cap;
};

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

/* See jsb.h. `argv0` and `icu_data_path` are unused: QuickJS needs no
 * process-wide setup and has no ICU. */
jsb_runtime *jsb_runtime_create(const char *argv0, const char *icu_data_path) {
    (void)argv0;
    (void)icu_data_path;
    jsb_runtime *rt = calloc(1, sizeof *rt);
    if (!rt) {
        fprintf(stderr, "jsb: out of memory for a runtime\n");
        return NULL;
    }
    rt->qrt = JS_NewRuntime();
    rt->js = rt->qrt ? JS_NewContext(rt->qrt) : NULL;
    if (!rt->js) {
        fprintf(stderr, "jsb: cannot create a QuickJS context\n");
        jsb_runtime_destroy(rt);
        return NULL;
    }
    JS_SetContextOpaque(rt->js, rt);
    return rt;
}

/* See jsb.h. Frees the context before the runtime, as QuickJS requires. */
void jsb_runtime_destroy(jsb_runtime *rt) {
    if (!rt) return;
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
