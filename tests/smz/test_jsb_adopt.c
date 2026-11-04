/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * The QuickJS engine's adopted jsb runtime (card M1-T007b, spec section
 * 4.1, docs/js-binding.md "Adoption").
 *
 * Why: binding ports define their modules through js_engine_get_jsb(), so
 * that runtime must share the game's globals while the engine's console,
 * rejection tracking and watchdog keep working, and shutdown must free it
 * before the context. Built on js_engine.c in both engine modes
 * (tests/smz/jsb_adopt.cmake). Checks use assert() with NDEBUG undefined.
 */

#include "engine/js_engine.h"
#include "jsb.h"
#include "jsb_adopt.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Last console line of each level, for the checks. */
static char g_log[512];
static char g_error[512];
/* How often the external buffer's free function ran. */
static int g_freed;

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

/* A callback kept alive until shutdown, which must release it before the
   context goes (QuickJS asserts on leaked objects otherwise). */
static jsb_callback g_kept;

/* __adopt.keep(fn): keeps `fn` as a persistent jsb_callback. */
static void js_keep(jsb_ctx *ctx)
{
    jsb_ret_bool(ctx, jsb_callback_new(ctx, jsb_arg_val(ctx, 0), &g_kept));
}

/* Fills __adopt with add (length 2), keep and an external 4-byte buffer. */
static bool init_adopt(jsb_ctx *ctx, jsb_val *module)
{
    static unsigned char bytes[4] = {9, 8, 7, 6};
    jsb_val *buf = jsb_new_external_buffer(ctx, bytes, sizeof bytes,
                                           count_free, NULL);
    return buf && jsb_set_fn_n(ctx, module, "add", js_add, 2) &&
           jsb_set_fn_n(ctx, module, "keep", js_keep, 1) &&
           jsb_set_prop(ctx, module, "buf", buf);
}

/* Setup callback: defines __adopt. */
static bool define_adopt(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__adopt", init_adopt);
}

int main(void)
{
    printf("=== jsb adoption tests ===\n");
    JSEngine *engine = js_engine_init();
    note("engine starts");
    assert(engine != NULL);
    js_engine_set_console_callback(engine, on_console, NULL);

    jsb_runtime *rt = js_engine_get_jsb(engine);
    note("get_jsb is not NULL and is the context's runtime");
    assert(rt != NULL);
    assert(jsb_runtime_of(js_engine_get_context(engine)) == rt);
    note("a second adoption of the same runtime is refused");
    assert(jsb_runtime_adopt(js_engine_get_context(engine)) == NULL);
    assert(jsb_runtime_of(NULL) == NULL);

    note("a module defined through it is visible to engine scripts");
    assert(jsb_runtime_setup(rt, define_adopt, NULL));
    assert(js_engine_eval(engine,
        "console.log(String(__adopt.add(2, '3')) + ' ' + __adopt.add.length"
        " + ' ' + new Uint8Array(__adopt.buf).join())", "adopt.js"));
    assert(strcmp(g_log, "5 2 9,8,7,6") == 0);
    note("a jsb error reaches the engine script as a TypeError");
    assert(js_engine_eval(engine,
        "try { __adopt.add(Symbol()); console.log('no throw'); }"
        " catch (e) { console.log(e instanceof TypeError); }", "err.js"));
    assert(strcmp(g_log, "true") == 0);
    note("jsb_runtime_eval sees the engine's globals");
    assert(js_engine_eval(engine, "var fromEngine = 41;", "set.js"));
    char out[16] = "";
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
    printf("OK\n");
    return 0;
}
