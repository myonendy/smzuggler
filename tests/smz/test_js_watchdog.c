/* SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 The smzuggler contributors

   Tests the V8 script watchdog (card M1-T008, spec section 4.1 item 3).

   Why: a plugin stuck in a loop freezes the whole game; the watchdog must
   stop it within the limit, say where it was stuck, and leave the engine
   usable. Game-free: every script is a string in this file. See
   docs/js-binding.md, "Watchdog".

   Rules: main thread only; asserts stay live in Release (-UNDEBUG). */

#include "engine/js_engine.h"
#include "engine/js_watchdog.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Last console line at level "error", for checking the watchdog report. */
static char g_error[1024];

/* Console callback: keeps the last error line and echoes everything, so a
   failure shows what the engine said. */
static void capture(const char *level, const char *message, void *userdata)
{
    (void)userdata;
    if (strcmp(level, "error") == 0)
        snprintf(g_error, sizeof g_error, "%s", message);
    fprintf(stderr, "[%s] %s\n", level, message);
}

/* Wall-clock milliseconds (C11 timespec_get; only differences are used). */
static double now_ms(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

/* Returns a fresh engine with the capturing console. */
static JSEngine *new_engine(void)
{
    JSEngine *engine = js_engine_init();
    assert(engine);
    js_engine_set_console_callback(engine, capture, NULL);
    return engine;
}

/* With the limit off (the default, and after setting 0) no watchdog thread
   exists, even while scripts run; with a limit there is exactly one, and
   shutdown joins it. */
static void test_no_thread_when_off(void)
{
    assert(js_watchdog_live_threads() == 0);
    JSEngine *engine = new_engine();
    assert(js_engine_eval(engine, "var x = 1 + 1;", "off.js"));
    assert(js_watchdog_live_threads() == 0);
    js_engine_set_watchdog(engine, 0);
    js_engine_watchdog_arm(engine);
    assert(js_watchdog_live_threads() == 0);
    js_engine_set_watchdog(engine, 200);
    assert(js_watchdog_live_threads() == 1);
    js_engine_set_watchdog(engine, 300);   /* replaced, not added */
    assert(js_watchdog_live_threads() == 1);
    js_engine_set_watchdog(engine, 0);
    assert(js_watchdog_live_threads() == 0);
    js_engine_set_watchdog(engine, 200);
    js_engine_shutdown(engine);
    assert(js_watchdog_live_threads() == 0);
}

/* `while(true){}` under a 200 ms limit stops within 400 ms, the report
   names spin.js line 2, a catch block cannot swallow the termination, and
   the next eval runs normally. */
static void test_terminates_loop(void)
{
    JSEngine *engine = new_engine();
    js_engine_set_watchdog(engine, 200);
    assert(js_engine_eval(engine, "var after = false;", "setup.js"));
    g_error[0] = '\0';
    double start = now_ms();
    bool ok = js_engine_eval(engine,
                             "try {\n  while (true) {}\n} catch (e) {}\nafter = true;\n",
                             "spin.js");
    double took = now_ms() - start;
    fprintf(stderr, "terminated after %.0f ms\n", took);
    assert(!ok);
    assert(took >= 150.0 && took < 400.0);
    assert(strstr(g_error, "watchdog") != NULL);
    assert(strstr(g_error, "spin.js:2:") != NULL);
    char *after = js_engine_eval_string(engine, "String(after)", "check.js");
    assert(after && strcmp(after, "false") == 0);
    js_engine_free_string(after);
    char *sum = js_engine_eval_string(engine, "1 + 1", "next.js");
    assert(sum && strcmp(sum, "2") == 0);
    js_engine_free_string(sum);
    js_engine_shutdown(engine);
}

/* Each eval restarts the clock: several short scripts whose total time is
   over the limit all succeed, and idle time between them does not count. */
static void test_rearm_per_eval(void)
{
    JSEngine *engine = new_engine();
    js_engine_set_watchdog(engine, 200);
    const char *busy = "var t = Date.now(); while (Date.now() - t < 120) {}";
    for (int i = 0; i < 3; i++) assert(js_engine_eval(engine, busy, "busy.js"));
    double idle = now_ms();
    while (now_ms() - idle < 300.0) {}   /* the clock runs out with no JS */
    assert(js_engine_eval(engine, "var y = 2;", "after_idle.js"));
    js_engine_shutdown(engine);
}

/* A microtask that loops is terminated too; the checkpoint returns -1 and
   the engine runs scripts and promise jobs afterwards. */
static void test_terminates_microtask(void)
{
    JSEngine *engine = new_engine();
    js_engine_set_watchdog(engine, 200);
    g_error[0] = '\0';
    assert(js_engine_eval(engine,
                          "Promise.resolve().then(function () {\n"
                          "  for (;;) {}\n"
                          "});\n",
                          "job.js"));
    double start = now_ms();
    assert(js_engine_execute_pending_jobs(engine) == -1);
    assert(now_ms() - start < 400.0);
    assert(strstr(g_error, "job.js:2:") != NULL);
    assert(js_engine_eval(engine, "var done = 0; Promise.resolve().then(function () { done = 1; });",
                          "again.js"));
    assert(js_engine_execute_pending_jobs(engine) == 1);
    char *done = js_engine_eval_string(engine, "String(done)", "check.js");
    assert(done && strcmp(done, "1") == 0);
    js_engine_free_string(done);
    js_engine_shutdown(engine);
}

/* Runs every case; a failed assert aborts with its line. */
int main(void)
{
    test_no_thread_when_off();
    test_terminates_loop();
    test_rearm_per_eval();
    test_terminates_microtask();
    assert(js_watchdog_live_threads() == 0);
    printf("js_watchdog: all tests passed\n");
    return 0;
}
