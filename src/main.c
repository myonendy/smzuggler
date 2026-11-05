/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-10-30: use the configurable runtime identity.
 * Modified 2026-10-31: dispatch headless surface inventory before startup.
 * Modified 2026-10-31: dispatch --share-bundle to the packager.
 * Modified 2026-10-31: return process.exitCode after a clean quit.
 * Modified 2026-11-01: default logs, .jsc cache and config to host data dirs.
 *
 * Modified 2026-11-04: keep runtime services on the process main thread.
 * Modified 2026-11-05: start an optional V8 CPU profile from --cpu-prof.
 *
 * Runtime entry point; owns startup and shutdown on the main thread.
 * See docs/DECISIONS.md (DEC-M0-T001) for identity ownership.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__linux__) || defined(__ANDROID__)
#include <SDL3/SDL.h>
#endif

#ifndef RMMZ_DATA_APP
#define RMMZ_DATA_APP "outsider"
#endif

/** Drop a trailing slash so later joins do not produce empty path segments. */
static void rmmz_trim_slash(char *path)
{
    size_t n;
    if (!path) return;
    n = strlen(path);
    while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\'))
        path[--n] = '\0';
}

/** Fill OUT with the platform state dir used by --data-dir without PATH.
 * Linux (not Android) uses $XDG_STATE_HOME/<app> or $HOME/.local/state/<app>
 * when XDG_STATE_HOME is unset; other platforms use SDL_GetPrefPath.
 * out is caller-owned. Returns false if the environment/pref path is missing
 * or the result does not fit. */
static bool rmmz_platform_state_dir(char *out, size_t out_size)
{
#if defined(__linux__) && !defined(__ANDROID__)
    char prefix[1024];
    const char *base = getenv("XDG_STATE_HOME");
    int n;
    if (base && base[0]) {
        n = snprintf(prefix, sizeof(prefix), "%s", base);
        if (n <= 0 || (size_t)n >= sizeof(prefix)) return false;
        rmmz_trim_slash(prefix);
        n = snprintf(out, out_size, "%s/%s", prefix, RMMZ_DATA_APP);
        return n > 0 && (size_t)n < out_size;
    }
    base = getenv("HOME");
    if (!base || !base[0]) return false;
    n = snprintf(prefix, sizeof(prefix), "%s", base);
    if (n <= 0 || (size_t)n >= sizeof(prefix)) return false;
    rmmz_trim_slash(prefix);
    n = snprintf(out, out_size, "%s/.local/state/%s", prefix, RMMZ_DATA_APP);
    return n > 0 && (size_t)n < out_size;
#else
    char *pref = SDL_GetPrefPath("", RMMZ_DATA_APP);
    int n;
    if (!pref || !out || out_size == 0) {
        if (pref) SDL_free(pref);
        return false;
    }
    n = snprintf(out, out_size, "%s", pref);
    SDL_free(pref);
    if (n <= 0 || (size_t)n >= out_size) return false;
    rmmz_trim_slash(out);
    return true;
#endif
}

/** Resolve the directory that debug.log and console.log are written to.
 * argv is borrowed and scanned for --data-dir / --data-dir=PATH (last one
 * wins). game_dir is the already-resolved --game path, or NULL. Without the
 * flag the game directory is used and an empty result means logs stay in
 * the working directory, matching the historical default. out is
 * caller-owned. Returns false if the path does not fit in out_size. */
bool rmmz_resolve_log_dir(int argc, char **argv, const char *game_dir,
                          char *out, size_t out_size)
{
    bool use_data_dir = false;
    const char *data_dir_path = NULL;
    int i, n;
    if (!out || out_size == 0) return false;
    out[0] = '\0';
    for (i = 1; argv && i < argc; i++) {
        if (!argv[i]) continue;
        if (strcmp(argv[i], "--data-dir") == 0) {
            use_data_dir = true;
            data_dir_path = NULL;
        } else if (strncmp(argv[i], "--data-dir=", 11) == 0) {
            use_data_dir = true;
            data_dir_path = argv[i] + 11;
        }
    }
    if (use_data_dir) {
        if (data_dir_path && data_dir_path[0]) {
            n = snprintf(out, out_size, "%s", data_dir_path);
            if (n <= 0 || (size_t)n >= out_size) return false;
            rmmz_trim_slash(out);
            return true;
        }
        return rmmz_platform_state_dir(out, out_size);
    }
    if (game_dir && game_dir[0]) {
        n = snprintf(out, out_size, "%s", game_dir);
        return n > 0 && (size_t)n < out_size;
    }
    return true;
}

/* Compile-time seam so tests/test_data_dir.c can link this helper without
 * pulling the rest of the runtime or a second main(). */
#ifndef RMMZ_TEST_DATA_DIR

#include "platform/platform.h"
#ifdef SMZUGGLER_NAME
#include "smzuggler/data_dirs.h"
#endif
#include "smzuggler/dump_surface.h"
#include "engine/js_engine.h"
#include "engine/js_profiler.h"
#include "engine/game_loop.h"
#include "engine/script_loader.h"
#include "bindings/bind_io.h"
#include "bindings/bind_image.h"
#include "bindings/bind_canvas2d.h"
#include "bindings/bind_renderer.h"
#include "bindings/bind_font.h"
#include "bindings/bind_tilemap.h"
#include "bindings/bind_audio.h"
#include "bindings/bind_input.h"
#include "bindings/bind_effekseer.h"
#include "bindings/bind_video.h"
#include "bindings/bind_filters.h"
#include "bindings/bind_platform.h"
#include "engine/error_handler.h"
#include "audio/audio_engine.h"
#include "effects/effekseer_backend.h"
#include "video/video_player.h"
#include "input/input_manager.h"
#include "rendering/image_loader.h"
#include "rendering/canvas2d.h"
#include "rendering/font_manager.h"
#include "rendering/renderer.h"
#include "rendering/filters.h"
#include "io/file_io.h"
#include "platform/win_console.h"
#include "platform/android_support.h"

#include <SDL3/SDL.h>
/* SDL3 dropped the SDL2main library: including this header in the file that
   defines main() supplies the Windows GUI-subsystem WinMain entry point. */
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <direct.h>
#include <process.h>
#define smz_getcwd _getcwd
#else
#include <unistd.h>
#include <sys/wait.h>
#define smz_getcwd getcwd
#endif

#ifndef RMMZ_DEFAULT_WIDTH
#define RMMZ_DEFAULT_WIDTH 816
#endif
#ifndef RMMZ_DEFAULT_HEIGHT
#define RMMZ_DEFAULT_HEIGHT 624
#endif

#define APP_TITLE RMMZ_APP_NAME

/* Report a fatal startup error: log it and, where the platform provides one,
   show a dialog so the message is visible even without a console. */
static void fatal_error(const char *fmt, ...)
{
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    error_handler_log(LOG_ERROR, "%s", msg);
    platform_show_error_dialog(APP_TITLE, msg);
}

/** True when DIR contains the packager; copies its path into OUT (caller-owned). */
static bool share_try_script(const char *dir, char *out, size_t n)
{
    int k = snprintf(out, n, "%s/tools/share/make_bundle.py", dir);
    if (k > 0 && (size_t)k < n && file_io_exists(out)) return true;
    k = snprintf(out, n, "%s/share/make_bundle.py", dir);
    return k > 0 && (size_t)k < n && file_io_exists(out);
}

/** Strip a trailing slash so parent walking does not stall on SDL_GetBasePath. */
static void share_trim(char *dir)
{
    size_t n = strlen(dir);
    while (n > 1 && (dir[n - 1] == '/' || dir[n - 1] == '\\'))
        dir[--n] = '\0';
}

/** Walk START and its parents looking for the packager; OUT is caller-owned. */
static bool share_walk(const char *start, char *out, size_t n)
{
    char dir[1024];
    if (!start || !start[0]) return false;
    snprintf(dir, sizeof(dir), "%s", start);
    share_trim(dir);
    for (int i = 0; i < 8; i++) {
        char *slash;
        if (share_try_script(dir, out, n)) return true;
        slash = strrchr(dir, '/');
#ifdef _WIN32
        {
            char *alt = strrchr(dir, '\\');
            if (!slash || (alt && alt > slash)) slash = alt;
        }
#endif
        if (!slash || slash == dir) break;
        *slash = '\0';
        if (!dir[0]) break;
    }
    return false;
}

/** Locate make_bundle.py from the executable, argv0 or cwd; OUT is caller-owned. */
static bool share_find(const char *argv0, char *out, size_t n)
{
    const char *base = SDL_GetBasePath();
    char cwd[1024];
    if (base && share_walk(base, out, n)) return true;
    if (smz_getcwd(cwd, sizeof(cwd)) && share_walk(cwd, out, n)) return true;
    if (argv0 && strpbrk(argv0, "/\\")) {
        char *slash;
        snprintf(cwd, sizeof(cwd), "%s", argv0);
        share_trim(cwd);
        slash = strrchr(cwd, '/');
#ifdef _WIN32
        {
            char *alt = strrchr(cwd, '\\');
            if (!slash || (alt && alt > slash)) slash = alt;
        }
#endif
        if (slash) {
            *slash = '\0';
            if (share_walk(cwd, out, n)) return true;
        }
    }
    return false;
}

/** Run ARGS as python3 and return its exit status; 127 if the spawn itself fails. */
static int share_spawn(char *const *args)
{
#ifdef _WIN32
    int rc = _spawnvp(_P_WAIT, args[0], args);
    return rc < 0 ? 127 : rc;
#else
    pid_t pid = fork();
    int status = 0;
    if (pid < 0) return 127;
    if (pid == 0) {
        execvp(args[0], args);
        _exit(127);
    }
    if (waitpid(pid, &status, 0) < 0) return 127;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return 1;
#endif
}

/** Dispatch --share-bundle to the Python packager for the last state-dir run.
 * out_path may be NULL (default zip name). argv0 locates the source tree.
 * The calling thread owns the strings; this function does not keep them. */
static int share_bundle_run(const char *out_path, const char *argv0)
{
    char script[1024];
    const char *out;
    char *args[6];
    if (!share_find(argv0, script, sizeof(script))) {
        fprintf(stderr, "share-bundle: tools/share/make_bundle.py not found\n");
        return 1;
    }
    out = (out_path && out_path[0]) ? out_path : "share-bundle.zip";
    args[0] = "python3";
    args[1] = script;
    args[2] = "--last-run";
    args[3] = "-o";
    args[4] = (char *)out;
    args[5] = NULL;
    return share_spawn(args);
}

/* If a directory exists at "<exe dir>/<name>", copy its path into out.
   Used to locate a packaged game/shims folder next to the executable so
   the runtime can be launched without arguments (e.g. by double-click). */
static bool find_dir_next_to_exe(const char *name, char *out, size_t out_size)
{
    /* SDL3 owns this string; it must not be freed. */
    const char *base = SDL_GetBasePath();
    if (!base) return false;
    int n = snprintf(out, out_size, "%s%s", base, name);
    if (n <= 0 || (size_t)n >= out_size) return false;
    return file_io_is_directory(out);
}

/** Read process.exitCode after the loop stops so process.exit(code) becomes
 * the process status. Borrows js on the main thread; 0 when the code is
 * absent or evaluation fails. Node process.exit; compat process.exit. */
static int process_exit_status(JSEngine *js)
{
    char *text;
    int code;
    if (!js) return 0;
    text = js_engine_eval_string(js,
        "(typeof process === 'object' && process &&"
        " typeof process.exitCode === 'number') ? String(process.exitCode | 0) : '0'",
        "<exit-code>");
    if (!text) return 0;
    code = atoi(text);
    js_engine_free_string(text);
    return code;
}

/** Starts the optional V8 CPU profile requested by `--cpu-prof`.
 *
 * Why: path selection and start-up logging are one decision, so keeping them
 * out of runtime_main leaves the entry point readable. `path` is borrowed
 * from argv and may be NULL or empty, in which case the profile goes under
 * `<state_dir>/profiles`; `state_dir` and `game_dir` are borrowed and may be
 * NULL. Returns the caller-owned handle to pass to js_profiler_stop, or NULL
 * after logging when no path can be chosen or V8 refuses to start. Main
 * thread, before the first script is evaluated. */
static js_profiler *start_cpu_profile(jsb_runtime *jsb, const char *path,
                                      const char *state_dir, const char *game_dir)
{
    char default_path[1024];
    const char *out = path;
    if (!out || !out[0]) {
        if (!js_profiler_default_path(state_dir, default_path, sizeof default_path)) {
            error_handler_log(LOG_ERROR,
                              "--cpu-prof needs a state directory; pass --cpu-prof=FILE");
            return NULL;
        }
        out = default_path;
    }
    js_profiler *profiler = js_profiler_start(jsb, out, game_dir);
    if (profiler)
        error_handler_log(LOG_INFO, "CPU profile will be written to %s", out);
    else
        error_handler_log(LOG_ERROR, "Could not start the V8 CPU profiler");
    return profiler;
}

/** Start the runtime on the main thread using borrowed CLI arguments; return
 * an exit status on startup failure or after releasing runtime resources. */
static int runtime_main(int argc, char *argv[])
{
    /* Android: printf output is lost unless routed to logcat. No-op elsewhere. */
    android_support_init_logging();

    /* Parse command-line arguments. */
    const char *game_dir = NULL;
    const char *shim_dir = "src/shims";
    int win_w = RMMZ_DEFAULT_WIDTH;
    int win_h = RMMZ_DEFAULT_HEIGHT;
    bool explicit_resolution = false;
    int no_audio = 0;
    int max_frames = 0;
    const char *screenshot_path = NULL;
    const char *probe_expr = NULL;
    int watchdog_ms = 0;
    int perf_every = 0;
    bool control_mode = false;
    bool dump_requested = false;
    const char *dump_path = NULL;
    bool share_requested = false;
    const char *share_path = NULL;
    bool cpu_prof_requested = false;
    const char *cpu_prof_path = NULL;
#ifdef SMZUGGLER_NAME
    SmzHostDirs host_dirs = {0};
#else
    char log_path_buf[1024];
    char console_log_buf[1024];
    char log_dir_buf[1024];
#endif

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dump-surface") == 0) {
            dump_requested = true;
        } else if (strncmp(argv[i], "--dump-surface=", 15) == 0) {
            dump_requested = true;
            dump_path = argv[i] + 15;
        } else if (strcmp(argv[i], "--share-bundle") == 0) {
            share_requested = true;
        } else if (strncmp(argv[i], "--share-bundle=", 15) == 0) {
            share_requested = true;
            share_path = argv[i] + 15;
        } else if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
            game_dir = argv[++i];
        } else if (strcmp(argv[i], "--shims") == 0 && i + 1 < argc) {
            shim_dir = argv[++i];
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            /* Debug: exit cleanly after N frames. */
            max_frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            /* Debug: write the last frame (see --frames) as a PPM file. */
            screenshot_path = argv[++i];
        } else if (strcmp(argv[i], "--probe") == 0 && i + 1 < argc) {
            /* Debug: JS expression logged when --frames stops the loop. */
            probe_expr = argv[++i];
        } else if (strcmp(argv[i], "--perf") == 0 && i + 1 < argc) {
            /* Debug: log average per-phase frame times every N frames. */
            perf_every = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--watchdog") == 0 && i + 1 < argc) {
            /* Debug: interrupt JS that blocks a frame or eval for longer than
               this many milliseconds, logging where it was. */
            watchdog_ms = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--resolution") == 0 && i + 1 < argc) {
            if (sscanf(argv[++i], "%dx%d", &win_w, &win_h) != 2) {
                fatal_error("Invalid resolution format, expected WIDTHxHEIGHT");
                return 1;
            }
            explicit_resolution = true;
        } else if (strcmp(argv[i], "--control") == 0) {
            /* Automation: take eval/screenshot commands from stdin (see
               game_loop_enable_control and tools/visual_eval.py). */
            control_mode = true;
        } else if (strcmp(argv[i], "--cpu-prof") == 0) {
            /* Record a V8 CPU profile; the default path is under the state
               dir (see js_profiler_default_path). */
            cpu_prof_requested = true;
        } else if (strncmp(argv[i], "--cpu-prof=", 11) == 0) {
            cpu_prof_requested = true;
            cpu_prof_path = argv[i] + 11;
        } else if (strcmp(argv[i], "--no-audio") == 0) {
            no_audio = 1;
        } else if (strcmp(argv[i], "--data-dir") == 0 ||
                   strncmp(argv[i], "--data-dir=", 11) == 0) {
            /* Log dir. smz_host_dirs / rmmz_resolve_log_dir read argv later. */
            continue;
        } else if (argv[i][0] != '-') {
            /* Positional argument: treat as game directory. */
            game_dir = argv[i];
        }
    }

    if (share_requested) {
        return share_bundle_run(share_path, argv[0]);
    }

    if (dump_requested) {
        if (strcmp(shim_dir, "src/shims") == 0) shim_dir = SMZ_SURFACE_SHIM_DIR;
        return dump_surface_run(dump_path, shim_dir);
    }

    /* No explicit paths: look for a packaged layout next to the executable
       (build_game.py produces <exe dir>/game/ with shims inside it). */
    static char default_game_dir[1024];
    static char default_shim_dir[1024];
    if (!game_dir && find_dir_next_to_exe("game", default_game_dir, sizeof(default_game_dir))) {
        game_dir = default_game_dir;
    }
#ifdef __ANDROID__
    /* Android: the game lives in the app's external files directory
       (<storage>/Android/data/<package>/files/game) and the shims are
       unpacked there from the APK's assets on every launch. */
    static char android_root[1024];
    if (android_support_files_dir(android_root, sizeof(android_root))) {
        if (!game_dir) {
            snprintf(default_game_dir, sizeof(default_game_dir), "%s/game", android_root);
            if (file_io_is_directory(default_game_dir)) game_dir = default_game_dir;
        }
        if (android_support_extract_shims(android_root, script_loader_shim_files())) {
            snprintf(default_shim_dir, sizeof(default_shim_dir), "%s/shims", android_root);
            shim_dir = default_shim_dir;
        }
    }
#endif
    if (strcmp(shim_dir, "src/shims") == 0 && !file_io_is_directory(shim_dir)) {
        if (game_dir) {
            snprintf(default_shim_dir, sizeof(default_shim_dir), "%s/shims", game_dir);
            if (file_io_is_directory(default_shim_dir)) shim_dir = default_shim_dir;
        }
        if (shim_dir != default_shim_dir &&
            find_dir_next_to_exe("shims", default_shim_dir, sizeof(default_shim_dir))) {
            shim_dir = default_shim_dir;
        }
    }

    /* Determine log file paths. smzuggler always uses the host state dir
       (or --data-dir=PATH). Upstream without SMZUGGLER_NAME keeps logs in
       the game directory unless --data-dir is passed. */
    const char *log_path = NULL;
    const char *console_log_path = NULL;
#ifdef SMZUGGLER_NAME
    smz_host_dirs(argc, argv, game_dir, &host_dirs);
    if (host_dirs.log_dir[0]) {
        file_io_mkdir(host_dirs.log_dir);
        log_path = host_dirs.debug_log;
        console_log_path = host_dirs.console_log;
    }
#else
    log_path = "debug.log";
    console_log_path = "console.log";
    if (rmmz_resolve_log_dir(argc, argv, game_dir, log_dir_buf,
                             sizeof(log_dir_buf)) &&
        log_dir_buf[0]) {
        /* The game dir already exists; the state dir or explicit PATH may not. */
        if (!game_dir || strcmp(log_dir_buf, game_dir) != 0)
            file_io_mkdir(log_dir_buf);
        snprintf(log_path_buf, sizeof(log_path_buf), "%s/debug.log", log_dir_buf);
        log_path = log_path_buf;
        snprintf(console_log_buf, sizeof(console_log_buf), "%s/console.log", log_dir_buf);
        console_log_path = console_log_buf;
    }
#endif

    /* Directory for a default --cpu-prof file: next to the logs, which are
       already the host state directory (or an explicit --data-dir). */
#ifdef SMZUGGLER_NAME
    const char *state_dir = host_dirs.log_dir;
#else
    const char *state_dir = log_dir_buf[0] ? log_dir_buf : NULL;
#endif

    /* GUI-subsystem builds start without stdout/stderr; attach to the parent
       console if there is one, otherwise capture raw output to console.log. */
    platform_setup_console(console_log_path);
    /* Progress messages are few and matter most when the process dies before a
       normal exit would flush them, so do not buffer stdout. */
    setvbuf(stdout, NULL, _IONBF, 0);

    error_handler_init(log_path, LOG_INFO);
    error_handler_log(LOG_INFO, SMZUGGLER_NAME " (RPG Maker MZ native runtime) starting");
#ifdef SMZUGGLER_NAME
    if (host_dirs.config_dir[0])
        error_handler_log(LOG_INFO, "Config directory %s", host_dirs.config_dir);
#endif

    /* Validate the game directory before opening a window, so a bad launch
       (wrong path, bare double-click) produces a clear error instead of a
       black screen. */
    if (!game_dir) {
        fatal_error("No game directory specified.\n\n"
                    "Run with:  " SMZUGGLER_BIN " --game <path to RPG Maker MZ game>\n"
                    "or place a \"game\" folder next to the executable.");
        error_handler_shutdown();
        return 1;
    }
    {
        char main_js[1024];
        snprintf(main_js, sizeof(main_js), "%s/js/main.js", game_dir);
        if (!file_io_exists(main_js)) {
            fatal_error("Not an RPG Maker MZ game directory (js/main.js not found):\n%s",
                        game_dir);
            error_handler_shutdown();
            return 1;
        }
    }

    Platform *platform = platform_init(APP_TITLE, win_w, win_h);
    if (!platform) {
        fatal_error("Failed to initialize window or OpenGL 4.5 context.\n"
                    "Check that your graphics driver is up to date.");
        error_handler_shutdown();
        return 1;
    }

    /* The window opens in raw pixels (DPI-aware), while NW.js scales the
       game with the desktop. Match that so the default window looks the
       same size as the original player's; --resolution is taken literally. */
    if (!explicit_resolution) {
        float desktop_scale = platform_get_desktop_scale(platform);
        if (desktop_scale > 1.0f) {
            win_w = (int)(win_w * desktop_scale + 0.5f);
            win_h = (int)(win_h * desktop_scale + 0.5f);
            platform_set_window_size(platform, win_w, win_h);
            error_handler_log(LOG_INFO, "Desktop scale %.2f: window sized to %dx%d",
                              desktop_scale, win_w, win_h);
        }
    }

    error_handler_log(LOG_INFO, "Window created (%dx%d). %s ready.",
                      win_w, win_h,
#ifdef RMMZ_RENDER_GPU
                      "GPU device"
#else
                      "OpenGL"
#endif
                      );

    JSEngine *js = js_engine_init();
    if (!js) {
        fatal_error("Failed to initialize JS engine");
        platform_shutdown(platform);
        error_handler_shutdown();
        return 1;
    }
    error_handler_log(LOG_INFO, "QuickJS engine initialized");
#ifdef SMZUGGLER_NAME
    if (host_dirs.jsc_dir[0])
        js_engine_set_cache_dir(js, host_dirs.jsc_dir, game_dir);
#endif
    if (watchdog_ms > 0) {
        js_engine_set_watchdog(js, (unsigned)watchdog_ms);
        error_handler_log(LOG_INFO, "Script watchdog enabled: %d ms", watchdog_ms);
    }

    jsb_runtime *jsb = js_engine_get_jsb(js);

    /* V8 samples only scripts once the profiler runs, so it starts before
       the first shim or game script is evaluated and stops before V8's
       isolate is destroyed. */
    js_profiler *profiler = cpu_prof_requested
                                ? start_cpu_profile(jsb, cpu_prof_path, state_dir, game_dir)
                                : NULL;

    bind_io_define(jsb);

    /* Initialize image loader (GL enabled since we have a context). */
    image_loader_init(true);
    bind_image_define(jsb);

    canvas2d_init();
    bind_canvas2d_define(jsb);

    font_manager_init();
    bind_font_define(jsb);

    bind_renderer_define(jsb);

    bind_filters_define(jsb);

    /* Register tilemap bindings (depends on renderer). */
    bind_tilemap_define(jsb);

    if (no_audio) {
        audio_engine_set_disabled(true);
    } else {
        audio_engine_init();
    }
    bind_audio_define(jsb);

    input_manager_init();
    bind_input_define(jsb);

    bind_platform_define(jsb, platform);

    effekseer_init(win_w, win_h, 8000);
    bind_effekseer_define(jsb);

    {
        extern Renderer *s_renderer;
        video_player_init(&s_renderer);
    }
    bind_video_define(jsb);

    file_io_set_game_root(game_dir);
#ifdef SMZUGGLER_NAME
    file_io_set_write_guard(true);
#endif

    int scripts_ok = 1;
    error_handler_log(LOG_INFO, "Loading game from: %s", game_dir);
    if (!script_loader_load_all(js, shim_dir, game_dir)) {
        /* A bad --shims/--game path must not produce a black window that
           exits 0. Fail loudly instead. */
        fatal_error("Failed to load game scripts from:\n%s\n\nShims: %s\n\n"
                    "Check the --game and --shims paths (see debug.log).",
                    game_dir, shim_dir);
        scripts_ok = 0;
    }

    /* Initialize display info in JS (screen size, devicePixelRatio). */
    js_engine_eval(js, "typeof __dom_initDisplayInfo === 'function' && __dom_initDisplayInfo();",
                   "<display-init>");

    /* Propagate actual window size to JS so letterbox scaling and mouse
       coordinate conversion are correct from the first frame. */
    {
        int init_w, init_h;
        platform_get_window_size(platform, &init_w, &init_h);
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "__dom_setWindowSize && __dom_setWindowSize(%d, %d);",
                 init_w, init_h);
        js_engine_eval(js, buf, "<window-size-init>");
    }

    /* A failed script load skips the loop and takes the same teardown. */
    GameLoop *loop = scripts_ok ? game_loop_create(platform, js, 60) : NULL;
    if (!loop) {
        if (scripts_ok) fatal_error("Failed to create game loop");
        video_player_shutdown();
        effekseer_shutdown();
        input_manager_shutdown();
        if (!no_audio) audio_engine_shutdown();
        font_manager_shutdown();
        canvas2d_shutdown();
        image_loader_shutdown();
        {
            extern Renderer *s_renderer;
            if (s_renderer) { renderer_destroy(s_renderer); s_renderer = NULL; }
        }
        bind_filters_shutdown();
        filters_shutdown();
        js_profiler_stop(profiler);
        js_engine_shutdown(js);
        platform_shutdown(platform);
        error_handler_shutdown();
        return 1;
    }

    if (perf_every > 0) {
        game_loop_set_perf_report(loop, perf_every);
    }
    if (max_frames > 0) {
        extern Renderer *s_renderer;
        game_loop_set_frame_limit(loop, max_frames, screenshot_path, &s_renderer);
        if (probe_expr) game_loop_set_exit_probe(loop, probe_expr);
    }
    if (control_mode) {
        extern Renderer *s_renderer;
        char shim_path[1024];
        snprintf(shim_path, sizeof(shim_path), "%s/control_shim.js", shim_dir);
        if (!js_engine_eval_file(js, shim_path) ||
            !game_loop_enable_control(loop, &s_renderer)) {
            error_handler_log(LOG_ERROR, "Control mode could not be enabled");
        }
    }
    game_loop_run(loop);

    game_loop_destroy(loop);
    video_player_shutdown();
    effekseer_shutdown();
    input_manager_shutdown();
    audio_engine_shutdown();
    font_manager_shutdown();
    canvas2d_shutdown();
    image_loader_shutdown();
    /* Clean up renderer and filter GPU resources before destroying GL context. */
    {
        extern Renderer *s_renderer;
        if (s_renderer) {
            renderer_destroy(s_renderer);
            s_renderer = NULL;
        }
    }
    bind_filters_shutdown();
    filters_shutdown();
    int exit_status = process_exit_status(js);
    js_profiler_stop(profiler);
    js_engine_shutdown(js);
    platform_shutdown(platform);
    error_handler_log(LOG_INFO, "Shutdown complete");
    error_handler_shutdown();
    return exit_status;
}

/** Run services on the process main thread with borrowed CLI arguments.
 * Returns the runtime exit status after JS and graphics teardown. */
int main(int argc, char *argv[])
{
    return runtime_main(argc, argv);
}

#endif /* RMMZ_TEST_DATA_DIR */
