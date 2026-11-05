/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Tests what is particular to the TRANSITIONAL jsb backend (src/jsb/,
 * through jsb.h only). Removed with that backend by card M1-T027.
 *
 * Why: test_jsb_core.c checks the seam's contract; this file checks the
 * QuickJS backend's own machinery that the contract test does not reach:
 * the per-call handle chunks (a call needing more handles than one chunk
 * holds), handles being released at the end of every call (many calls in
 * a row), the per-runtime function table (two live runtimes stay apart),
 * and runtimes being created and destroyed repeatedly.
 * Handle rules: no jsb_val is kept past the native call that made it
 * (spec section 15.8, docs/js-binding.md). Main thread only.
 */
#include "jsb.h"

#include <assert.h>

#include <stdio.h>
#include <string.h>

/* Handles made by one call in fn_chain; several times the backend's chunk
 * size of 16, so the chunk list must grow and keep earlier handles valid. */
#define CHAIN_LEN 100

/* Runs `src` on `rt` and asserts String(result) == `expect`, printing the
 * mismatch first so a failing assert says what went wrong. */
static void expect_js(jsb_runtime *rt, const char *src, const char *expect) {
    char out[512];
    bool ok = jsb_runtime_eval(rt, src, "test_jsb_transitional.js", out, sizeof out);
    if (!ok || strcmp(out, expect) != 0)
        fprintf(stderr, "FAIL: %s\n  expected %s, got %s%s\n", src, expect,
                ok ? "" : "exception ", out);
    assert(ok);
    assert(strcmp(out, expect) == 0);
}

/* Builds a linked list of CHAIN_LEN objects {i, next} in one call and
 * returns its head. Every node's handle is used again after all are made,
 * so a handle invalidated by chunk growth would corrupt the list. */
static void fn_chain(jsb_ctx *ctx) {
    jsb_val *nodes[CHAIN_LEN];
    for (int i = 0; i < CHAIN_LEN; i++) {
        nodes[i] = jsb_new_object(ctx);
        if (!nodes[i]) return;
    }
    for (int i = 0; i < CHAIN_LEN; i++) {
        jsb_val *undef = jsb_get_prop(ctx, nodes[i], "missing");
        if (!undef || !jsb_set_prop(ctx, nodes[i], "i", undef)) return;
    }
    for (int i = 0; i + 1 < CHAIN_LEN; i++)
        if (!jsb_set_prop(ctx, nodes[i], "next", nodes[i + 1])) return;
    jsb_ret_val(ctx, nodes[0]);
}

/* Returns a fresh object {v: arg0}; called many times in a loop to show
 * that each call's handles are released when it ends. */
static void fn_box(jsb_ctx *ctx) {
    jsb_val *v = jsb_arg_val(ctx, 0);
    jsb_val *box = jsb_new_object(ctx);
    if (!v || !box || !jsb_set_prop(ctx, box, "v", v)) return;
    jsb_ret_val(ctx, box);
}

/* Returns 1; registered only in runtime A. */
static void fn_one(jsb_ctx *ctx) { jsb_ret_i32(ctx, 1); }

/* Returns 2; registered only in runtime B. */
static void fn_two(jsb_ctx *ctx) { jsb_ret_i32(ctx, 2); }

/* Module init for runtime A: one function, `which`, returning 1, plus the
 * shared `chain` and `box` helpers. */
static bool init_a(jsb_ctx *ctx, jsb_val *m) {
    return jsb_set_fn(ctx, m, "which", fn_one) &&
           jsb_set_fn(ctx, m, "chain", fn_chain) &&
           jsb_set_fn(ctx, m, "box", fn_box);
}

/* Module init for runtime B: `which` returns 2. B registers a different
 * jsb_fn first, so a function table shared between runtimes would make one
 * of them call the wrong C function. */
static bool init_b(jsb_ctx *ctx, jsb_val *m) {
    return jsb_set_fn(ctx, m, "which", fn_two);
}

/* Setup callback: defines `__native_t` with the init passed as `user`. */
static bool setup(jsb_ctx *ctx, void *user) {
    return jsb_define_module(ctx, "__native_t", (jsb_module_init)user);
}

/* Creates a runtime with `__native_t` from `init`; asserts it worked. */
static jsb_runtime *make_runtime(jsb_module_init init) {
    jsb_runtime *rt = jsb_runtime_create("test_jsb_transitional", "");
    assert(rt);
    assert(jsb_runtime_setup(rt, setup, (void *)init));
    return rt;
}

/* One call that needs more handles than one chunk keeps them all valid. */
static void test_many_handles_in_one_call(jsb_runtime *rt) {
    expect_js(rt,
              "var n = __native_t.chain(), c = 0, ok = true;"
              "while (n) { ok = ok && ('i' in n); c++; n = n.next; }"
              "c + ':' + ok", "100:true");
}

/* Many calls in a row, each making handles, all return correct values. */
static void test_many_calls(jsb_runtime *rt) {
    expect_js(rt,
              "var s = 0; for (var k = 0; k < 100000; k++)"
              " s += __native_t.box(k).v; s", "4999950000");
}

/* Two live runtimes keep separate globals and separate function tables. */
static void test_two_runtimes(jsb_runtime *a) {
    jsb_runtime *b = make_runtime(init_b);
    expect_js(a, "__native_t.which()", "1");
    expect_js(b, "__native_t.which()", "2");
    expect_js(b, "typeof __native_t.chain", "undefined");
    expect_js(a, "globalThis.onlyA = 7; typeof onlyA", "number");
    expect_js(b, "typeof onlyA", "undefined");
    jsb_runtime_destroy(b);
    expect_js(a, "__native_t.which()", "1");
}

/* Runtimes can be created, used and destroyed over and over. */
static void test_runtime_churn(void) {
    for (int i = 0; i < 20; i++) {
        jsb_runtime *rt = make_runtime(i % 2 ? init_b : init_a);
        expect_js(rt, "__native_t.which()", i % 2 ? "2" : "1");
        jsb_runtime_destroy(rt);
    }
}

/* Runs every check; any failure aborts through assert. */
int main(void) {
    jsb_runtime *rt = make_runtime(init_a);
    test_many_handles_in_one_call(rt);
    test_many_calls(rt);
    test_two_runtimes(rt);
    jsb_runtime_destroy(rt);
    test_runtime_churn();
    printf("test_jsb_transitional: all checks passed\n");
    return 0;
}
