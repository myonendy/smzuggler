/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * The game loop's calls into JS, in both engine modes (card M1-T009, spec
 * section 4.1; docs/js-binding.md, "Game loop").
 *
 * Why: game_loop.c drives dom_shim.js's timers and rAF and must keep its
 * phase order, job drains, error reporting, window-close, exit-probe and
 * control-mode behaviour while it moves off QuickJS. This links the real
 * game_loop.c against js_engine.c (quickjs mode) or js_engine_v8.cc (v8
 * mode) and stubs only the platform, renderer, audio, video and log
 * functions it calls, so no window or GPU is needed. Checks use assert()
 * and the fragment compiles with NDEBUG undefined.
 */

#include "engine/game_loop.h"
#include "engine/error_handler.h"
#include "audio/audio_engine.h"
#include "rendering/renderer.h"
#include "video/video_player.h"
#include "test_paths.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define test_pipe(fds) _pipe((fds), 4096, _O_BINARY)
#define test_dup2 _dup2
#define test_write _write
#define test_close _close
#else
#include <unistd.h>
#define test_pipe(fds) pipe(fds)
#define test_dup2 dup2
#define test_write write
#define test_close close
#endif

/* ---- Stubs for what game_loop.c calls outside JS ------------------------ */

static bool g_poll_ok = true;   /* false: SDL reported a quit request */
static bool g_resize;           /* true: report one 800x600 resize */
static char g_log[16384];       /* every error_handler_log line */

/* Stub: SDL event polling; false means quit or window close. */
bool platform_poll_events(Platform *p) { (void)p; return g_poll_ok; }

/* Stub: reports one pending resize to 800x600 when g_resize is set. */
bool platform_check_resize(Platform *p, int *w, int *h)
{
    (void)p;
    if (!g_resize) return false;
    g_resize = false;
    *w = 800;
    *h = 600;
    return true;
}

/* Stub: a 1x1 window. */
void platform_get_window_size(Platform *p, int *w, int *h) { (void)p; *w = 1; *h = 1; }
/* Stub: no window to swap. */
void platform_swap_buffers(Platform *p) { (void)p; }
/* Stub: no renderer. */
void renderer_present(Renderer *r) { (void)r; }
/* Stub: no renderer. */
void renderer_set_output_size(Renderer *r, int w, int h) { (void)r; (void)w; (void)h; }
/* Stub: no read-back; screenshots fail. */
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

/* Stub: appends each formatted log line to g_log. */
void error_handler_log(LogLevel level, const char *fmt, ...)
{
    (void)level;
    size_t used = strlen(g_log);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_log + used, sizeof g_log - used, fmt, ap);
    va_end(ap);
    used = strlen(g_log);
    snprintf(g_log + used, sizeof g_log - used, "\n");
}

/* ---- Helpers ------------------------------------------------------------ */

/* Prints what the next assert checks (on stderr: the control test takes
   over stdout). */
static void note(const char *what)
{
    fprintf(stderr, "  check: %s\n", what);
}

/* dom_shim.js stand-ins that record each phase call in `log`. */
static const char *const kPhasesJs =
    "var log = [], stamps = [], dispatched = [];\n"
    "function __dom_setWindowSize(w, h) { log.push('resize:' + w + 'x' + h); }\n"
    "function __dom_flushInputEvents() { log.push('input:' + arguments.length); }\n"
    "function __dom_updateGamepads() { log.push('pads'); }\n"
    "function __dom_flushTimers() {\n"
    "  log.push('timers');\n"
    "  Promise.resolve().then(function () { log.push('job'); });\n"
    "}\n"
    "function __dom_flushAnimationFrames(t) {\n"
    "  stamps.push(t);\n"
    "  log.push('raf:' + typeof t);\n"
    "  Promise.resolve().then(function () { log.push('rafjob'); });\n"
    "}\n"
    "function __error_handler_dispatch(m, e) {\n"
    "  dispatched.push(m + '|' + (e instanceof Error));\n"
    "}\n";

static Platform *const kPlatform = (Platform *)&g_poll_ok; /* never dereferenced */

/* A fresh engine with the phase stand-ins and a loop on it. */
static GameLoop *make_loop(JSEngine **engine)
{
    g_poll_ok = true;
    g_resize = false;
    g_log[0] = '\0';
    *engine = js_engine_init();
    assert(*engine);
    assert(js_engine_eval(*engine, kPhasesJs, "phases.js"));
    GameLoop *loop = game_loop_create(kPlatform, *engine, 1000);
    assert(loop);
    return loop;
}

/* True when `expr` evaluates to the string `expected`. */
static bool js_is(JSEngine *engine, const char *expr, const char *expected)
{
    char *r = js_engine_eval_string(engine, expr, "check.js");
    bool same = r && strcmp(r, expected) == 0;
    if (!same) fprintf(stderr, "    %s -> %s (want %s)\n", expr, r ? r : "NULL", expected);
    js_engine_free_string(r);
    return same;
}

/* Frees the loop, then the engine. */
static void finish(GameLoop *loop, JSEngine *engine)
{
    game_loop_destroy(loop);
    js_engine_shutdown(engine);
}

/* ---- Tests -------------------------------------------------------------- */

/* Phases run in order with their arguments, jobs drain after timers and
   after rAF, and the rAF timestamp is a growing Number of ms. */
static void test_phase_order(void)
{
    JSEngine *engine;
    GameLoop *loop = make_loop(&engine);
    note("frame 1 runs resize, input, pads, timers, job, raf, rafjob");
    g_resize = true;
    assert(game_loop_step(loop));
    assert(js_is(engine, "log.join(',')",
                 "resize:800x600,input:0,pads,timers,job,raf:number,rafjob"));
    note("frame 2 has no resize and a later timestamp");
    assert(js_is(engine, "log.length = 0, ''", ""));
    assert(game_loop_step(loop));
    assert(js_is(engine, "log.join(',')", "input:0,pads,timers,job,raf:number,rafjob"));
    assert(js_is(engine, "String(stamps[0] >= 0 && stamps[1] > stamps[0])", "true"));
    note("phase functions are the ones present on the first frame");
    assert(js_is(engine, "__dom_flushTimers = function () { log.push('new'); },"
                         " log.length = 0, ''", ""));
    assert(game_loop_step(loop));
    assert(js_is(engine, "log.indexOf('timers') >= 0 && log.indexOf('new') < 0 ? 'old' : 'new'",
                 "old"));
    finish(loop, engine);
}

/* A throwing phase is logged with its stack and sent to window.onerror,
   and the later phases of the frame still run. */
static void test_phase_exception(void)
{
    JSEngine *engine;
    GameLoop *loop = make_loop(&engine);
    assert(js_is(engine, "__dom_flushTimers = function () { throw new Error('boom'); }, ''", ""));
    note("timers throw: logged, dispatched, frame continues");
    assert(game_loop_step(loop));
    assert(strstr(g_log, "JS Exception: Error: boom\n"));
    assert(strstr(g_log, "Stack trace:\n"));
    assert(js_is(engine, "dispatched.join(',')", "Error: boom|true"));
    assert(js_is(engine, "log.join(',')", "input:0,pads,raf:number,rafjob"));
    finish(loop, engine);
    note("a thrown non-Error is logged without a stack");
    loop = make_loop(&engine);
    assert(js_is(engine, "__dom_updateGamepads = function () { throw 'plain'; }, ''", ""));
    assert(game_loop_step(loop));
    assert(strstr(g_log, "JS Exception: plain\n"));
    assert(!strstr(g_log, "Stack trace:"));
    assert(js_is(engine, "dispatched.join(',')", "plain|false"));
    finish(loop, engine);
}

/* A quit request asks __nw_hostClose; only a falsy answer keeps running. */
static void test_host_close(void)
{
    JSEngine *engine;
    GameLoop *loop = make_loop(&engine);
    g_poll_ok = false;
    note("no __nw_hostClose: quit stops the loop");
    assert(!game_loop_step(loop));
    game_loop_destroy(loop);

    note("listener returns false: the loop keeps running");
    loop = game_loop_create(kPlatform, engine, 1000);
    assert(js_is(engine, "var answer = false, closes = 0;"
                         " function __nw_hostClose() { closes++; return answer; } ''", ""));
    assert(game_loop_step(loop));
    assert(strstr(g_log, "Window close deferred to a 'close' listener"));
    assert(js_is(engine, "String(closes)", "1"));
    note("listener returns true: the loop stops");
    assert(js_is(engine, "answer = true, ''", ""));
    assert(!game_loop_step(loop));
    game_loop_destroy(loop);

    note("a throwing listener cannot keep the window open");
    loop = game_loop_create(kPlatform, engine, 1000);
    assert(js_is(engine, "__nw_hostClose = function () { throw new Error('nope'); }, ''", ""));
    assert(!game_loop_step(loop));
    assert(strstr(g_log, "JS Exception: Error: nope"));
    finish(loop, engine);
}

/* Runs one frame with a frame limit of 1 and `probe`; returns g_log. */
static const char *probe_log(const char *probe)
{
    JSEngine *engine;
    GameLoop *loop = make_loop(&engine);
    game_loop_set_frame_limit(loop, 1, NULL, NULL);
    game_loop_set_exit_probe(loop, probe);
    assert(game_loop_step(loop));
    assert(!game_loop_step(loop));
    finish(loop, engine);
    return g_log;
}

/* The exit probe logs its value or its error; sources reach JS intact. */
static void test_exit_probe(void)
{
    note("default probe names the scene");
    assert(strstr(probe_log(NULL), "stopping; scene: none\n"));
    note("probe value");
    assert(strstr(probe_log("'p' + (1 + 1)"), "stopping; probe: p2\n"));
    note("quotes, backslashes and newlines survive");
    assert(strstr(probe_log("'a\"b\\\\c' +\n'\\t'.length"), "stopping; probe: a\"b\\c1\n"));
    note("a throwing probe");
    assert(strstr(probe_log("null.x"), "stopping; probe: probe threw: TypeError"));
}

/* Writes all of `text` to `fd`. */
static void write_all(int fd, const char *text)
{
    size_t n = strlen(text);
    assert(test_write(fd, text, (unsigned)n) == (int)n);
}

/* Control mode: synchronous and promise replies, unknown commands, and a
   stop when stdin closes. Takes over stdin and stdout, so it runs last. */
static void test_control(void)
{
    JSEngine *engine;
    GameLoop *loop = make_loop(&engine);
    assert(js_engine_eval_file(engine, "src/shims/control_shim.js"));
    char out_path[TEST_PATH_MAX];
    snprintf(out_path, sizeof out_path, "%s", test_tmp_path("game_loop_js_stdout.txt"));
    int fds[2];
    assert(test_pipe(fds) == 0);
    assert(test_dup2(fds[0], 0) == 0);
    clearerr(stdin);
    assert(freopen(out_path, "w", stdout));
    write_all(fds[1], "1 eval \"40 + 2\"\n2 eval \"Promise.resolve('p')\"\n");
    assert(game_loop_enable_control(loop, NULL));
    note("frame 1 answers both evals");
    assert(game_loop_step(loop));
    write_all(fds[1], "3 bogus\n4 eval \"throw new Error('x')\"\n");
    test_close(fds[1]);
    int frames = 1;
    while (game_loop_step(loop)) assert(++frames < 1000);
    note("stdin closed: the loop stopped");
    assert(strstr(g_log, "Control mode: stdin closed, stopping"));
    finish(loop, engine);
    fflush(stdout);

    FILE *f = fopen(out_path, "r");
    assert(f);
    char text[4096];
    size_t n = fread(text, 1, sizeof text - 1, f);
    text[n] = '\0';
    fclose(f);
    remove(out_path);
    fprintf(stderr, "    stdout:\n%s", text);
    note("replies in order");
    const char *a = strstr(text, "@@ctl 1 ok 42\n");
    const char *b = strstr(text, "@@ctl 2 ok \"p\"\n");
    const char *c = strstr(text, "@@ctl 3 err \"unknown command\"\n");
    const char *d = strstr(text, "@@ctl 4 err ");
    assert(a && b && c && d && a < b && b < c && c < d);
}

int main(void)
{
    test_phase_order();
    test_phase_exception();
    test_host_close();
    test_exit_probe();
    test_control();
    fprintf(stderr, "game_loop_js: all checks passed\n");
    return 0;
}
