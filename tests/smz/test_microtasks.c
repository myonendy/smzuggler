/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * The order of macrotasks and microtasks the game loop produces (card
 * M1-T010, spec section 4.1; docs/js-binding.md, "Microtask checkpoints";
 * docs/decisions/DEC-M1-T010.md).
 *
 * Why: RPG Maker MZ and its plugins were written against NW.js, where
 * Chrome runs a microtask checkpoint after every task (each timer, each
 * rAF callback, each event dispatch) and Node runs process.nextTick jobs
 * before promise jobs. Plugins that chain promises off timers notice when
 * that order changes. This runs the real game_loop.c with the real
 * dom_shim.js and storage_shim.js on js_engine_v8.cc (v8 mode) or
 * js_engine.c (quickjs mode), stubbing only the platform, renderer, audio,
 * video and log calls, and checks the logged order. Checks use assert()
 * and the fragment compiles with NDEBUG undefined. Main thread only.
 */

#include "engine/game_loop.h"
#include "engine/error_handler.h"
#include "audio/audio_engine.h"
#include "rendering/renderer.h"
#include "video/video_player.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ---- Stubs for what game_loop.c calls outside JS ------------------------ */

static int g_platform;          /* stands in for the Platform; never read */

/* Stub: SDL event polling; never asks to quit. */
bool platform_poll_events(Platform *p) { (void)p; return true; }
/* Stub: the window never resizes. */
bool platform_check_resize(Platform *p, int *w, int *h) { (void)p; (void)w; (void)h; return false; }
/* Stub: a 1x1 window. */
void platform_get_window_size(Platform *p, int *w, int *h) { (void)p; *w = 1; *h = 1; }
/* Stub: no window to swap. */
void platform_swap_buffers(Platform *p) { (void)p; }
/* Stub: no renderer. */
void renderer_present(Renderer *r) { (void)r; }
/* Stub: no renderer. */
void renderer_set_output_size(Renderer *r, int w, int h) { (void)r; (void)w; (void)h; }
/* Stub: no read-back. */
uint8_t *renderer_read_pixels(Renderer *r, uint32_t fbo, int w, int h)
{
    (void)r; (void)fbo; (void)w; (void)h;
    return NULL;
}
/* Stub: no video. */
void video_player_update(double dt) { (void)dt; }
/* Stub: no video. */
void video_player_draw(void) {}
/* Stub: no audio. */
void audio_update(void) {}

/* Stub: prints each log line, so a JS error in the scenario is visible. */
void error_handler_log(LogLevel level, const char *fmt, ...)
{
    (void)level;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

/* ---- Helpers ------------------------------------------------------------ */

/* Supply native events to the real input dispatcher; no DOM phase is replaced. */
static const char *const kInputJs =
    "var log = [], events = [];\n"
    "var __native_input = {pollEvents: function () { var e = events; events = []; return e; },\n"
    "getGamepadCount: function () { return 0; }};\n";

/* One frame's worth of tasks, each logging itself and the microtasks it
   queues: two timeouts, an interval, two rAF callbacks and one input
   dispatch. Names: t = timer, p = promise job, n = nextTick, i = input,
   r = rAF, iv = interval. */
static const char *const kScenarioJs =
    "events = [{type: 'keydown', key: 'a'}, {type: 'keydown', key: 'b'}];\n"
    "document.addEventListener('keydown', function (e) {\n"
    "  log.push(e.key);\n"
    "  Promise.resolve().then(function () { log.push(e.key + 'p'); });\n"
    "});\n"
    "setTimeout(function () {\n"
    "  log.push('t1');\n"
    "  Promise.resolve().then(function () { log.push('p1'); })\n"
    "    .then(function () { log.push('p1b'); });\n"
    "  process.nextTick(function () {\n"
    "    log.push('n1');\n"
    "    process.nextTick(function () { log.push('n1b'); });\n"
    "  });\n"
    "}, 0);\n"
    "setTimeout(function () {\n"
    "  log.push('t2');\n"
    "  Promise.resolve().then(function () { log.push('p2'); });\n"
    "}, 0);\n"
    "var iv = setInterval(function () {\n"
    "  clearInterval(iv);\n"
    "  log.push('iv');\n"
    "  Promise.resolve().then(function () { log.push('ivp'); });\n"
    "}, 0);\n"
    "requestAnimationFrame(function () {\n"
    "  log.push('r1');\n"
    "  Promise.resolve().then(function () { log.push('rp1'); });\n"
    "  process.nextTick(function () { log.push('rn1'); });\n"
    "});\n"
    "requestAnimationFrame(function () { log.push('r2'); });\n";

/* The order NW.js (Chrome's event loop with Node's tick queue) produces:
   a checkpoint after every task, ticks first. */
static const char kChromeOrder[] =
    "a,ap,b,bp,t1,n1,n1b,p1,p1b,t2,p2,iv,ivp,r1,rn1,rp1,r2";

/* The order quickjs mode produces: js_engine.c has no checkpoint callable
   from JS, so the timers and the rAF callbacks of a frame each share one
   checkpoint after their phase (DEC-M1-T010 lists the differences). Ticks
   still run before promise jobs, and each phase's jobs before the next. */
static const char kQuickJsOrder[] =
    "a,b,ap,bp,t1,t2,iv,n1,n1b,p1,p2,ivp,p1b,r1,r2,rn1,rp1";

/* True when `expr` evaluates to the string `expected`. */
static bool js_is(JSEngine *engine, const char *expr, const char *expected)
{
    char *r = js_engine_eval_string(engine, expr, "check.js");
    bool same = r && strcmp(r, expected) == 0;
    if (!same) fprintf(stderr, "    %s -> %s (want %s)\n", expr, r ? r : "NULL", expected);
    js_engine_free_string(r);
    return same;
}

/* A fresh engine with dom_shim.js, storage_shim.js and native input stubs,
   and a loop that has not run a frame. Startup registration must work.
   The caller frees both. */
static GameLoop *make_loop(JSEngine **engine)
{
    *engine = js_engine_init();
    assert(*engine);
    assert(js_engine_eval_file(*engine, "src/shims/dom_shim.js"));
    assert(js_engine_eval_file(*engine, "src/shims/storage_shim.js"));
    assert(js_engine_execute_pending_jobs(*engine) >= 0);
    assert(js_engine_eval(*engine, kInputJs, "input.js"));
    GameLoop *loop = game_loop_create((Platform *)&g_platform, *engine, 1000);
    assert(loop);
    return loop;
}

/* ---- Tests -------------------------------------------------------------- */

/* One frame of timers, rAF and an input dispatch logs the NW.js order in
   v8 mode, and the phase-level order of DEC-M1-T010 in quickjs mode. */
static void test_frame_order(void)
{
    JSEngine *engine;
    GameLoop *loop = make_loop(&engine);
    assert(js_engine_eval(engine, kScenarioJs, "scenario.js"));
    assert(js_is(engine, "log.join(',')", ""));
    assert(game_loop_step(loop));
#ifdef SMZ_JS_ENGINE_V8
    fprintf(stderr, "  check: v8 mode follows Chrome/NW.js order\n");
    assert(js_is(engine, "log.join(',')", kChromeOrder));
#else
    fprintf(stderr, "  check: quickjs mode follows its phase-level order\n");
    assert(js_is(engine, "log.join(',')", kQuickJsOrder));
#endif
    fprintf(stderr, "  check: nothing is left for the next frame\n");
    assert(game_loop_step(loop));
    assert(js_is(engine, "String(log.length)", "17"));
    game_loop_destroy(loop);
    js_engine_shutdown(engine);
}

/* A tick queued outside any task (as during script load) runs at the next
   checkpoint, before promise jobs queued before it and with its arguments;
   a non-function is a TypeError, as in Node. */
static void test_tick_outside_task(void)
{
    JSEngine *engine;
    GameLoop *loop = make_loop(&engine);
    assert(js_engine_eval(engine,
        "Promise.resolve().then(function () { log.push('p'); });\n"
        "process.nextTick(function (a, b) { log.push('n' + a + b); }, 1, 2);\n"
        "Promise.resolve().then(function () { process.nextTick(function () { log.push('pn'); }); });\n",
        "load.js"));
    assert(js_engine_execute_pending_jobs(engine) >= 0);
#ifdef SMZ_JS_ENGINE_V8
    assert(js_is(engine, "log.join(',')", "n12,p,pn"));
#else
    assert(js_is(engine, "log.join(',')", "p,n12,pn"));
#endif
    assert(js_is(engine,
        "(function () { try { process.nextTick(1); return 'no'; }"
        " catch (e) { return e instanceof TypeError ? 'TypeError' : 'other'; } })()",
        "TypeError"));
    game_loop_destroy(loop);
    js_engine_shutdown(engine);
}

/* Consumed overlay keys finish default actions and jobs before the next key.
   Real overlay listeners bypass document.dispatchEvent; main thread only. */
static void test_overlay_event_order(void)
{
    JSEngine *engine;
    GameLoop *loop = make_loop(&engine);
    assert(js_engine_eval_file(engine, "src/shims/dom_overlay.js"));
    assert(js_engine_eval(engine,
        "var field = document.createElement('input');"
        "document.body.appendChild(field); field.value = 'abc'; field.focus();"
        "field.selectionStart = field.selectionEnd = 3;"
        "field.addEventListener('keydown', function () {"
        " log.push('key:' + field.value);"
        " process.nextTick(function () { log.push('tick:' + field.value); });"
        " Promise.resolve().then(function () { log.push('promise:' + field.value); });"
        "});"
        "field.addEventListener('input', function () { log.push('input:' + field.value); });"
        "events = [{type:'keydown', key:'Backspace'}, {type:'keydown', key:'Backspace'}];",
        "overlay-order.js"));
    assert(game_loop_step(loop));
#ifdef SMZ_JS_ENGINE_V8
    assert(js_is(engine, "log.join(',')",
        "key:abc,input:ab,tick:ab,promise:ab,key:ab,input:a,tick:a,promise:a"));
#else
    assert(js_is(engine, "log.join(',')",
        "key:abc,input:ab,key:ab,input:a,tick:a,tick:a,promise:a,promise:a"));
#endif
    game_loop_destroy(loop);
    js_engine_shutdown(engine);
}

/* Native video endings drain jobs after all synchronous listeners, before the
   next video or rAF callback. Fake player state feeds the real DOM dispatcher. */
static void test_video_event_order(void)
{
    JSEngine *engine;
    GameLoop *loop = make_loop(&engine);
    assert(js_engine_eval(engine,
        "var handle = 0, finished = false;"
        "var __native_video = {"
        " open: function () { return ++handle; },"
        " getState: function () { return {time:1, duration:1, width:1, height:1, ended:finished}; },"
        " setVisible: function () {}, setVolume: function () {}, setLoop: function () {},"
        " play: function () {} };"
        "function videoTask(id) {"
        " var v = document.createElement('video'); v.src = 'fixture.mp4'; v.play();"
        " v.addEventListener('ended', function () {"
        "  log.push(id);"
        "  Promise.resolve().then(function () { log.push(id + 'p'); });"
        "  process.nextTick(function () { log.push(id + 'n'); });"
        " });"
        " v.addEventListener('ended', function () { log.push(id + 's'); });"
        "}"
        "videoTask('v1'); videoTask('v2'); finished = true;"
        "requestAnimationFrame(function () { log.push('raf'); });",
        "video-order.js"));
    assert(game_loop_step(loop));
#ifdef SMZ_JS_ENGINE_V8
    assert(js_is(engine, "log.join(',')", "v2,v2s,v2n,v2p,v1,v1s,v1n,v1p,raf"));
#else
    assert(js_is(engine, "log.join(',')", "v2,v2s,v1,v1s,raf,v2n,v1n,v2p,v1p"));
#endif
    assert(game_loop_step(loop));
    assert(js_is(engine, "String(log.length)", "9"));
    game_loop_destroy(loop);
    js_engine_shutdown(engine);
}

/* Run main-thread ordering regressions; assertions terminate on failure. */
int main(void)
{
    fprintf(stderr, "test_frame_order\n");
    test_frame_order();
    test_overlay_event_order();
    test_video_event_order();
    fprintf(stderr, "test_tick_outside_task\n");
    test_tick_outside_task();
    fprintf(stderr, "microtasks: all passed\n");
    return 0;
}
