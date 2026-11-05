/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * The V8 engine's own jsb runtime (card M1-T027, spec section 4.1,
 * docs/js-binding.md).
 *
 * Why: the removed adoption test covered how Outsider's binding ports reach
 * the engine's globals. On V8 there is no adoption bridge: js_engine_get_jsb()
 * is the runtime, and bindings define their modules on it directly. This test
 * keeps that coverage plus the backend machinery the removed transitional test
 * checked (two live runtimes, repeated create/destroy), so the V8 backend is
 * exercised beyond the jsb contract tests. Checks use assert() with NDEBUG
 * undefined (tests/smz/js_engine_v8_jsb.cmake). Main thread only; every jsb_val
 * dies with its call and only jsb_callback outlives it (spec section 15.8).
 */

#include "engine/js_engine.h"
#include "jsb.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Last console line of each level, for the checks. */
static char g_log[512];
static char g_error[512];
/* How often the external buffer's free function ran. */
static int g_freed;
/* A callback kept alive until the engine shuts down. */
static jsb_callback g_kept;

/* Prints what the next assert checks, so a failure names it. */
static void note(const char *what)
{
    printf("  check: %s\n", what);
    fflush(stdout);
}

/* Console callback: keeps the last "log" and "error" line. */
static void on_console(const char *level, const char *msg, void *user)
{
    (void)user;
    char *dst = strcmp(level, "error") == 0 ? g_error : g_log;
    snprintf(dst, sizeof g_log, "%s", msg);
}

/* __adopt.add(a, b): the sum of ToNumber(a) and ToNumber(b). */
static void js_add(jsb_ctx *ctx)
{
    double a, b;
    if (!jsb_arg_to_f64(ctx, 0, &a) || !jsb_arg_to_f64(ctx, 1, &b)) return;
    jsb_ret_f64(ctx, a + b);
}

/* jsb_free_fn of the module's buffer: counts the release. */
static void count_free(void *data, size_t len, void *user)
{
    (void)data;
    (void)len;
    (void)user;
    g_freed++;
}

/* __adopt.keep(fn): keeps `fn` as a persistent jsb_callback. */
static void js_keep(jsb_ctx *ctx)
{
    jsb_ret_bool(ctx, jsb_callback_new(ctx, jsb_arg_val(ctx, 0), &g_kept));
}

/* __adopt.refuse(fn): reports that a non-function is refused, zeroing out. */
static void js_refuse(jsb_ctx *ctx)
{
    jsb_callback out = {0};
    bool ok = jsb_callback_new(ctx, jsb_arg_val(ctx, 0), &out);
    jsb_clear_exception(ctx);
    jsb_ret_bool(ctx, ok || out.slot != 0);
}

/* Fills __adopt with add (length 2), keep, refuse and a 4-byte buffer. */
static bool init_adopt(jsb_ctx *ctx, jsb_val *module)
{
    static unsigned char bytes[4] = {9, 8, 7, 6};
    jsb_val *buf = jsb_new_external_buffer(ctx, bytes, sizeof bytes,
                                           count_free, NULL);
    return buf && jsb_set_fn_n(ctx, module, "add", js_add, 2) &&
           jsb_set_fn_n(ctx, module, "keep", js_keep, 1) &&
           jsb_set_fn_n(ctx, module, "refuse", js_refuse, 1) &&
           jsb_set_prop(ctx, module, "buf", buf);
}

/* Setup callback: defines __adopt on the engine's runtime. */
static bool define_adopt(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__adopt", init_adopt);
}

/* A setup callback that fails after throwing, for the failure path. */
static bool define_broken(jsb_ctx *ctx, void *user)
{
    (void)user;
    jsb_throw_type(ctx, "broken module");
    return false;
}

/* Returns 1; registered only in runtime A. */
static void fn_one(jsb_ctx *ctx) { jsb_ret_i32(ctx, 1); }

/* Returns 2; registered only in runtime B. */
static void fn_two(jsb_ctx *ctx) { jsb_ret_i32(ctx, 2); }

/* Module init for runtime A. */
static bool init_a(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn(ctx, module, "which", fn_one);
}

/* Module init for runtime B. */
static bool init_b(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn(ctx, module, "which", fn_two);
}

/* Setup callback: defines __native_t with the init passed as `user`. */
static bool setup_module(jsb_ctx *ctx, void *user)
{
    return jsb_define_module(ctx, "__native_t", (jsb_module_init)user);
}

/* Creates a runtime whose __native_t comes from `init`; asserts it worked. */
static jsb_runtime *make_runtime(const char *argv0, jsb_module_init init)
{
    jsb_runtime *rt = jsb_runtime_create(argv0, SMZ_V8_ICU_DATA_PATH);
    assert(rt != NULL);
    assert(jsb_runtime_setup(rt, setup_module, (void *)init));
    return rt;
}

/* Runs `src` on `rt` and asserts String(result) == `expect`. */
static void expect_js(jsb_runtime *rt, const char *src, const char *expect)
{
    char out[512];
    bool ok = jsb_runtime_eval(rt, src, "engine_jsb.js", out, sizeof out);
    if (!ok || strcmp(out, expect) != 0)
        fprintf(stderr, "FAIL: %s\n  expected %s, got %s%s\n", src, expect,
                ok ? "" : "exception ", out);
    assert(ok);
    assert(strcmp(out, expect) == 0);
}

/* The engine's runtime sees the engine's globals, and a module defined on it
   is visible to engine scripts, including buffers and persistent callbacks. */
static void test_engine_adoption(void)
{
    JSEngine *engine = js_engine_init();
    note("engine starts");
    assert(engine != NULL);
    js_engine_set_console_callback(engine, on_console, NULL);

    jsb_runtime *rt = js_engine_get_jsb(engine);
    note("get_jsb is the engine's runtime");
    assert(rt != NULL);
    assert(js_engine_get_jsb(engine) == rt);
    assert(js_engine_get_jsb(NULL) == NULL);

    note("a failing setup reports failure and leaves no module");
    assert(!jsb_runtime_setup(rt, define_broken, NULL));
    assert(js_engine_eval(engine, "typeof __broken", "broken.js"));
    char out[16] = "";
    assert(jsb_runtime_eval(rt, "typeof __broken", "broken.js", out, sizeof out));
    assert(strcmp(out, "undefined") == 0);

    note("a module defined through it is visible to engine scripts");
    assert(jsb_runtime_setup(rt, define_adopt, NULL));
    assert(js_engine_eval(engine, "typeof __adopt", "type.js"));
    out[0] = '\0';
    assert(jsb_runtime_eval(rt, "typeof __adopt", "type.js", out, sizeof out));
    assert(strcmp(out, "object") == 0);
    assert(js_engine_eval(engine,
        "console.log(String(__adopt.add(2, '3')) + ' ' + __adopt.add.length"
        " + ' ' + new Uint8Array(__adopt.buf).join())", "adopt.js"));
    assert(strcmp(g_log, "5 2 9,8,7,6") == 0);
    assert(js_engine_eval(engine,
        "console.log(__adopt.buf.byteLength + ' ' + __adopt.keep.length)",
        "props.js"));
    assert(strcmp(g_log, "4 1") == 0);

    note("a jsb error reaches the engine script as a TypeError");
    assert(js_engine_eval(engine,
        "try { __adopt.add(Symbol()); console.log('no throw'); }"
        " catch (e) { console.log(e instanceof TypeError); }", "err.js"));
    assert(strcmp(g_log, "true") == 0);

    note("a non-callable is refused without leaking a callback");
    assert(js_engine_eval(engine, "console.log(__adopt.refuse(1))", "refuse.js"));
    assert(strcmp(g_log, "false") == 0);

    note("jsb_runtime_eval sees the engine's globals");
    assert(js_engine_eval(engine, "var fromEngine = 41;", "set.js"));
    out[0] = '\0';
    assert(jsb_runtime_eval(rt, "fromEngine + 1", "get.js", out, sizeof out));
    assert(strcmp(out, "42") == 0);

    note("rejection tracking still reports unhandled rejections");
    g_error[0] = '\0';
    assert(js_engine_eval(engine, "Promise.reject(new Error('lost'));",
                          "reject.js"));
    js_engine_execute_pending_jobs(engine);
    assert(strstr(g_error, "Unhandled promise rejection") != NULL);
    assert(strstr(g_error, "lost") != NULL);

    note("the watchdog still stops a runaway script, and the engine lives");
    js_engine_set_watchdog(engine, 50);
    assert(!js_engine_eval(engine, "for (;;) {}", "loop.js"));
    js_engine_set_watchdog(engine, 0);
    assert(js_engine_eval(engine, "console.log('alive ' + __adopt.add(1, 1))",
                          "after.js"));
    assert(strcmp(g_log, "alive 2") == 0);

    note("a live callback is released at shutdown, before the context");
    assert(js_engine_eval(engine,
        "console.log(__adopt.keep(function () {}))", "keep.js"));
    assert(strcmp(g_log, "true") == 0);
    note("shutdown frees the runtime, then the context and its buffers");
    assert(g_freed == 0);
    js_engine_shutdown(engine);
    assert(g_freed == 1);

    note("a fresh engine adopts again after shutdown");
    engine = js_engine_init();
    assert(engine != NULL && js_engine_get_jsb(engine) != NULL);
    js_engine_shutdown(engine);
}

/* Two live runtimes keep separate globals and separate function tables; a
   destroyed runtime leaves the other usable. */
static void test_runtime_isolation(const char *argv0)
{
    jsb_runtime *a = make_runtime(argv0, init_a);
    jsb_runtime *b = make_runtime(argv0, init_b);
    expect_js(a, "__native_t.which()", "1");
    expect_js(b, "__native_t.which()", "2");
    expect_js(a, "typeof __native_t.which", "function");
    expect_js(b, "typeof __native_t.which", "function");
    expect_js(a, "globalThis.onlyA = 7; typeof onlyA", "number");
    expect_js(b, "typeof onlyA", "undefined");
    jsb_runtime_destroy(b);
    expect_js(a, "__native_t.which()", "1");
    jsb_runtime_destroy(a);
}

/* Runtimes can be created, used and destroyed over and over. */
static void test_runtime_churn(const char *argv0)
{
    for (int i = 0; i < 10; i++) {
        jsb_runtime *rt = make_runtime(argv0, i % 2 ? init_b : init_a);
        expect_js(rt, "__native_t.which()", i % 2 ? "2" : "1");
        jsb_runtime_destroy(rt);
    }
}

/* Runs every check; any failure aborts through assert. */
int main(int argc, char **argv)
{
    (void)argc;
    printf("=== V8 engine-owned jsb runtime tests ===\n");
    test_engine_adoption();
    test_runtime_isolation(argv[0]);
    test_runtime_churn(argv[0]);
    printf("OK\n");
    return 0;
}
