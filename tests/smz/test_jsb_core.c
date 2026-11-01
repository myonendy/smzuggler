/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Tests the jsb seam core (src/jsb/jsb.h) from plain C, as a binding would
 * use it.
 *
 * Why: the bindings ported in M1 rely on each getter, return and throw
 * behaving as docs/js-binding.md says, including type errors and missing
 * arguments. Writing the test in C also proves that jsb.h needs no C++.
 * Handle rules: no jsb_val is kept past the native call that made it
 * (spec section 15.8, docs/js-binding.md). Main thread only.
 */
#include "jsb.h"

#include <stdio.h>
#include <string.h>

static int g_failures;

/* Runs `src` and checks that it completes with String(result) == `expect`;
 * counts and prints a failure otherwise. */
static void expect_js(jsb_runtime *rt, const char *src, const char *expect) {
    char out[512];
    bool ok = jsb_runtime_eval(rt, src, "test_jsb_core.js", out, sizeof out);
    if (!ok || strcmp(out, expect) != 0) {
        fprintf(stderr, "FAIL: %s\n  expected %s, got %s%s\n", src, expect,
                ok ? "" : "exception ", out);
        g_failures++;
    }
}

/* Returns arg0 + 1 as int32 (tests jsb_arg_i32 and jsb_ret_i32). */
static void fn_i32(jsb_ctx *ctx) {
    int32_t v;
    if (!jsb_arg_i32(ctx, 0, &v)) return;
    jsb_ret_i32(ctx, (int32_t)((uint32_t)v + 1u));
}

/* Returns arg0 * 2 as a double (jsb_arg_f64, jsb_ret_f64). */
static void fn_f64(jsb_ctx *ctx) {
    double v;
    if (!jsb_arg_f64(ctx, 0, &v)) return;
    jsb_ret_f64(ctx, v * 2.0);
}

/* Returns !arg0 (jsb_arg_bool, jsb_ret_bool). */
static void fn_bool(jsb_ctx *ctx) {
    bool v;
    if (!jsb_arg_bool(ctx, 0, &v)) return;
    jsb_ret_bool(ctx, !v);
}

/* Returns "<arg0>:<byte length>" using an 8-byte buffer (jsb_arg_str,
 * jsb_ret_str); longer strings must raise a RangeError. */
static void fn_str(jsb_ctx *ctx) {
    char buf[8], out[32];
    size_t len = 0;
    if (!jsb_arg_str(ctx, 0, buf, sizeof buf, &len)) return;
    snprintf(out, sizeof out, "%s:%zu", buf, len);
    jsb_ret_str(ctx, out);
}

/* Returns arg0[arg1] (jsb_arg_obj, jsb_get_prop, jsb_ret_val). */
static void fn_get(jsb_ctx *ctx) {
    char key[32];
    jsb_val *obj = jsb_arg_obj(ctx, 0);
    if (!obj || !jsb_arg_str(ctx, 1, key, sizeof key, NULL)) return;
    jsb_ret_val(ctx, jsb_get_prop(ctx, obj, key));
}

/* Returns a new object whose "v" is arg0 (jsb_new_object, jsb_set_prop,
 * jsb_arg_val). */
static void fn_box(jsb_ctx *ctx) {
    jsb_val *obj = jsb_new_object(ctx);
    if (!jsb_set_prop(ctx, obj, "v", jsb_arg_val(ctx, 0))) return;
    jsb_ret_val(ctx, obj);
}

/* Returns the argument count (jsb_arg_count). */
static void fn_count(jsb_ctx *ctx) { jsb_ret_i32(ctx, jsb_arg_count(ctx)); }

/* Sets a result, then resets it (jsb_ret_undefined: the last call wins). */
static void fn_undef(jsb_ctx *ctx) {
    jsb_ret_i32(ctx, 7);
    jsb_ret_undefined(ctx);
}

/* Throws a TypeError, then tries a RangeError and a return that must both
 * be ignored (jsb_throw_type, first throw wins). */
static void fn_throw_type(jsb_ctx *ctx) {
    jsb_throw_type(ctx, "bad %s %d", "thing", 3);
    jsb_throw_range(ctx, "ignored");
    jsb_ret_i32(ctx, 1);
}

/* Throws a RangeError (jsb_throw_range). */
static void fn_throw_range(jsb_ctx *ctx) { jsb_throw_range(ctx, "out of %s", "range"); }

/* Fills the test module, including a nested object made with jsb_set_fn. */
static bool init_module(jsb_ctx *ctx, jsb_val *m) {
    jsb_val *nested = jsb_new_object(ctx);
    return jsb_set_fn(ctx, m, "i32", fn_i32) && jsb_set_fn(ctx, m, "f64", fn_f64) &&
           jsb_set_fn(ctx, m, "bool", fn_bool) && jsb_set_fn(ctx, m, "str", fn_str) &&
           jsb_set_fn(ctx, m, "get", fn_get) && jsb_set_fn(ctx, m, "box", fn_box) &&
           jsb_set_fn(ctx, m, "count", fn_count) &&
           jsb_set_fn(ctx, m, "undef", fn_undef) &&
           jsb_set_fn(ctx, m, "throwType", fn_throw_type) &&
           jsb_set_fn(ctx, m, "throwRange", fn_throw_range) &&
           jsb_set_fn(ctx, nested, "count", fn_count) &&
           jsb_set_prop(ctx, m, "nested", nested);
}

/* A module init that fails, so its global must never appear. */
static bool init_failing(jsb_ctx *ctx, jsb_val *m) {
    (void)ctx;
    (void)m;
    return false;
}

/* Defines both test modules; `user` carries a bool* for the failing one. */
static bool setup(jsb_ctx *ctx, void *user) {
    *(bool *)user = jsb_define_module(ctx, "__native_broken", init_failing);
    return jsb_define_module(ctx, "__native_test", init_module);
}

/* Checks each getter's happy path and coercion rules. */
static void test_getters(jsb_runtime *rt) {
    expect_js(rt, "__native_test.i32(41)", "42");
    expect_js(rt, "__native_test.i32(2147483647)", "-2147483648");
    expect_js(rt, "__native_test.i32(4294967297.9)", "2");
    expect_js(rt, "__native_test.i32(NaN)", "1");
    expect_js(rt, "__native_test.f64(1.25)", "2.5");
    expect_js(rt, "__native_test.bool(false)", "true");
    expect_js(rt, "__native_test.str('h\\u00e9')", "h\xc3\xa9:3");
    expect_js(rt, "__native_test.str('1234567')", "1234567:7");
    expect_js(rt, "__native_test.get({a: 5}, 'a')", "5");
    expect_js(rt, "__native_test.get([], 'missing')", "undefined");
    expect_js(rt, "__native_test.box('x').v", "x");
    expect_js(rt, "__native_test.box().v === undefined", "true");
    expect_js(rt, "__native_test.count(1, 2, 3)", "3");
    expect_js(rt, "__native_test.nested.count()", "0");
    expect_js(rt, "__native_test.undef()", "undefined");
    expect_js(rt, "__native_test.i32.name", "i32");
}

/* Returns JS that calls `call` and yields the error's "Type: message". */
#define CATCH(call) "try { " call "; 'no throw' } catch (e) { e.name + ': ' + e.message }"

/* Checks wrong types, missing arguments, size limits and explicit throws. */
static void test_errors(jsb_runtime *rt) {
    expect_js(rt, CATCH("__native_test.i32('1')"), "TypeError: argument 1 must be a number");
    expect_js(rt, CATCH("__native_test.i32()"), "TypeError: argument 1 must be a number");
    expect_js(rt, CATCH("__native_test.f64(true)"), "TypeError: argument 1 must be a number");
    expect_js(rt, CATCH("__native_test.f64()"), "TypeError: argument 1 must be a number");
    expect_js(rt, CATCH("__native_test.bool(1)"), "TypeError: argument 1 must be a boolean");
    expect_js(rt, CATCH("__native_test.bool()"), "TypeError: argument 1 must be a boolean");
    expect_js(rt, CATCH("__native_test.str(5)"), "TypeError: argument 1 must be a string");
    expect_js(rt, CATCH("__native_test.str()"), "TypeError: argument 1 must be a string");
    expect_js(rt, CATCH("__native_test.str('12345678')"),
              "RangeError: argument 1 is 8 bytes, the limit is 7");
    expect_js(rt, CATCH("__native_test.get(null, 'a')"), "TypeError: argument 1 must be an object");
    expect_js(rt, CATCH("__native_test.get('s', 'a')"), "TypeError: argument 1 must be an object");
    expect_js(rt, CATCH("__native_test.get({})"), "TypeError: argument 2 must be a string");
    expect_js(rt, CATCH("__native_test.get({get a() { throw new Error('g') }}, 'a')"),
              "Error: g");
    expect_js(rt, CATCH("__native_test.throwType()"), "TypeError: bad thing 3");
    expect_js(rt, "try { __native_test.throwType() } catch (e) { e instanceof TypeError }",
              "true");
    expect_js(rt, CATCH("__native_test.throwRange()"), "RangeError: out of range");
}

/* Creates a runtime, defines the modules and runs every check. */
int main(int argc, char **argv) {
    (void)argc;
    jsb_runtime *rt = jsb_runtime_create(argv[0], SMZ_V8_ICU_DATA_PATH);
    if (!rt) {
        fprintf(stderr, "FAIL: jsb_runtime_create\n");
        return 1;
    }
    bool broken_defined = true;
    if (!jsb_runtime_setup(rt, setup, &broken_defined)) {
        fprintf(stderr, "FAIL: module setup\n");
        g_failures++;
    }
    if (broken_defined) {
        fprintf(stderr, "FAIL: failing init reported success\n");
        g_failures++;
    }
    expect_js(rt, "typeof __native_broken", "undefined");
    expect_js(rt, "typeof __native_test", "object");
    test_getters(rt);
    test_errors(rt);
    char out[64];
    if (jsb_runtime_eval(rt, "throw new RangeError('top')", "x.js", out, sizeof out) ||
        strcmp(out, "RangeError: top") != 0) {
        fprintf(stderr, "FAIL: eval exception reporting gave %s\n", out);
        g_failures++;
    }
    jsb_runtime_destroy(rt);
    printf("test_jsb_core: %d failure(s)\n", g_failures);
    return g_failures ? 1 : 0;
}
