/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Tests the jsb persistent callbacks (src/jsb/jsb.h): jsb_callback_new,
 * jsb_callback_call, jsb_callback_free and jsb_set_error_handler.
 *
 * Why: audio `onended` and video events keep a JS function past the call
 * that handed it to C and run it later from the host. That must work after
 * the creating scope has ended, keep exceptions out of C, release the
 * function on free, and refuse freed handles, in both engine modes
 * (docs/js-binding.md, "Persistent callbacks"). gc() comes from the
 * "--expose-gc" engine flag. Handle rules: no jsb_val is kept past its
 * native call; only jsb_callback handles are (spec section 15.8,
 * docs/js-binding.md). Main thread only, except the external buffer's free
 * function, which V8 may run on a GC helper thread (hence the atomic).
 */
#include "jsb.h"

#include <assert.h>

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXT_LEN 8

/* The callback kept by B.keep, the one C calls. */
static jsb_callback g_cb;

/* How many times ext_free has run. */
static atomic_int g_frees;

/* What the error handler last received, and how often it ran. */
static char g_err_msg[256];
static char g_err_stack[1024];
static int g_err_count;

/* Runs `src` and asserts that it completes with String(result) == `expect`;
 * prints the mismatch first so a failing assert says what went wrong. */
static void expect_js(jsb_runtime *rt, const char *src, const char *expect) {
    char out[512];
    bool ok = jsb_runtime_eval(rt, src, "test_jsb_callbacks.js", out,
                               sizeof out);
    if (!ok || strcmp(out, expect) != 0)
        fprintf(stderr, "FAIL: %s\n  expected %s, got %s%s\n", src, expect,
                ok ? "" : "exception ", out);
    assert(ok);
    assert(strcmp(out, expect) == 0);
}

/* Records a callback exception for the assertions; `user` must be
 * &g_err_count. */
static void on_error(const char *message, const char *stack, void *user) {
    assert(user == (void *)&g_err_count);
    snprintf(g_err_msg, sizeof g_err_msg, "%s", message);
    snprintf(g_err_stack, sizeof g_err_stack, "%s", stack);
    g_err_count++;
}

/* B.keep(fn): replaces g_cb with a handle to `fn` (the old one is freed).
 * A non-function makes jsb_callback_new throw its TypeError. */
static void fn_keep(jsb_ctx *ctx) {
    jsb_callback cb;
    if (!jsb_callback_new(ctx, jsb_arg_val(ctx, 0), &cb)) return;
    if (g_cb.rt) jsb_callback_free(&g_cb);
    g_cb = cb;
}

/* B.drop(): frees g_cb from JS; returns whether jsb accepted the free. */
static void fn_drop(jsb_ctx *ctx) { jsb_ret_bool(ctx, jsb_callback_free(&g_cb)); }

/* B.callKept(n): calls g_cb(n) from inside a native call and returns the
 * status, so JS can see that the callee's exception did not reach it. */
static void fn_call_kept(jsb_ctx *ctx) {
    double n;
    if (!jsb_arg_f64(ctx, 0, &n)) return;
    jsb_cval arg = {JSB_TYPE_NUMBER, false, n, NULL};
    jsb_ret_i32(ctx, jsb_callback_call(g_cb, 1, &arg, NULL));
}

/* Counts frees of fn_ext's memory; `user` must be &g_frees. */
static void ext_free(void *data, size_t len, void *user) {
    assert(len == EXT_LEN);
    assert(user == (void *)&g_frees);
    free(data);
    atomic_fetch_add(&g_frees, 1);
}

/* B.ext(): a fresh external ArrayBuffer whose free is counted, used as a
 * weak probe: it is freed only once whatever holds it is collected. */
static void fn_ext(jsb_ctx *ctx) {
    void *data = calloc(1, EXT_LEN);
    assert(data);
    jsb_val *buf = jsb_new_external_buffer(ctx, data, EXT_LEN, ext_free,
                                           &g_frees);
    if (!buf) {
        free(data);
        return;
    }
    jsb_ret_val(ctx, buf);
}

/* Fills the B module. */
static bool init_b(jsb_ctx *ctx, jsb_val *m) {
    return jsb_set_fn(ctx, m, "keep", fn_keep) &&
           jsb_set_fn(ctx, m, "drop", fn_drop) &&
           jsb_set_fn(ctx, m, "callKept", fn_call_kept) &&
           jsb_set_fn(ctx, m, "ext", fn_ext);
}

/* Setup callback: defines the global B. */
static bool setup(jsb_ctx *ctx, void *user) {
    (void)user;
    return jsb_define_module(ctx, "B", init_b);
}

/* Calls gc() until the free count reaches `want`, at most 8 times: V8 may
 * finish freeing backing stores on a helper thread. Returns the count. */
static int collect_until(jsb_runtime *rt, int want) {
    for (int i = 0; i < 8 && atomic_load(&g_frees) < want; i++)
        expect_js(rt, "gc(); 0", "0");
    return atomic_load(&g_frees);
}

/* Calls g_cb with numbers a and b and returns the Number result. */
static double call2(double a, double b) {
    jsb_cval args[2] = {{JSB_TYPE_NUMBER, false, a, NULL},
                        {JSB_TYPE_NUMBER, false, b, NULL}};
    jsb_result r = {0};
    assert(jsb_callback_call(g_cb, 2, args, &r) == JSB_OK);
    assert(r.type == JSB_TYPE_NUMBER);
    return r.num;
}

/* A function kept in a native call runs later from C, with no JS on the
 * stack, after the creating scope and its locals are gone. */
static void test_after_scope(jsb_runtime *rt) {
    expect_js(rt, "(function () { var k = 10;"
                  " B.keep(function (a, b) { return a * b + k; }); })(); 0",
              "0");
    assert(call2(3, 4) == 22);
    expect_js(rt, "gc(); 0", "0"); /* the handle keeps it alive */
    assert(call2(-1, 0.5) == 9.5);
}

/* Every argument type arrives as the matching JS value; every result type
 * comes back, strings copied (and truncation reported by str_len). */
static void test_args_and_result(jsb_runtime *rt) {
    expect_js(rt, "B.keep(function () { return Array.prototype.map.call("
                  "arguments, function (v) { return v === null ? 'null' :"
                  " typeof v + ':' + v; }).join(); }); 0", "0");
    jsb_cval args[5] = {{JSB_TYPE_UNDEFINED, false, 0, NULL},
                        {JSB_TYPE_NULL, false, 0, NULL},
                        {JSB_TYPE_BOOL, true, 0, NULL},
                        {JSB_TYPE_NUMBER, false, 2.5, NULL},
                        {JSB_TYPE_STRING, false, 0, "h\xc3\xa9"}};
    char buf[64];
    jsb_result r = {0};
    r.str = buf;
    r.str_cap = sizeof buf;
    assert(jsb_callback_call(g_cb, 5, args, &r) == JSB_OK);
    assert(r.type == JSB_TYPE_STRING);
    const char *want = "undefined:undefined,null,boolean:true,number:2.5,"
                       "string:h\xc3\xa9";
    assert(strcmp(buf, want) == 0 && r.str_len == strlen(want));
    r.str_cap = 6; /* truncated, NUL-terminated, full length reported */
    assert(jsb_callback_call(g_cb, 5, args, &r) == JSB_OK);
    assert(strcmp(buf, "undef") == 0 && r.str_len == strlen(want));

    expect_js(rt, "B.keep(function (x) { return x; }); 0", "0");
    jsb_cval t = {JSB_TYPE_BOOL, false, 0, NULL};
    assert(jsb_callback_call(g_cb, 1, &t, &r) == JSB_OK);
    assert(r.type == JSB_TYPE_BOOL && !r.b);
    assert(jsb_callback_call(g_cb, 0, NULL, &r) == JSB_OK);
    assert(r.type == JSB_TYPE_UNDEFINED);
    expect_js(rt, "B.keep(function () { return {}; }); 0", "0");
    assert(jsb_callback_call(g_cb, 0, NULL, &r) == JSB_OK);
    assert(r.type == JSB_TYPE_OBJECT);
}

/* A throwing callback goes to the error handler with its message and
 * stack, returns JSB_ERR_THREW, and never reaches an enclosing JS caller. */
static void test_exception(jsb_runtime *rt) {
    jsb_set_error_handler(rt, on_error, &g_err_count);
    expect_js(rt, "B.keep(function (n) { throw new Error('boom ' + n); }); 0",
              "0");
    jsb_cval arg = {JSB_TYPE_NUMBER, false, 7, NULL};
    jsb_result r = {JSB_TYPE_NUMBER, true, 1, NULL, 0, 0};
    assert(jsb_callback_call(g_cb, 1, &arg, &r) == JSB_ERR_THREW);
    assert(r.type == JSB_TYPE_UNDEFINED);
    assert(g_err_count == 1);
    assert(strcmp(g_err_msg, "Error: boom 7") == 0);
    assert(strstr(g_err_stack, "test_jsb_callbacks.js") != NULL);
    /* From inside a native call: JS sees the status, not the exception. */
    expect_js(rt, "try { B.callKept(8); } catch (e) { 'leaked ' + e; }", "-1");
    assert(g_err_count == 2 && strcmp(g_err_msg, "Error: boom 8") == 0);
    expect_js(rt, "B.keep(function () { throw 42; }); 0", "0");
    assert(jsb_callback_call(g_cb, 0, NULL, NULL) == JSB_ERR_THREW);
    assert(g_err_count == 3 && strcmp(g_err_msg, "42") == 0);
    assert(strcmp(g_err_stack, "") == 0);
    /* The runtime still works after the errors. */
    expect_js(rt, "1 + 1", "2");
}

/* The kept function holds a weak probe (an external buffer): GC cannot
 * free it while the handle lives, and does once the handle is freed. */
static void test_free_releases(jsb_runtime *rt) {
    expect_js(rt, "(function () { var f = function () { return 1; };"
                  " f.probe = B.ext(); B.keep(f); })(); 0", "0");
    assert(collect_until(rt, 1) == 0);
    jsb_callback copy = g_cb;
    jsb_callback stale = g_cb;
    assert(jsb_callback_free(&g_cb));
    assert(g_cb.rt == NULL);
    assert(collect_until(rt, 1) == 1);

    /* Freed handles are checked errors, through any copy. */
    assert(jsb_callback_call(copy, 0, NULL, NULL) == JSB_ERR_HANDLE);
    assert(jsb_callback_call(g_cb, 0, NULL, NULL) == JSB_ERR_HANDLE);
    assert(!jsb_callback_free(&copy));
    assert(!jsb_callback_free(&g_cb));
    /* Still refused after a new callback reuses the slot. */
    expect_js(rt, "B.keep(function () { return 5; }); 0", "0");
    assert(jsb_callback_call(stale, 0, NULL, NULL) == JSB_ERR_HANDLE);
    assert(call2(0, 0) == 5);
}

/* Misuse from C and JS is refused without running anything. */
static void test_misuse(jsb_runtime *rt) {
    expect_js(rt, "B.keep(function () { throw new Error('ran'); }); 0", "0");
    int before = g_err_count;
    jsb_cval obj = {JSB_TYPE_OBJECT, false, 0, NULL};
    jsb_cval nul = {JSB_TYPE_STRING, false, 0, NULL};
    assert(jsb_callback_call(g_cb, -1, NULL, NULL) == JSB_ERR_ARG);
    assert(jsb_callback_call(g_cb, 1, NULL, NULL) == JSB_ERR_ARG);
    assert(jsb_callback_call(g_cb, 65, &obj, NULL) == JSB_ERR_ARG);
    assert(jsb_callback_call(g_cb, 1, &obj, NULL) == JSB_ERR_ARG);
    assert(jsb_callback_call(g_cb, 1, &nul, NULL) == JSB_ERR_ARG);
    assert(g_err_count == before);
    expect_js(rt, "try { B.keep(42); 'no' } catch (e) { e.name }",
              "TypeError");
    /* A callback that frees its own handle finishes normally. */
    expect_js(rt, "B.keep(function () { return B.drop() ? 9 : 0; }); 0", "0");
    jsb_callback self = g_cb;
    jsb_result r = {0};
    assert(jsb_callback_call(self, 0, NULL, &r) == JSB_OK);
    assert(r.type == JSB_TYPE_NUMBER && r.num == 9);
    assert(jsb_callback_call(self, 0, NULL, NULL) == JSB_ERR_HANDLE);
}

int main(int argc, char **argv) {
    (void)argc;
    assert(jsb_set_flags("--expose-gc"));
    jsb_runtime *rt = jsb_runtime_create(argv[0], SMZ_V8_ICU_DATA_PATH);
    assert(rt);
    assert(jsb_runtime_setup(rt, setup, NULL));
    test_after_scope(rt);
    test_args_and_result(rt);
    test_exception(rt);
    test_free_releases(rt);
    test_misuse(rt);
    /* A callback still alive at shutdown is released by destroy. */
    expect_js(rt, "B.keep(function () {}); 0", "0");
    jsb_runtime_destroy(rt);
    printf("jsb_callbacks: all tests passed\n");
    return 0;
}
