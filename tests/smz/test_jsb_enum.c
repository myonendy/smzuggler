/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * The jsb seam calls of card M1-T026b: jsb_set_fn/jsb_set_fn_n define
 * non-enumerable functions, and jsb_arg_is_array_buffer is a strict
 * ArrayBuffer brand check.
 *
 * Why: binding ports must expose the same JS API as QuickJS's JS_CFUNC_DEF
 * and keep their ArrayBuffer-only rules, on both backends. Built against
 * smz_jsb (V8 in v8 mode) and the transitional backend (tests/smz/
 * jsb_enum.cmake); a JS script checks the results and returns "ok" or the
 * list of failures. Checks use assert() with NDEBUG undefined.
 */

#include "jsb.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* t.zero(): a jsb_set_fn function with arity 0; returns 7. */
static void js_zero(jsb_ctx *ctx) { jsb_ret_i32(ctx, 7); }

/* t.two(a, b): a jsb_set_fn_n function with arity 2; returns a + b. */
static void js_two(jsb_ctx *ctx)
{
    int32_t a, b;
    if (!jsb_arg_i32(ctx, 0, &a) || !jsb_arg_i32(ctx, 1, &b)) return;
    jsb_ret_i32(ctx, a + b);
}

/* t.isArrayBuffer(x): the shared brand check, exposed for the JS checks. */
static void js_is_array_buffer(jsb_ctx *ctx)
{
    jsb_ret_bool(ctx, jsb_arg_is_array_buffer(ctx, 0));
}

/* Fills the test module `__t`. */
static bool init_t(jsb_ctx *ctx, jsb_val *m)
{
    return jsb_set_fn(ctx, m, "zero", js_zero) &&
           jsb_set_fn_n(ctx, m, "two", js_two, 2) &&
           jsb_set_fn_n(ctx, m, "isArrayBuffer", js_is_array_buffer, 1);
}

/* Setup callback: defines `__t`. */
static bool define_t(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__t", init_t);
}

/* The checks, run in JS; each failure is listed in the result. */
static const char k_script[] =
    "(function () { var m = __t, e = [];"
    "function eq(a, b, w) { if (a !== b) e.push(w + ': ' + String(a)); }"
    "function attrs(n) { var d = Object.getOwnPropertyDescriptor(m, n);"
    "  return [d.writable, d.enumerable, d.configurable].join(); }"
    "eq(Object.keys(m).length, 0, 'Object.keys');"
    "eq(attrs('zero'), 'true,false,true', 'zero attrs');"
    "eq(attrs('two'), 'true,false,true', 'two attrs');"
    "eq(attrs('isArrayBuffer'), 'true,false,true', 'isArrayBuffer attrs');"
    "eq(m.zero.length, 0, 'zero.length'); eq(m.zero.name, 'zero', 'zero.name');"
    "eq(m.two.length, 2, 'two.length'); eq(m.two.name, 'two', 'two.name');"
    "eq(m.isArrayBuffer.length, 1, 'isArrayBuffer.length');"
    "eq(m.isArrayBuffer.name, 'isArrayBuffer', 'isArrayBuffer.name');"
    "eq(m.zero(), 7, 'zero call'); eq(m.two(2, 3), 5, 'two call');"
    "m.zero = function () { return 9; }; eq(m.zero(), 9, 'reassign');"
    "eq(delete m.zero, true, 'delete'); eq('zero' in m, false, 'deleted');"
    "eq(m.isArrayBuffer(new ArrayBuffer(4)), true, 'ArrayBuffer');"
    "eq(m.isArrayBuffer(new Uint8Array(4)), false, 'TypedArray');"
    "eq(m.isArrayBuffer(new DataView(new ArrayBuffer(4))), false, 'DataView');"
    "eq(m.isArrayBuffer({}), false, 'plain object');"
    "eq(m.isArrayBuffer(5), false, 'number');"
    "eq(m.isArrayBuffer(), false, 'missing');"
    "var ab = new ArrayBuffer(2);"
    "Object.defineProperty(ab, 'byteOffset',"
    "  { get: function () { throw new Error('getter'); } });"
    "eq(m.isArrayBuffer(ab), true, 'shadowed ArrayBuffer');"
    "return e.length ? e.join('; ') : 'ok'; })()";

int main(int argc, char **argv)
{
    (void)argc;
    printf("=== jsb enumeration and ArrayBuffer tests ===\n");
    jsb_runtime *rt = jsb_runtime_create(argv[0], SMZ_V8_ICU_DATA_PATH);
    assert(rt != NULL);
    assert(jsb_runtime_setup(rt, define_t, NULL));
    char out[2048] = "";
    bool ok = jsb_runtime_eval(rt, k_script, "enum.js", out, sizeof out);
    printf("  result: %s\n", out);
    assert(ok);
    assert(strcmp(out, "ok") == 0);
    jsb_runtime_destroy(rt);
    printf("OK\n");
    return 0;
}
