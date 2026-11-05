/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Checks the jsb port of bind_platform.c (card M1-T014) on the engine's own
 * jsb runtime: js_engine.c in the default engine mode, smz_js_engine_v8 in v8 mode.
 *
 * Why: --dump-surface lists no native modules and no function `length`
 * (DEC-M0-T009b), so this test is what shows __native_platform and
 * __native_process kept the pre-port JS API: property names and order,
 * typeof and length, no-device results, and wrong-argument errors. The
 * expected values are those of the pre-port QuickJS binding. The Platform
 * functions are replaced by recording doubles below, so no window or GPU is
 * needed. Main thread only.
 */

#include "bindings/bind_platform.h"
#include "engine/js_engine.h"
#include "platform/platform.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* ---- Platform doubles: record what the binding passed down ------------ */

static bool s_fullscreen;
static int s_fullscreen_calls;
static char s_title[512];

/* Records the requested fullscreen state. */
void platform_set_fullscreen(Platform *p, bool fullscreen)
{
    (void)p;
    s_fullscreen = fullscreen;
    s_fullscreen_calls++;
}

/* Reports the last recorded fullscreen state. */
bool platform_is_fullscreen(Platform *p)
{
    (void)p;
    return s_fullscreen;
}

/* Reports a fixed 2560x1440 display. */
void platform_get_display_size(Platform *p, int *w, int *h)
{
    (void)p;
    *w = 2560;
    *h = 1440;
}

/* Reports a fixed 1.5 scale. */
float platform_get_display_scale(Platform *p)
{
    (void)p;
    return 1.5f;
}

/* Reports a fixed 816x624 window. */
void platform_get_window_size(Platform *p, int *w, int *h)
{
    (void)p;
    *w = 816;
    *h = 624;
}

/* Records the title the binding built. */
void platform_set_window_title(Platform *p, const char *title)
{
    (void)p;
    snprintf(s_title, sizeof(s_title), "%s", title);
}

/* ---- Helpers ----------------------------------------------------------- */

static JSEngine *s_js;

/* Evaluates `src` and asserts that String(result) equals `want`. */
static void expect(const char *src, const char *want)
{
    char *got = js_engine_eval_string(s_js, src, "bind_platform_jsb.js");
    if (!got || strcmp(got, want) != 0) {
        fprintf(stderr, "FAIL: %s\n  want: %s\n  got:  %s\n", src, want,
                got ? got : "(exception)");
    }
    assert(got && strcmp(got, want) == 0);
    js_engine_free_string(got);
}

/* Evaluates `call` inside try/catch and asserts the thrown error's
 * `name: message` (an empty message checks only the name). */
static void expect_throw(const char *call, const char *name, const char *message)
{
    char src[512], want[256];
    snprintf(src, sizeof(src),
             "(function(){try{%s;return 'no throw';}catch(e){"
             "return e.name + (%d ? ': ' + e.message : '');}})()",
             call, message[0] != '\0');
    snprintf(want, sizeof(want), message[0] ? "%s: %s" : "%s%s", name, message);
    expect(src, want);
}

/* ---- Checks ------------------------------------------------------------ */

/* Names, order, typeof and length, as JS_CFUNC_DEF defined them. */
static void check_shape(void)
{
    expect("Object.getOwnPropertyNames(__native_platform).join()",
           "setFullscreen,isFullscreen,getDisplaySize,getDisplayScale,"
           "getWindowSize,setWindowTitle,quit");
    expect("Object.getOwnPropertyNames(__native_platform).map(function(k){"
           "return typeof __native_platform[k] + '/' + __native_platform[k].length;"
           "}).join()",
           "function/1,function/0,function/0,function/0,function/0,function/1,"
           "function/0");
    expect("typeof __native_platform", "object");

    expect("Object.getOwnPropertyNames(__native_process).join()",
           "platform,arch,engine,environ,hrtime");
    expect("[typeof __native_process.platform, typeof __native_process.arch,"
           " typeof __native_process.environ, __native_process.environ.length,"
           " typeof __native_process.hrtime, __native_process.hrtime.length].join()",
           "string,string,function,0,function,0");
    expect("__native_process.platform", bind_platform_os_name());
    expect("__native_process.arch", bind_platform_arch_name());
    expect("__native_process.engine", jsb_engine_version());
}

/* Results with no Platform (no window): the pre-port defaults. */
static void check_no_device(void)
{
    expect("String(__native_platform.setFullscreen())", "undefined");
    expect("String(__native_platform.setFullscreen(true))", "undefined");
    expect("__native_platform.isFullscreen() === false", "true");
    expect("JSON.stringify(__native_platform.getDisplaySize())",
           "{\"width\":1920,\"height\":1080}");
    expect("__native_platform.getDisplayScale() === 1", "true");
    expect("JSON.stringify(__native_platform.getWindowSize())",
           "{\"width\":0,\"height\":0}");
    expect("String(__native_platform.setWindowTitle('x'))", "undefined");
    expect("String(__native_platform.quit())", "undefined");
    assert(s_fullscreen_calls == 0 && s_title[0] == '\0');

    expect("var t = __native_process.hrtime(); Array.isArray(t) && t.length === 2"
           " && typeof t[0] === 'number' && t[1] >= 0 && t[1] < 1e9"
           " && (t[1] | 0) === t[1]", "true");
    expect("var e = __native_process.environ(); Object.getPrototypeOf(e) ==="
           " Object.prototype && e.BIND_PLATFORM_JSB_PROBE", "probe=value");
}

/* With a Platform: values pass through, coercion and errors as before. */
static void check_with_device(void)
{
    expect_throw("__native_platform.setFullscreen()", "TypeError",
                 "setFullscreen requires a boolean");
    assert(s_fullscreen_calls == 0);
    expect("__native_platform.setFullscreen(1); __native_platform.isFullscreen()",
           "true");
    expect("__native_platform.setFullscreen(''); __native_platform.isFullscreen()",
           "false");
    assert(s_fullscreen_calls == 2);

    expect("JSON.stringify(__native_platform.getDisplaySize())",
           "{\"width\":2560,\"height\":1440}");
    expect("__native_platform.getDisplayScale()", "1.5");
    expect("JSON.stringify(__native_platform.getWindowSize())",
           "{\"width\":816,\"height\":624}");

    char want[256];
    expect("String(__native_platform.setWindowTitle({toString:function(){"
           "return 'Menu';}}))", "undefined");
    snprintf(want, sizeof(want), "Menu [%s]", RMMZ_APP_NAME);
    assert(strcmp(s_title, want) == 0);
    expect("String(__native_platform.setWindowTitle(''))", "undefined");
    assert(strcmp(s_title, RMMZ_APP_NAME) == 0);
    s_title[0] = '\0';
    expect("String(__native_platform.setWindowTitle())", "undefined");
    assert(s_title[0] == '\0');
    expect_throw("__native_platform.setWindowTitle(Symbol('s'))", "TypeError", "");
    expect_throw("__native_platform.setWindowTitle({toString:function(){"
                 "throw new RangeError('boom');}})", "RangeError", "boom");
    assert(s_title[0] == '\0');
}

int main(void)
{
#ifdef _WIN32
    _putenv("BIND_PLATFORM_JSB_PROBE=probe=value");
#else
    setenv("BIND_PLATFORM_JSB_PROBE", "probe=value", 1);
#endif
    s_js = js_engine_init();
    assert(s_js);
    jsb_runtime *rt = js_engine_get_jsb(s_js);
    assert(rt);

    assert(bind_platform_define(rt, NULL));
    check_shape();
    check_no_device();

    /* A dummy, never dereferenced: the doubles above ignore it. */
    static char fake_platform;
    assert(bind_platform_define(rt, (Platform *)&fake_platform));
    check_shape();
    check_with_device();

    js_engine_shutdown(s_js);
    printf("bind_platform_jsb: all checks passed\n");
    return 0;
}
