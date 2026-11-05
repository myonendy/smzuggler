/* SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: remove former-engine-only branches; retain V8 checks.

   Tests the engine-neutral JS error paths of error_handler.c (card M1-T011,
   spec section 4.1 item 5): error_handler_report_js_error, the window.onerror
   dispatch and crash auto-save, which reach JS only through js_engine.h, and
   error_handler_on_jsb_error for jsb callback exceptions (installed on the
   engine's jsb runtime in v8 mode).

   Why: these replace the QuickJS calls error_handler.c made, and must behave
   the same on js_engine.c and js_engine_v8.cc. Game-free: the DOM shim's
   __error_handler_dispatch and __error_handler_autoSave are stood in for by
   small scripts in this file. See docs/js-binding.md, "Error handler".

   Rules: main thread only; asserts stay live in Release (-UNDEBUG). */

#include "engine/error_handler.h"
#include "engine/js_engine.h"
#include "jsb.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Log lines seen by the error handler since the last reset_log(). */
static char g_log[8192];

/* Log callback: appends each message and a newline to g_log. */
static void capture(LogLevel level, const char *message, void *userdata)
{
    (void)userdata;
    size_t used = strlen(g_log);
    snprintf(g_log + used, sizeof g_log - used, "%s %s\n",
             error_handler_level_name(level), message);
}

/* Empties g_log before a case. */
static void reset_log(void)
{
    g_log[0] = '\0';
}

/* Evaluates `expr` and asserts its String() form is `want`. */
static void expect_js(JSEngine *engine, const char *expr, const char *want)
{
    char *got = js_engine_eval_string(engine, expr, "expect.js");
    if (!got || strcmp(got, want) != 0)
        fprintf(stderr, "expected '%s' from %s, got '%s'\n", want, expr,
                got ? got : "(null)");
    assert(got && strcmp(got, want) == 0);
    js_engine_free_string(got);
}

/* Logging without an engine: message, stack, NULL message, no dispatch. */
static void test_report_without_engine(void)
{
    reset_log();
    error_handler_report_js_error(NULL, "Error: plain", "Error: plain\n    at a.js:1:1", false);
    assert(strstr(g_log, "ERROR JS Exception: Error: plain\n"));
    assert(strstr(g_log, "ERROR Stack trace:\nError: plain\n    at a.js:1:1\n"));

    reset_log();
    error_handler_report_js_error(NULL, NULL, NULL, false);
    assert(strstr(g_log, "JS Exception: Unknown error"));
    assert(!strstr(g_log, "Stack trace"));
}

/* The jsb_error_fn logs scrubbed message and stack; an empty stack and a
   NULL message are handled. Engine-free, so it runs in both modes. */
static void test_on_jsb_error(void)
{
    reset_log();
    error_handler_on_jsb_error("Error: cb", "Error: cb\n    at c.js:2:5", NULL);
    assert(strstr(g_log, "ERROR JS Exception: Error: cb\n"));
    assert(strstr(g_log, "ERROR Stack trace:\nError: cb\n    at c.js:2:5\n"));

    reset_log();
    error_handler_on_jsb_error(NULL, "", NULL);
    assert(strstr(g_log, "ERROR JS Exception: Unknown error\n"));
    assert(!strstr(g_log, "Stack trace"));
}

/* window.onerror gets the exact text, quotes, backslashes, newlines and
   non-ASCII included, as an Error carrying the stack. */
static void test_dispatch(JSEngine *engine)
{
    assert(js_engine_eval(engine,
        "var got = null;\n"
        "globalThis.__error_handler_dispatch = function (m, e) {\n"
        "  got = [m, e instanceof Error, e.message === m, e.stack].join('|');\n"
        "};\n", "setup.js"));
    reset_log();
    error_handler_report_js_error(engine, "Error: say \"hi\" \\ caf\xc3\xa9",
                                  "Error: x\n    at b.js:3:7", false);
    expect_js(engine, "got",
              "Error: say \"hi\" \\ caf\xc3\xa9|true|true|Error: x\n    at b.js:3:7");
    assert(strstr(g_log, "JS Exception: Error: say"));
    assert(!strstr(g_log, "dispatch threw"));

    /* No stack: the rebuilt Error keeps its own; the message still arrives. */
    error_handler_report_js_error(engine, "TypeError: t", NULL, false);
    expect_js(engine, "got.split('|').slice(0, 3).join('|')", "TypeError: t|true|true");

    /* A literal end cannot be smuggled in through the message. */
    error_handler_report_js_error(engine, "\"); got = 'hijacked'; (\"", NULL, false);
    expect_js(engine, "got.split('|')[0]", "\"); got = 'hijacked'; (\"");
}

/* No dispatcher is a quiet no-op; a throwing one is logged as a warning. */
static void test_dispatch_failures(JSEngine *engine)
{
    assert(js_engine_eval(engine, "delete globalThis.__error_handler_dispatch;", "d.js"));
    reset_log();
    error_handler_report_js_error(engine, "Error: none", NULL, false);
    assert(strstr(g_log, "JS Exception: Error: none"));
    assert(!strstr(g_log, "WARN"));

    assert(js_engine_eval(engine,
        "globalThis.__error_handler_dispatch = function () { throw new Error('bad'); };",
        "t.js"));
    reset_log();
    error_handler_report_js_error(engine, "Error: first", NULL, false);
    assert(strstr(g_log, "WARN window.onerror dispatch threw"));
}

/* Auto-save: missing, success, zero result and a throw. */
static void test_auto_save(JSEngine *engine)
{
    reset_log();
    assert(!error_handler_attempt_auto_save(NULL));
    assert(!error_handler_attempt_auto_save(engine));
    assert(strstr(g_log, "WARN Auto-save function not available"));

    assert(js_engine_eval(engine,
        "var saves = 0;\n"
        "globalThis.__error_handler_autoSave = function () { saves++; return 1; };",
        "s.js"));
    reset_log();
    assert(error_handler_attempt_auto_save(engine));
    assert(strstr(g_log, "INFO Auto-save succeeded"));
    expect_js(engine, "saves", "1");

    assert(js_engine_eval(engine, "__error_handler_autoSave = function () { return 0; };", "z.js"));
    assert(!error_handler_attempt_auto_save(engine));

    assert(js_engine_eval(engine,
        "__error_handler_autoSave = function () { throw new Error('disk full'); };", "e.js"));
    reset_log();
    assert(!error_handler_attempt_auto_save(engine));
    assert(strstr(g_log, "ERROR Auto-save failed: Error: disk full"));
    /* The throw was consumed: the engine still runs scripts. */
    expect_js(engine, "1 + 1", "2");
}

/* The callback kept by __t_eh.keep, owned by this test. */
static jsb_callback g_cb;

/* __t_eh.keep(fn): keeps fn in g_cb. */
static void fn_keep(jsb_ctx *ctx)
{
    jsb_callback_new(ctx, jsb_arg_val(ctx, 0), &g_cb);
}

/* Fills the __t_eh module. */
static bool init_module(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn(ctx, module, "keep", fn_keep);
}

/* Defines __t_eh on the engine's runtime. */
static bool setup_module(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__t_eh", init_module);
}

/* A throwing jsb callback lands in this log once installed. */
static void test_attach_jsb(JSEngine *engine)
{
    jsb_runtime *rt = js_engine_get_jsb(engine);
    assert(rt);
    jsb_set_error_handler(rt, error_handler_on_jsb_error, NULL);
    assert(jsb_runtime_setup(rt, setup_module, NULL));
    assert(js_engine_eval(engine,
        "__t_eh.keep(function () { throw new Error('cb boom'); });", "cb.js"));
    reset_log();
    assert(jsb_callback_call(g_cb, 0, NULL, NULL) == JSB_ERR_THREW);
    assert(strstr(g_log, "ERROR JS Exception: Error: cb boom"));
    assert(strstr(g_log, "Stack trace:\nError: cb boom"));
    assert(strstr(g_log, "cb.js:1"));
    assert(jsb_callback_free(&g_cb));
}

int main(void)
{
    error_handler_init(NULL, LOG_DEBUG);
    error_handler_set_callback(capture, NULL);
    test_report_without_engine();
    test_on_jsb_error();

    JSEngine *engine = js_engine_init();
    assert(engine);
    test_dispatch(engine);
    test_dispatch_failures(engine);
    test_auto_save(engine);
    test_attach_jsb(engine);
    js_engine_shutdown(engine);
    error_handler_shutdown();
    printf("error_handler_jsb: all checks passed\n");
    return 0;
}
