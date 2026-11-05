/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: quickjs-mode get_jsb is now the adopted runtime
 * (card M1-T007b), so its assertion expects non-NULL.
 *
 * Stack-trace locations and the jsb accessor of js_engine.h, in both engine
 * modes (card M1-T007, spec section 4.1).
 *
 * Why: plugin authors debug from stack traces, and NW.js reports a throw in
 * foo.js line 3 as `foo.js:3:<col>`. The V8 engine names each script through
 * a ScriptOrigin; this proves the location survives in both modes and that
 * v8 mode names scripts relative to the game root as NW.js does. It also
 * checks that js_engine_get_jsb() hands out the runtime the game runs in.
 * Built against js_engine_v8.cc in v8 mode and js_engine.c in quickjs mode
 * (tests/smz/js_engine_v8.cmake); SMZ_JS_ENGINE_V8 tells which. Checks use
 * assert() and the fragment compiles with NDEBUG undefined, as the other
 * tests/smz tests do.
 */

#include "engine/js_engine.h"
#include "test_paths.h"

#ifdef SMZ_JS_ENGINE_V8
#include "jsb.h"
#endif

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Prints what the next assert checks, so a failing run names it even
   when the abort message is cut off. */
static void note(const char *what)
{
    printf("  check: %s\n", what);
    fflush(stdout);
}

/* Everything the console callback received, error lines joined by '\n'. */
static char captured[8192];

/* Console callback: appends error-level output to `captured`. */
static void capture(const char *level, const char *message, void *user)
{
    (void)user;
    if (strcmp(level, "error") != 0) return;
    size_t used = strlen(captured);
    snprintf(captured + used, sizeof captured - used, "%s\n", message);
}

/* Writes `text` to `path`; returns 0 on failure. */
static int write_text(const char *path, const char *text)
{
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fputs(text, f);
    return fclose(f) == 0;
}

/* Line 3 throws from inside a function, so the trace has a real frame. */
static const char *const kFooJs =
    "function boom() {\n"
    "  var unused = 1;\n"
    "  throw new Error('boom');\n"
    "}\n"
    "boom();\n";

/* A throw in foo.js line 3, run from a file and from a string named
   foo.js, reports foo.js:3 in what the engine prints. */
static void test_stack_names_file_and_line(void)
{
    char dir[TEST_PATH_MAX], path[TEST_PATH_MAX];
    snprintf(dir, sizeof dir, "%s", test_tmp_path("smz_js_engine_v8"));
    snprintf(path, sizeof path, "%s/foo.js", dir);
    test_rm_rf(dir);
    note("write foo.js");
    assert(test_mkdir_p(dir) && write_text(path, kFooJs));

    JSEngine *engine = js_engine_init();
    note("engine starts");
    assert(engine != NULL);
    js_engine_set_console_callback(engine, capture, NULL);

    captured[0] = '\0';
    bool ok = js_engine_eval_file(engine, path);
    note("eval_file reports the throw with foo.js:3");
    assert(!ok);
    assert(strstr(captured, "foo.js:3") != NULL);

    captured[0] = '\0';
    ok = js_engine_eval(engine, kFooJs, "foo.js");
    note("eval of a string named foo.js reports foo.js:3");
    assert(!ok);
    assert(strstr(captured, "foo.js:3") != NULL);

    captured[0] = '\0';
    ok = js_engine_eval(engine, "\n\nfunction {", "foo.js");
    note("syntax error on line 3 reports foo.js:3");
    assert(!ok);
    assert(strstr(captured, "foo.js:3") != NULL);

    js_engine_shutdown(engine);
    test_rm_rf(dir);
}

#ifdef SMZ_JS_ENGINE_V8
/* With a game root known, v8 mode names scripts by their game-relative
   path, as NW.js does, so the root never shows in a trace. */
static void test_stack_is_game_relative(void)
{
    char game[TEST_PATH_MAX], plugins[TEST_PATH_MAX], path[TEST_PATH_MAX];
    char cache[TEST_PATH_MAX];
    snprintf(game, sizeof game, "%s", test_tmp_path("smz_js_engine_v8_game"));
    snprintf(cache, sizeof cache, "%s", test_tmp_path("smz_js_engine_v8_cache"));
    snprintf(plugins, sizeof plugins, "%s/js/plugins", game);
    snprintf(path, sizeof path, "%s/foo.js", plugins);
    test_rm_rf(game);
    note("write game foo.js");
    assert(test_mkdir_p(plugins) && write_text(path, kFooJs));

    JSEngine *engine = js_engine_init();
    note("engine starts");
    assert(engine != NULL);
    js_engine_set_console_callback(engine, capture, NULL);
    bool ok = js_engine_set_cache_dir(engine, cache, game);
    note("set game root");
    assert(ok);

    captured[0] = '\0';
    ok = js_engine_eval_file(engine, path);
    note("frame names js/plugins/foo.js:3 without the game root");
    assert(!ok);
    assert(strstr(captured, "at boom (js/plugins/foo.js:3:") != NULL);
    assert(strstr(captured, game) == NULL);

    js_engine_shutdown(engine);
    test_rm_rf(game);
    test_rm_rf(cache);
}

/* js_engine_get_jsb returns the runtime the engine's scripts run in. */
static void test_get_jsb_shares_globals(void)
{
    JSEngine *engine = js_engine_init();
    note("engine starts");
    assert(engine != NULL);
    jsb_runtime *rt = js_engine_get_jsb(engine);
    note("get_jsb is not NULL");
    assert(rt != NULL);

    bool ok = js_engine_eval(engine, "var sharedValue = 6 * 7;", "set.js");
    note("set a global through js_engine_eval");
    assert(ok);
    char out[32] = "";
    ok = jsb_runtime_eval(rt, "sharedValue", "get.js", out, sizeof out);
    note("jsb sees the engine's global");
    assert(ok);
    assert(strcmp(out, "42") == 0);

    note("get_jsb(NULL) is NULL");
    assert(js_engine_get_jsb(NULL) == NULL);
    js_engine_shutdown(engine);
}
#else
/* quickjs mode adopts the engine's context into a jsb runtime (js_engine.h). */
static void test_get_jsb_quickjs(void)
{
    JSEngine *engine = js_engine_init();
    note("engine starts");
    assert(engine != NULL);
    note("quickjs get_jsb is not NULL and get_context remains");
    assert(js_engine_get_jsb(engine) != NULL);
    assert(js_engine_get_context(engine) != NULL);
    js_engine_shutdown(engine);
}
#endif

int main(void)
{
    printf("=== js_engine stack and jsb tests ===\n");
    test_stack_names_file_and_line();
#ifdef SMZ_JS_ENGINE_V8
    test_stack_is_game_relative();
    test_get_jsb_shares_globals();
#else
    test_get_jsb_quickjs();
#endif
    printf("OK\n");
    return 0;
}
