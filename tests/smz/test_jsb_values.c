/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * The jsb seam calls of card M1-T007b: values, coercing reads, array
 * elements, error kinds, jsb_clear_exception and function `length`.
 *
 * Why: binding ports must keep Outsider's JS API byte-for-byte, so each
 * call must act like the QuickJS one it replaces, on both backends. Built
 * against smz_jsb (V8 in v8 mode) and smz_jsb_quickjs (tests/smz/
 * jsb_values.cmake); a JS script checks the results and returns "ok" or
 * the list of failures. Checks use assert() with NDEBUG undefined.
 */

#include "jsb.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* t.make(): an object holding one value of each jsb_new_* kind. */
static void js_make(jsb_ctx *ctx)
{
    static const unsigned char bytes[3] = {1, 2, 3};
    jsb_val *o = jsb_new_object(ctx);
    jsb_val *arr = jsb_new_array(ctx);
    jsb_set_index(ctx, arr, 0, jsb_new_i32(ctx, 1));
    jsb_set_index(ctx, arr, 1, jsb_new_str(ctx, "x"));
    jsb_set_prop(ctx, o, "i", jsb_new_i32(ctx, -5));
    jsb_set_prop(ctx, o, "u", jsb_new_u32(ctx, 4000000000u));
    jsb_set_prop(ctx, o, "f", jsb_new_f64(ctx, 1.5));
    jsb_set_prop(ctx, o, "t", jsb_new_bool(ctx, true));
    jsb_set_prop(ctx, o, "n", jsb_new_null(ctx));
    jsb_set_prop(ctx, o, "s", jsb_new_str(ctx, "h\xc3\xa9llo"));
    jsb_set_prop(ctx, o, "z", jsb_new_str_len(ctx, "a\0b", 3));
    jsb_set_prop(ctx, o, "e", jsb_new_str_len(ctx, NULL, 0));
    jsb_set_prop(ctx, o, "arr", arr);
    jsb_set_prop(ctx, o, "buf", jsb_new_buffer_copy(ctx, bytes, 3));
    jsb_set_prop(ctx, o, "none", jsb_new_buffer_copy(ctx, NULL, 0));
    jsb_set_prop(ctx, o, "version", jsb_new_str(ctx, jsb_engine_version()));
    jsb_ret_val(ctx, o);
}

/* t.toI32(x), t.toU32(x), t.toF64(x), t.truthy(x): the coercions. */
static void js_to_i32(jsb_ctx *ctx)
{
    int32_t v;
    if (jsb_arg_to_i32(ctx, 0, &v)) jsb_ret_i32(ctx, v);
}

static void js_to_u32(jsb_ctx *ctx)
{
    uint32_t v;
    if (jsb_arg_to_u32(ctx, 0, &v)) jsb_ret_f64(ctx, v);
}

static void js_to_f64(jsb_ctx *ctx)
{
    double v;
    if (jsb_arg_to_f64(ctx, 0, &v)) jsb_ret_f64(ctx, v);
}

static void js_truthy(jsb_ctx *ctx) { jsb_ret_bool(ctx, jsb_arg_truthy(ctx, 0)); }

/* t.toStr(x): [String(x) rebuilt from its bytes, its UTF-8 length]. */
static void js_to_str(jsb_ctx *ctx)
{
    size_t len = 0;
    char *s = jsb_arg_to_str(ctx, 0, &len);
    if (!s) return;
    jsb_val *arr = jsb_new_array(ctx);
    jsb_set_index(ctx, arr, 0, jsb_new_str_len(ctx, s, len));
    jsb_set_index(ctx, arr, 1, jsb_new_u32(ctx, (uint32_t)len));
    jsb_free_str(s);
    jsb_ret_val(ctx, arr);
}

/* t.sum(arr): adds ToNumber of every element, plus ToInt32 and ToUint32 of
   element 0 so their element forms are exercised too. */
static void js_sum(jsb_ctx *ctx)
{
    jsb_val *arr = jsb_arg_val(ctx, 0);
    if (!jsb_is_array(ctx, arr)) {
        jsb_throw_type(ctx, "not an array");
        return;
    }
    double n = 0, sum = 0, v;
    int32_t i0;
    uint32_t u0;
    if (!jsb_val_to_f64(ctx, jsb_get_prop(ctx, arr, "length"), &n)) return;
    for (uint32_t i = 0; i < (uint32_t)n; i++) {
        if (!jsb_val_to_f64(ctx, jsb_get_index(ctx, arr, i), &v)) return;
        sum += v;
    }
    if (n > 0 && (!jsb_val_to_i32(ctx, jsb_get_index(ctx, arr, 0), &i0) ||
                  !jsb_val_to_u32(ctx, jsb_get_index(ctx, arr, 0), &u0)))
        return;
    jsb_ret_f64(ctx, n > 0 ? sum + (double)i0 - (double)u0 : sum);
}

/* t.tryOr(x): ToInt32(x), or -1 after dropping the error it threw. */
static void js_try_or(jsb_ctx *ctx)
{
    int32_t v;
    if (jsb_arg_to_i32(ctx, 0, &v)) {
        jsb_ret_i32(ctx, v);
        return;
    }
    jsb_clear_exception(ctx);
    jsb_ret_i32(ctx, -1);
}

/* t.clearThenThrow(): a dropped error lets a later, different one through. */
static void js_clear_then_throw(jsb_ctx *ctx)
{
    jsb_throw_type(ctx, "first");
    jsb_clear_exception(ctx);
    jsb_throw_range(ctx, "second");
}

/* t.kinds(...): the jsb_val_type of each argument, joined by commas. */
static void js_kinds(jsb_ctx *ctx)
{
    char out[64] = "";
    for (int i = 0; i < jsb_arg_count(ctx) && i < 16; i++) {
        size_t at = strlen(out);
        snprintf(out + at, sizeof out - at, "%s%d", i ? "," : "",
                 (int)jsb_val_type(ctx, jsb_arg_val(ctx, i)));
    }
    jsb_ret_str(ctx, out);
}

/* t.err(), t.internal(), t.oom(), t.nullStr(), t.badLen(): errors. */
static void js_err(jsb_ctx *ctx) { jsb_throw_error(ctx, "boom %d", 7); }
static void js_internal(jsb_ctx *ctx) { jsb_throw_internal(ctx, "deep"); }
static void js_oom(jsb_ctx *ctx) { jsb_throw_internal(ctx, "out of memory"); }

static void js_null_str(jsb_ctx *ctx)
{
    if (!jsb_new_str(ctx, NULL)) return;
    jsb_ret_str(ctx, "no throw");
}

static void js_bad_len(jsb_ctx *ctx)
{
    if (jsb_set_fn_n(ctx, jsb_new_object(ctx), "x", js_err, 256))
        jsb_ret_str(ctx, "no throw");
}

/* Fills the test module `__t`. */
static bool init_t(jsb_ctx *ctx, jsb_val *m)
{
    return jsb_set_fn(ctx, m, "make", js_make) &&
           jsb_set_fn_n(ctx, m, "toI32", js_to_i32, 1) &&
           jsb_set_fn_n(ctx, m, "toU32", js_to_u32, 1) &&
           jsb_set_fn_n(ctx, m, "toF64", js_to_f64, 1) &&
           jsb_set_fn_n(ctx, m, "truthy", js_truthy, 1) &&
           jsb_set_fn_n(ctx, m, "toStr", js_to_str, 1) &&
           jsb_set_fn_n(ctx, m, "sum", js_sum, 1) &&
           jsb_set_fn_n(ctx, m, "tryOr", js_try_or, 1) &&
           jsb_set_fn(ctx, m, "clearThenThrow", js_clear_then_throw) &&
           jsb_set_fn(ctx, m, "kinds", js_kinds) &&
           jsb_set_fn(ctx, m, "err", js_err) &&
           jsb_set_fn(ctx, m, "internal", js_internal) &&
           jsb_set_fn(ctx, m, "oom", js_oom) &&
           jsb_set_fn(ctx, m, "nullStr", js_null_str) &&
           jsb_set_fn(ctx, m, "badLen", js_bad_len) &&
           jsb_set_fn_n(ctx, m, "f3", js_err, 3);
}

/* Setup callback: defines `__t`. */
static bool define_t(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__t", init_t);
}

/* The checks, run in JS; each failure is listed in the result. */
static const char k_script[] =
    "(function () { var t = __t, e = [];"
    "function eq(a, b, w) { if (a !== b) e.push(w + ': ' + String(a)); }"
    "function throws(f, ctor, w) { try { f(); e.push(w + ': no throw'); }"
    "  catch (x) { if (!(x instanceof ctor)) e.push(w + ': ' + x); } }"
    "var o = t.make();"
    "eq(o.i, -5, 'i'); eq(o.u, 4000000000, 'u'); eq(o.f, 1.5, 'f');"
    "eq(o.t, true, 't'); eq(o.n, null, 'n'); eq(o.s, 'h\\u00e9llo', 's');"
    "eq(o.z, 'a\\u0000b', 'z'); eq(o.z.length, 3, 'z.length'); eq(o.e, '', 'e');"
    "eq(Array.isArray(o.arr), true, 'arr'); eq(o.arr.join(), '1,x', 'arr items');"
    "eq(o.buf instanceof ArrayBuffer, true, 'buf');"
    "eq(new Uint8Array(o.buf).join(), '1,2,3', 'buf bytes');"
    "eq(o.none.byteLength, 0, 'empty buf');"
    "eq(typeof o.version === 'string' && o.version.length > 0, true, 'version');"
    "var vo = { valueOf: function () { return 7; } };"
    "eq(t.toI32('42'), 42, 'i32 str'); eq(t.toI32(4294967297), 1, 'i32 wrap');"
    "eq(t.toI32(vo), 7, 'i32 valueOf'); eq(t.toI32(NaN), 0, 'i32 NaN');"
    "eq(t.toI32(), 0, 'i32 missing'); eq(t.toI32(-1.9), -1, 'i32 trunc');"
    "eq(t.toU32(-1), 4294967295, 'u32'); eq(t.toU32(vo), 7, 'u32 valueOf');"
    "eq(t.toF64('2.5'), 2.5, 'f64 str'); eq(isNaN(t.toF64('x')), true, 'f64 NaN');"
    "eq(t.toF64(null), 0, 'f64 null'); eq(t.toF64(vo), 7, 'f64 valueOf');"
    "throws(function () { t.toI32(Symbol()); }, TypeError, 'i32 symbol');"
    "throws(function () { t.toF64(Symbol()); }, TypeError, 'f64 symbol');"
    "throws(function () { t.toU32({ valueOf: function () {"
    "  throw new RangeError('v'); } }); }, RangeError, 'valueOf throws');"
    "eq(t.truthy(''), false, 'b empty'); eq(t.truthy('0'), true, 'b str');"
    "eq(t.truthy(0), false, 'b 0'); eq(t.truthy({}), true, 'b obj');"
    "eq(t.truthy(Symbol()), true, 'b symbol'); eq(t.truthy(), false, 'b missing');"
    "eq(t.truthy(NaN), false, 'b NaN');"
    "var r = t.toStr('a\\u0000b'); eq(r[0], 'a\\u0000b', 'str nul');"
    "eq(r[1], 3, 'str nul len'); eq(t.toStr('\\u00e9')[1], 2, 'str utf8 len');"
    "eq(t.toStr(12)[0], '12', 'str num'); eq(t.toStr(null)[0], 'null', 'str null');"
    "eq(t.toStr()[0], 'undefined', 'str missing');"
    "eq(t.toStr({ toString: function () { return 'T'; } })[0], 'T', 'str obj');"
    "throws(function () { t.toStr(Symbol('q')); }, TypeError, 'str symbol');"
    "eq(t.sum([1, '2', vo]), 10, 'sum'); eq(t.sum([]), 0, 'sum empty');"
    "eq(t.sum([-1]), -1 - 1 - 4294967295, 'sum i32/u32');"
    "throws(function () { t.sum({ length: 1, 0: 1 }); }, TypeError, 'sum obj');"
    "throws(function () { t.sum([Symbol()]); }, TypeError, 'sum symbol');"
    "eq(t.tryOr(Symbol()), -1, 'tryOr fallback'); eq(t.tryOr('5'), 5, 'tryOr');"
    "throws(function () { t.clearThenThrow(); }, RangeError, 'clear');"
    "eq(t.kinds(undefined, null, true, 1, 's', {}, Symbol(), function () {}, []),"
    "  '0,1,2,3,4,5,5,5,5', 'kinds');"
    "try { t.err(); e.push('err: no throw'); } catch (x) {"
    "  eq(Object.getPrototypeOf(x), Error.prototype, 'err proto');"
    "  eq(x.name, 'Error', 'err name'); eq(x.message, 'boom 7', 'err msg'); }"
    "try { t.internal(); e.push('internal: no throw'); } catch (x) {"
    "  eq(x.name, 'InternalError', 'internal name');"
    "  eq(String(x), 'InternalError: deep', 'internal str');"
    "  eq(x instanceof Error, true, 'internal is Error'); }"
    "try { t.oom(); } catch (x) {"
    "  eq(String(x), 'InternalError: out of memory', 'oom'); }"
    "throws(function () { t.nullStr(); }, TypeError, 'null str');"
    "throws(function () { t.badLen(); }, RangeError, 'bad length');"
    "eq(t.f3.length, 3, 'f3.length'); eq(t.f3.name, 'f3', 'f3.name');"
    "eq(t.make.length, 0, 'make.length'); eq(t.toStr.length, 1, 'toStr.length');"
    "return e.length ? e.join('; ') : 'ok'; })()";

int main(int argc, char **argv)
{
    (void)argc;
    printf("=== jsb value seam tests ===\n");
    jsb_runtime *rt = jsb_runtime_create(argv[0], SMZ_V8_ICU_DATA_PATH);
    assert(rt != NULL);
    assert(jsb_runtime_setup(rt, define_t, NULL));
    char out[2048] = "";
    bool ok = jsb_runtime_eval(rt, k_script, "values.js", out, sizeof out);
    printf("  result: %s\n", out);
    assert(ok);
    assert(strcmp(out, "ok") == 0);
    jsb_runtime_destroy(rt);
    printf("OK\n");
    return 0;
}
