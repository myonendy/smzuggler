/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-01: call into JS only through js_engine.h, with no QuickJS
 * calls, so the loop runs unchanged on both engines (card M1-T009).
 */

/*
 * The frame loop: SDL events, JS timers, microtasks, requestAnimationFrame,
 * video, audio and present, in that order, once per frame.
 *
 * Why: RPG Maker MZ expects a browser event loop; this stands in for
 * Chromium's (as NW.js runs it), driving the dom_shim.js timer and rAF
 * queues from C. See docs/js-binding.md, "Game loop".
 *
 * Rules a reader must not break:
 * - Main thread only, except control_reader_thread, which touches nothing
 *   but the control queue under control_mutex.
 * - The loop holds no JS handle of any kind (spec section 15.8,
 *   docs/js-binding.md "Handle rules"). Every call into JS goes through
 *   js_engine.h to the helper in kLoopJs, which keeps the JS functions the
 *   loop calls inside JS itself.
 * - Phase order and the two job drains (after timers, after rAF) are the
 *   game's timing contract; do not move them.
 */

#include "engine/game_loop.h"
#include "engine/error_handler.h"
#include "audio/audio_engine.h"
#include "rendering/renderer.h"
#include "video/video_player.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ControlLine {
    char *text;
    struct ControlLine *next;
} ControlLine;

struct GameLoop {
    Platform  *platform;
    JSEngine  *engine;
    int        target_fps;
    double     frame_time_ms;  /* 1000.0 / target_fps */
    bool       running;

    /* Timing state */
    uint64_t   perf_freq;
    uint64_t   start_ticks;
    uint64_t   last_frame_ticks;
    uint64_t   prev_frame_start_ticks;   /* start of the previous frame (video clock) */

    /* FPS tracking */
    double     fps;
    int        frame_count;
    uint64_t   fps_timer_ticks;

    /* Per-frame performance stats */
    GameLoopStats stats;

    /* Debug frame limit (0 = run until quit) and optional final screenshot */
    int        max_frames;
    int        total_frames;
    char      *screenshot_path;
    Renderer **screenshot_renderer_slot;
    char      *probe_expr;

    /* Periodic phase-time report every perf_interval frames (0 = off) */
    int           perf_interval;
    int           perf_count;
    GameLoopStats perf_acc;

    /* Control mode (--control): commands read from stdin by a thread. */
    bool         control_enabled;
    bool         control_started;      /* first command received */
    bool         control_eof;
    SDL_Thread  *control_thread;
    SDL_Mutex   *control_mutex;
    ControlLine *control_head;
    ControlLine *control_tail;
    long         control_shot_id;
    char        *control_shot_path;    /* screenshot requested this frame */

    /* JS side: kLoopJs installed (js_ready) or failed to install (js_failed);
       phase functions snapshotted on the first frame (js_funcs_cached). */
    bool       js_ready;
    bool       js_failed;
    bool       js_cache_tried;
    bool       js_funcs_cached;
};

static void control_shutdown(GameLoop *loop);

/* ---- Calling into JS ------------------------------------------------------
   kLoopJs defines the non-writable global __smz_gameLoop(op, args...), the
   loop's single entry into JS. It returns a string whose first character is
   a code:
     'n'  the function is missing (not callable); nothing ran
     't' / 'f'  it ran and returned a truthy / falsy value
     'x'  it threw: message, then U+0001 and the stack if it had one. The
          helper has already passed it to __error_handler_dispatch
          (window.onerror) as error_handler_report_js_exception does
     'v'  a value follows (probe result, drained control replies)
     'e'  the probe threw: its message follows
   Keeping the functions in JS keeps the loop free of engine handles, so the
   same code runs on QuickJS and on V8 (docs/js-binding.md, "Game loop"). */
static const char kLoopJs[] =
    "(function (g) {\n"
    "  'use strict';\n"
    "  if (typeof g.__smz_gameLoop === 'function') return;\n"
    "  var NAMES = ['__dom_setWindowSize', '__dom_flushInputEvents',\n"
    "               '__dom_updateGamepads', '__dom_flushTimers',\n"
    "               '__dom_flushAnimationFrames'];\n"
    "  var phases = {}, controlEval, outbox = [];\n"
    "  function report(e) {\n"
    "    var msg = 'Unknown error', stack = null;\n"
    "    try { msg = String(e); } catch (ignored) {}\n"
    "    if (e instanceof Error) {\n"
    "      try { if (e.stack !== undefined) stack = String(e.stack); } catch (ignored) {}\n"
    "    }\n"
    "    var dispatch = g.__error_handler_dispatch;\n"
    "    if (typeof dispatch === 'function') {\n"
    "      try { dispatch.call(g, msg, e); } catch (ignored) {}\n"
    "    }\n"
    "    return 'x' + msg + (stack === null ? '' : '\\u0001' + stack);\n"
    "  }\n"
    "  function invoke(fn, self, args) {\n"
    "    if (typeof fn !== 'function') return 'n';\n"
    "    try { return fn.apply(self, args) ? 't' : 'f'; } catch (e) { return report(e); }\n"
    "  }\n"
    "  var ops = {\n"
    "    cache: function () {\n"
    "      phases = {};\n"
    "      for (var i = 0; i < NAMES.length; i++) phases[NAMES[i]] = g[NAMES[i]];\n"
    "      return 't';\n"
    "    },\n"
    "    phase: function (a) { return invoke(phases[a[0]], g, a.slice(1)); },\n"
    "    call: function (a) { return invoke(g[a[0]], g, a.slice(1)); },\n"
    "    controlInit: function () {\n"
    "      g.__native_control_send = function (text) {\n"
    "        if (arguments.length < 1) return;\n"
    "        try { outbox.push(String(text)); } catch (ignored) {}\n"
    "      };\n"
    "      controlEval = g.__control_eval;\n"
    "      return typeof controlEval === 'function' ? 't' : 'f';\n"
    "    },\n"
    "    control: function (a) { return invoke(controlEval, undefined, a); },\n"
    "    drain: function () {\n"
    "      if (outbox.length === 0) return 'e';\n"
    "      var text = outbox.join('\\n');\n"
    "      outbox.length = 0;\n"
    "      return 'v' + text;\n"
    "    },\n"
    "    probe: function (a) {\n"
    "      try { return 'v' + String((0, eval)(a[0])); } catch (e) {\n"
    "        var m = '?';\n"
    "        try { m = String(e); } catch (ignored) {}\n"
    "        return 'e' + m;\n"
    "      }\n"
    "    }\n"
    "  };\n"
    "  Object.defineProperty(g, '__smz_gameLoop', { value: function (op) {\n"
    "    return ops[op](Array.prototype.slice.call(arguments, 1));\n"
    "  } });\n"
    "})(globalThis);\n";

/* One argument of a JS call built in C: a Number, or a String when `str`
   is not NULL (borrowed for the call). */
typedef struct LoopArg {
    double      num;
    const char *str;
} LoopArg;

/* A growable byte buffer for building call sources; `ok` turns false on
   the first allocation failure and stays false. */
typedef struct StrBuf {
    char  *data;
    size_t len, cap;
    bool   ok;
} StrBuf;

/* Appends `n` bytes of `text` to `sb`, growing it; sets sb->ok = false if
   memory runs out. The buffer stays NUL-terminated. */
static void sb_append(StrBuf *sb, const char *text, size_t n)
{
    if (!sb->ok) return;
    if (sb->len + n + 1 > sb->cap) {
        size_t cap = sb->cap ? sb->cap : 256;
        while (sb->len + n + 1 > cap) cap *= 2;
        char *grown = realloc(sb->data, cap);
        if (!grown) { sb->ok = false; return; }
        sb->data = grown;
        sb->cap = cap;
    }
    memcpy(sb->data + sb->len, text, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

/* Appends `text` (UTF-8, borrowed) as a double-quoted JS string literal.
   Why: control payloads and probe sources are arbitrary text and must
   reach JS byte for byte, so quotes, backslashes and control characters
   are escaped; other bytes pass through as UTF-8. */
static void sb_js_string(StrBuf *sb, const char *text)
{
    sb_append(sb, "\"", 1);
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        char esc[8];
        if (*p == '"' || *p == '\\') {
            esc[0] = '\\';
            esc[1] = (char)*p;
            sb_append(sb, esc, 2);
        } else if (*p < 0x20) {
            snprintf(esc, sizeof esc, "\\u%04x", *p);
            sb_append(sb, esc, 6);
        } else {
            sb_append(sb, (const char *)p, 1);
        }
    }
    sb_append(sb, "\"", 1);
}

/* Installs kLoopJs once per loop. Why: the helper must exist before the
   first call, and the shims it reads may load after game_loop_create.
   Returns whether it is usable; a failure is logged once and the loop then
   runs without JS, as it did when QuickJS had no context. Main thread. */
static bool ensure_js(GameLoop *loop)
{
    if (loop->js_ready || loop->js_failed) return loop->js_ready;
    loop->js_ready = js_engine_eval(loop->engine, kLoopJs, "<game-loop>");
    if (!loop->js_ready) {
        loop->js_failed = true;
        error_handler_log(LOG_ERROR, "Game loop: cannot install its JS helper");
    }
    return loop->js_ready;
}

/* Runs __smz_gameLoop(op, argv...) and returns its result string.
   `op` names one of kLoopJs's ops; `argv` holds `argc` (0..4) arguments.
   Returns a malloc'd string the caller frees with js_engine_free_string,
   or NULL when the helper is unavailable or JS was stopped uncatchably
   (the watchdog); the engine has then reported why. Main thread only. */
static char *loop_js(GameLoop *loop, const char *op, int argc, const LoopArg *argv)
{
    if (!ensure_js(loop)) return NULL;
    StrBuf sb = { NULL, 0, 0, true };
    sb_append(&sb, "__smz_gameLoop(", 15);
    sb_js_string(&sb, op);
    for (int i = 0; i < argc; i++) {
        sb_append(&sb, ",", 1);
        if (argv[i].str) {
            sb_js_string(&sb, argv[i].str);
        } else {
            char num[40];
            int n = snprintf(num, sizeof num, "%.17g", argv[i].num);
            sb_append(&sb, num, (size_t)n);
        }
    }
    sb_append(&sb, ")", 1);
    char *res = NULL;
    if (sb.ok) {
        res = js_engine_eval_string(loop->engine, sb.data, "<game-loop>");
    } else {
        error_handler_log(LOG_ERROR, "Game loop: out of memory calling JS (%s)", op);
    }
    free(sb.data);
    return res;
}

/* Logs a thrown exception from an 'x' result (`text` is what follows the
   code) with the same lines error_handler_report_js_exception writes. */
static void log_js_failure(const char *text)
{
    const char *sep = strchr(text, '\x01');
    if (!sep) {
        error_handler_log(LOG_ERROR, "JS Exception: %s", text);
        return;
    }
    error_handler_log(LOG_ERROR, "JS Exception: %.*s", (int)(sep - text), text);
    error_handler_log(LOG_ERROR, "Stack trace:\n%s", sep + 1);
}

/* Calls a JS function through the helper and reports an exception.
   `op` is "phase" (a function snapshotted on the first frame), "call" (a
   global looked up now) or "control"; `argv[0]` names the function for the
   first two. Returns the result code ('n', 't', 'f', 'x'), or 0 when
   nothing could run. Main thread only. */
static char call_js(GameLoop *loop, const char *op, int argc, const LoopArg *argv)
{
    char *res = loop_js(loop, op, argc, argv);
    if (!res) return 0;
    char code = res[0];
    if (code == 'x') log_js_failure(res + 1);
    js_engine_free_string(res);
    return code;
}

/* Calls the phase function `name` with `argc` (0..2) Number arguments. */
static void call_phase(GameLoop *loop, const char *name, int argc,
                       double a, double b)
{
    LoopArg argv[3] = { { 0, name }, { a, NULL }, { b, NULL } };
    call_js(loop, "phase", argc + 1, argv);
}

/* Snapshots the dom_shim.js phase functions inside JS on the first frame.
   Why: as in Outsider, later reassignment of those globals does not change
   what the loop calls, and lookups by name are not repeated per frame.
   Tried once per loop; if it fails the phases are skipped. */
static void cache_js_functions(GameLoop *loop)
{
    if (loop->js_cache_tried) return;
    loop->js_cache_tried = true;
    loop->js_funcs_cached = call_js(loop, "cache", 0, NULL) == 't';
}

GameLoop *game_loop_create(Platform *platform, JSEngine *engine, int target_fps)
{
    if (!platform || !engine) return NULL;
    if (target_fps <= 0) target_fps = 60;

    GameLoop *loop = calloc(1, sizeof(GameLoop));
    if (!loop) return NULL;

    loop->platform = platform;
    loop->engine = engine;
    loop->target_fps = target_fps;
    loop->frame_time_ms = 1000.0 / (double)target_fps;
    loop->running = true;

    loop->perf_freq = SDL_GetPerformanceFrequency();
    loop->start_ticks = SDL_GetPerformanceCounter();
    loop->last_frame_ticks = loop->start_ticks;
    loop->fps_timer_ticks = loop->start_ticks;
    loop->fps = 0.0;
    loop->frame_count = 0;

    return loop;
}

/* Frees the loop. The engine must still be alive: control mode flushes
   its last replies through JS. Neither the platform nor the engine is
   destroyed. */
void game_loop_destroy(GameLoop *loop)
{
    if (!loop) return;
    control_shutdown(loop);
    free(loop->screenshot_path);
    free(loop->probe_expr);
    free(loop);
}

void game_loop_set_perf_report(GameLoop *loop, int every_frames)
{
    if (!loop) return;
    loop->perf_interval = every_frames > 0 ? every_frames : 0;
    loop->perf_count = 0;
    memset(&loop->perf_acc, 0, sizeof(loop->perf_acc));
}

void game_loop_set_exit_probe(GameLoop *loop, const char *js_expression)
{
    if (!loop) return;
    free(loop->probe_expr);
    loop->probe_expr = js_expression ? strdup(js_expression) : NULL;
}

void game_loop_set_frame_limit(GameLoop *loop, int max_frames,
                               const char *screenshot_path, Renderer **renderer_slot)
{
    if (!loop) return;
    loop->max_frames = max_frames > 0 ? max_frames : 0;
    free(loop->screenshot_path);
    loop->screenshot_path = screenshot_path ? strdup(screenshot_path) : NULL;
    loop->screenshot_renderer_slot = renderer_slot;
}

/* Write the current default framebuffer as a binary PPM (RGB, top-down). */
static bool write_screenshot(GameLoop *loop, const char *path)
{
    Renderer *r = loop->screenshot_renderer_slot ? *loop->screenshot_renderer_slot : NULL;
    int w = 0, h = 0;
    platform_get_window_size(loop->platform, &w, &h);
    uint8_t *pixels = r ? renderer_read_pixels(r, 0, w, h) : NULL;
    if (!pixels) {
        error_handler_log(LOG_WARN, "Screenshot failed: no renderer or read-back failed");
        return false;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        error_handler_log(LOG_WARN, "Screenshot failed: cannot open %s", path);
        free(pixels);
        return false;
    }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (size_t i = 0, n = (size_t)w * (size_t)h; i < n; i++) {
        fwrite(pixels + i * 4, 1, 3, f);
    }
    fclose(f);
    free(pixels);
    error_handler_log(LOG_INFO, "Screenshot written: %s (%dx%d)", path, w, h);
    return true;
}

/* --- Control mode ---------------------------------------------------------
   A reader thread turns stdin into a queue of lines; the loop drains the
   queue at the start of every frame. Replies go to stdout as
   "@@ctl <id> <ok|err> <json>" so a driver can find them among the logs. */

static void control_send(long id, const char *status, const char *json)
{
    printf("@@ctl %ld %s %s\n", id, status, json);
    fflush(stdout);
}

/* Prints the lines control_shim.js passed to __native_control_send since
   the last flush, in order. Why: the loop keeps no native function in JS,
   so kLoopJs queues those lines and the loop prints them after each
   command, at the end of every frame and at shutdown. Main thread only. */
static void control_flush_replies(GameLoop *loop)
{
    char *res = loop_js(loop, "drain", 0, NULL);
    if (res && res[0] == 'v') {
        printf("%s\n", res + 1);
        fflush(stdout);
    }
    js_engine_free_string(res);
}

/* Read one line of any length from stdin (without the newline). */
static char *read_stdin_line(void)
{
    size_t cap = 4096, len = 0;
    char *buf = malloc(cap);
    if (!buf) return NULL;
    int c;
    while ((c = fgetc(stdin)) != EOF) {
        if (c == '\n') break;
        if (len + 1 >= cap) {
            cap *= 2;
            char *grown = realloc(buf, cap);
            if (!grown) { free(buf); return NULL; }
            buf = grown;
        }
        buf[len++] = (char)c;
    }
    if (c == EOF && len == 0) { free(buf); return NULL; }
    while (len > 0 && buf[len - 1] == '\r') len--;
    buf[len] = '\0';
    return buf;
}

static int control_reader_thread(void *arg)
{
    GameLoop *loop = arg;
    for (;;) {
        char *text = read_stdin_line();
        SDL_LockMutex(loop->control_mutex);
        if (!text) {
            loop->control_eof = true;
            SDL_UnlockMutex(loop->control_mutex);
            return 0;
        }
        ControlLine *line = calloc(1, sizeof(*line));
        if (line) {
            line->text = text;
            if (loop->control_tail) loop->control_tail->next = line;
            else loop->control_head = line;
            loop->control_tail = line;
        } else {
            free(text);
        }
        SDL_UnlockMutex(loop->control_mutex);
    }
}

static ControlLine *control_pop(GameLoop *loop, bool *eof)
{
    SDL_LockMutex(loop->control_mutex);
    ControlLine *line = loop->control_head;
    if (line) {
        loop->control_head = line->next;
        if (!loop->control_head) loop->control_tail = NULL;
    }
    *eof = loop->control_eof && !loop->control_head;
    SDL_UnlockMutex(loop->control_mutex);
    return line;
}

/* Turns on control mode. Defines __native_control_send through kLoopJs and
   takes __control_eval from control_shim.js, which must already be loaded;
   returns false (logged) if it is not, or the reader thread cannot start. */
bool game_loop_enable_control(GameLoop *loop, struct Renderer **renderer_slot)
{
    if (!loop || loop->control_enabled) return false;
    if (!ensure_js(loop)) return false;

    loop->control_mutex = SDL_CreateMutex();
    if (!loop->control_mutex) return false;

    if (call_js(loop, "controlInit", 0, NULL) != 't') {
        error_handler_log(LOG_ERROR, "Control mode: __control_eval missing (control_shim.js not loaded)");
        return false;
    }

    loop->screenshot_renderer_slot = renderer_slot;
    loop->control_enabled = true;
    loop->control_thread = SDL_CreateThread(control_reader_thread, "control-stdin", loop);
    if (!loop->control_thread) {
        error_handler_log(LOG_ERROR, "Control mode: cannot start stdin reader");
        loop->control_enabled = false;
        return false;
    }
    error_handler_log(LOG_INFO, "Control mode enabled: reading commands from stdin");
    return true;
}

/* Handle one "<id> <command> [payload]" line. An eval runs
   __control_eval(id, payload) at once and prints the replies it sent. */
static void control_handle_line(GameLoop *loop, const char *text)
{
    char *end = NULL;
    long id = strtol(text, &end, 10);
    if (end == text) return;                      /* not a command line */
    while (*end == ' ') end++;
    const char *cmd = end;
    while (*end && *end != ' ') end++;
    size_t cmd_len = (size_t)(end - cmd);
    while (*end == ' ') end++;
    const char *payload = end;

    if (cmd_len == 4 && strncmp(cmd, "eval", 4) == 0) {
        LoopArg argv[2] = { { (double)id, NULL }, { 0, payload } };
        if (call_js(loop, "control", 2, argv) == 'x') {
            control_send(id, "err", "\"__control_eval threw\"");
        }
        control_flush_replies(loop);
    } else if (cmd_len == 4 && strncmp(cmd, "shot", 4) == 0) {
        if (loop->control_shot_path) {
            control_send(id, "err", "\"a screenshot is already pending this frame\"");
        } else if (!*payload) {
            control_send(id, "err", "\"shot needs a file path\"");
        } else {
            loop->control_shot_id = id;
            loop->control_shot_path = strdup(payload);
        }
    } else if (cmd_len == 4 && strncmp(cmd, "quit", 4) == 0) {
        control_send(id, "ok", "null");
        loop->running = false;
    } else {
        control_send(id, "err", "\"unknown command\"");
    }
}

/* Drain queued commands. Before the first frame this blocks until the
   driver has sent something, so a prelude can run before any game frame. */
static void control_dispatch(GameLoop *loop)
{
    bool eof = false;
    for (;;) {
        ControlLine *line = control_pop(loop, &eof);
        if (!line) {
            if (eof) {
                error_handler_log(LOG_INFO, "Control mode: stdin closed, stopping");
                loop->running = false;
                return;
            }
            if (loop->control_started) return;
            SDL_Delay(1);
            continue;
        }
        loop->control_started = true;
        control_handle_line(loop, line->text);
        free(line->text);
        free(line);
        if (!loop->running) return;
    }
}

/* End of frame in control mode: print replies sent by promise jobs this
   frame, then take a requested screenshot and answer it. */
static void control_finish_frame(GameLoop *loop)
{
    control_flush_replies(loop);
    if (!loop->control_shot_path) return;
    bool ok = write_screenshot(loop, loop->control_shot_path);
    control_send(loop->control_shot_id, ok ? "ok" : "err",
                 ok ? "null" : "\"screenshot failed\"");
    free(loop->control_shot_path);
    loop->control_shot_path = NULL;
}

/* Leaves control mode: prints replies still queued in JS (the engine is
   still alive, see game_loop_destroy) and frees the pending shot path. */
static void control_shutdown(GameLoop *loop)
{
    if (!loop->control_enabled) return;
    /* The reader thread blocks in fgetc; detach it so shutdown never waits
       on a driver that keeps stdin open. */
    if (loop->control_thread) SDL_DetachThread(loop->control_thread);
    control_flush_replies(loop);
    free(loop->control_shot_path);
    /* Lines left in the queue are leaked deliberately: the reader thread
       may still be appending, and the process is about to exit. */
    loop->control_enabled = false;
}

/* Convert performance counter ticks to milliseconds. */
static double ticks_to_ms(const GameLoop *loop, uint64_t ticks)
{
    return (double)ticks * 1000.0 / (double)loop->perf_freq;
}

double game_loop_elapsed_ms(const GameLoop *loop)
{
    if (!loop) return 0.0;
    uint64_t now = SDL_GetPerformanceCounter();
    return ticks_to_ms(loop, now - loop->start_ticks);
}

double game_loop_get_fps(const GameLoop *loop)
{
    if (!loop) return 0.0;
    return loop->fps;
}

GameLoopStats game_loop_get_stats(const GameLoop *loop)
{
    GameLoopStats empty = {0};
    if (!loop) return empty;
    return loop->stats;
}

void game_loop_request_stop(GameLoop *loop)
{
    if (loop) loop->running = false;
}

/* SDL quit or window close: let the nw.Window shim emit "close". As in NW.js,
 * a "close" listener keeps the app running until it calls win.close(true) or
 * nw.App.quit(). Returns true when the loop should stop: also when
 * __nw_hostClose is missing or throws (logged), so a broken listener never
 * leaves the window unclosable. */
static bool host_close_quits(GameLoop *loop)
{
    LoopArg name = { 0, "__nw_hostClose" };
    bool quit = call_js(loop, "call", 1, &name) != 'f';
    if (!quit) error_handler_log(LOG_INFO, "Window close deferred to a 'close' listener");
    return quit;
}

/* Phase 1 after SDL polling: a resize, queued input events and gamepads go
   to dom_shim.js. Skipped while the JS side is unavailable. */
static void run_input_phase(GameLoop *loop)
{
    if (!loop->js_funcs_cached) return;
    int rw, rh;
    if (platform_check_resize(loop->platform, &rw, &rh)) {
        call_phase(loop, "__dom_setWindowSize", 2, rw, rh);
    }
    call_phase(loop, "__dom_flushInputEvents", 0, 0, 0);
    call_phase(loop, "__dom_updateGamepads", 0, 0, 0);
}

/* Phase 4: rAF callbacks with the loop's elapsed time (ms since
   game_loop_create) as their DOMHighResTimeStamp, then a job drain. */
static void run_raf_phase(GameLoop *loop)
{
    if (loop->js_funcs_cached) {
        call_phase(loop, "__dom_flushAnimationFrames", 1,
                   game_loop_elapsed_ms(loop), 0);
    }
    js_engine_execute_pending_jobs(loop->engine);
}

/* Evaluates the exit probe (or the default active-scene expression) and
   writes its String() form, or "probe threw: <error>", to `out`; leaves
   `out` alone when JS could not run at all. */
static void run_exit_probe(GameLoop *loop, char *out, size_t cap)
{
    LoopArg src = { 0, loop->probe_expr ? loop->probe_expr :
        "typeof SceneManager !== 'undefined' && SceneManager._scene"
        "    ? SceneManager._scene.constructor.name : 'none'" };
    char *res = loop_js(loop, "probe", 1, &src);
    if (!res) return;                     /* reported by the engine */
    if (res[0] == 'v') {
        snprintf(out, cap, "%s", res + 1);
    } else {
        snprintf(out, cap, "probe threw: %s", res + 1);
    }
    js_engine_free_string(res);
}

/* Debug frame limit: after max_frames frames, capture the final frame
   before it is swapped, log where the game got to, and stop. */
static void check_frame_limit(GameLoop *loop)
{
    loop->total_frames++;
    if (loop->max_frames <= 0 || loop->total_frames < loop->max_frames) return;
    if (loop->screenshot_path) {
        write_screenshot(loop, loop->screenshot_path);
    }
    /* Report engine state so a log alone says where the game got to:
       the active scene by default, or the caller's probe expression. */
    char probe_result[1024] = "unknown";
    run_exit_probe(loop, probe_result, sizeof(probe_result));
    error_handler_log(LOG_INFO, "Frame limit reached (%d frames), stopping; %s: %s",
                      loop->total_frames, loop->probe_expr ? "probe" : "scene",
                      probe_result);
    loop->running = false;
}

/* Phase 5: the frame was drawn into an offscreen surface; putting it on
   screen is the renderer's job, and the window swap follows it. */
static void present_frame(GameLoop *loop)
{
    Renderer *r = loop->screenshot_renderer_slot ? *loop->screenshot_renderer_slot : NULL;
    if (r) {
        int ww = 0, wh = 0;
        platform_get_window_size(loop->platform, &ww, &wh);
        renderer_set_output_size(r, ww, wh);
        renderer_present(r);
    }
    platform_swap_buffers(loop->platform);
}

/* Optional periodic phase report (debug aid). */
static void report_perf(GameLoop *loop)
{
    if (loop->perf_interval <= 0) return;
    loop->perf_acc.events_ms += loop->stats.events_ms;
    loop->perf_acc.timers_ms += loop->stats.timers_ms;
    loop->perf_acc.jobs_ms   += loop->stats.jobs_ms;
    loop->perf_acc.raf_ms    += loop->stats.raf_ms;
    loop->perf_acc.swap_ms   += loop->stats.swap_ms;
    loop->perf_acc.total_ms  += loop->stats.total_ms;
    if (++loop->perf_count >= loop->perf_interval) {
        double n = (double)loop->perf_count;
        error_handler_log(LOG_INFO,
            "perf: %d frames avg %.1f ms (events %.1f, timers %.1f, jobs %.1f, "
            "rAF %.1f, swap %.1f)",
            loop->perf_count, loop->perf_acc.total_ms / n,
            loop->perf_acc.events_ms / n, loop->perf_acc.timers_ms / n,
            loop->perf_acc.jobs_ms / n, loop->perf_acc.raf_ms / n,
            loop->perf_acc.swap_ms / n);
        memset(&loop->perf_acc, 0, sizeof(loop->perf_acc));
        loop->perf_count = 0;
    }
}

/* FPS tracking, then manual pacing to the target FPS in case VSync is off
   or not working. `now` is the end of this frame's present. */
static void pace_frame(GameLoop *loop, uint64_t now)
{
    loop->frame_count++;
    uint64_t fps_elapsed = now - loop->fps_timer_ticks;
    double fps_elapsed_ms = ticks_to_ms(loop, fps_elapsed);
    if (fps_elapsed_ms >= 1000.0) {
        loop->fps = (double)loop->frame_count * 1000.0 / fps_elapsed_ms;
        loop->frame_count = 0;
        loop->fps_timer_ticks = now;
    }

    uint64_t frame_elapsed = now - loop->last_frame_ticks;
    double frame_elapsed_ms = ticks_to_ms(loop, frame_elapsed);
    if (frame_elapsed_ms < loop->frame_time_ms) {
        double sleep_ms = loop->frame_time_ms - frame_elapsed_ms;
        if (sleep_ms > 1.0) {
            SDL_Delay((uint32_t)(sleep_ms - 1.0));
        }
    }
    loop->last_frame_ticks = SDL_GetPerformanceCounter();
}

/* Runs one frame: events, timers, jobs, rAF (+ jobs), video, audio,
   control and frame-limit checks, present, then stats and pacing. Returns
   false once the loop should exit. Main thread only. */
bool game_loop_step(GameLoop *loop)
{
    if (!loop || !loop->running) return false;

    uint64_t frame_start = SDL_GetPerformanceCounter();
    uint64_t phase_start, phase_end;

    cache_js_functions(loop);

    if (loop->control_enabled) {
        control_dispatch(loop);
        if (!loop->running) return false;
    }

    js_engine_watchdog_arm(loop->engine);

    /* --- Phase 1: Events (SDL polling + resize + input) --- */
    phase_start = frame_start;
    if (!platform_poll_events(loop->platform) && host_close_quits(loop)) {
        loop->running = false;
        return false;
    }
    run_input_phase(loop);
    phase_end = SDL_GetPerformanceCounter();
    loop->stats.events_ms = ticks_to_ms(loop, phase_end - phase_start);

    /* --- Phase 2: Timers --- */
    phase_start = phase_end;
    if (loop->js_funcs_cached) call_phase(loop, "__dom_flushTimers", 0, 0, 0);
    phase_end = SDL_GetPerformanceCounter();
    loop->stats.timers_ms = ticks_to_ms(loop, phase_end - phase_start);

    /* --- Phase 3: Microtasks/Jobs --- */
    phase_start = phase_end;
    js_engine_execute_pending_jobs(loop->engine);
    phase_end = SDL_GetPerformanceCounter();
    loop->stats.jobs_ms = ticks_to_ms(loop, phase_end - phase_start);

    /* --- Phase 4: requestAnimationFrame callbacks, then video over the
       finished frame --- */
    phase_start = phase_end;
    run_raf_phase(loop);
    uint64_t prev = loop->prev_frame_start_ticks ? loop->prev_frame_start_ticks : frame_start;
    loop->prev_frame_start_ticks = frame_start;
    video_player_update(ticks_to_ms(loop, frame_start - prev) / 1000.0);
    video_player_draw();
    phase_end = SDL_GetPerformanceCounter();
    loop->stats.raf_ms = ticks_to_ms(loop, phase_end - phase_start);

    audio_update();                                  /* fades, voice cleanup */
    if (loop->control_enabled) control_finish_frame(loop);
    check_frame_limit(loop);

    /* --- Phase 5: Present --- */
    phase_start = phase_end;
    present_frame(loop);
    phase_end = SDL_GetPerformanceCounter();
    loop->stats.swap_ms = ticks_to_ms(loop, phase_end - phase_start);
    loop->stats.total_ms = ticks_to_ms(loop, phase_end - frame_start);

    report_perf(loop);
    pace_frame(loop, phase_end);
    return true;
}

void game_loop_run(GameLoop *loop)
{
    if (!loop) return;
    while (game_loop_step(loop)) {
    }
}
