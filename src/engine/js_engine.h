/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: expose only the V8 engine and host code cache.
 * Runtime API used by the loader and loop; see docs/js-binding.md.
 * All calls run on the main thread; the engine owns its jsb runtime.
 */

#ifndef RMMZ_JS_ENGINE_H
#define RMMZ_JS_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {  /* js_engine_v8.cc implements this API in C++ */
#endif

typedef struct JSEngine JSEngine;

/* The jsb seam's engine instance (src/jsb/jsb.h); declared here so this
   header does not pull in jsb.h. */
struct jsb_runtime;

/* Callback signature for console output capture (used by tests). */
typedef void (*JSConsoleCallback)(const char *level, const char *message, void *userdata);

/* Create the V8 engine with one context.
   Returns NULL on failure. */
JSEngine *js_engine_init(void);

/* Shut down and free all engine resources. Safe to call with NULL. */
void js_engine_shutdown(JSEngine *engine);

/* Evaluate a JS string. Returns false on error (the exception is printed). */
bool js_engine_eval(JSEngine *engine, const char *script, const char *filename);

/* Evaluate a JS string and return its result as a string, or NULL on error.
   Free the result with js_engine_free_string(). */
char *js_engine_eval_string(JSEngine *engine, const char *script, const char *filename);

/* Free a string returned by js_engine_eval_string(). */
void js_engine_free_string(char *str);

/* Load and evaluate a JS file from disk. Returns true on success. */
bool js_engine_eval_file(JSEngine *engine, const char *filepath);

/* Evaluate with V8 code cached outside the game folder, keyed by source,
   V8 version and flags. Cache failures fall back to compilation. Main thread. */
bool js_engine_eval_file_cached(JSEngine *engine, const char *filepath);

/* Copy nonempty cache_dir as the host override and game_root for source names.
   NULL/empty cache_dir resets both; NULL game_root is allowed. Main thread;
   returns false for a NULL engine. Never writes game files. */
bool js_engine_set_cache_dir(JSEngine *engine, const char *cache_dir, const char *game_root);

/* Fill out with the platform cache dir for app's legacy host cache directory:
   $XDG_CACHE_HOME/<app>/jsc, or $HOME/.cache/<app>/jsc when XDG_CACHE_HOME is
   unset. Linux only (no SDL here); elsewhere returns false and the caller
   should use SDL_GetPrefPath. Returns false if the result does not fit. */
bool js_engine_default_cache_dir(const char *app, char *out, size_t out_size);

/* Drain the job queue (promise callbacks) and report unhandled rejections.
   Returns the number of jobs executed, or -1 on error. */
int js_engine_execute_pending_jobs(JSEngine *engine);

/* Route console.log/warn/error to a callback instead of stdout/stderr. */
void js_engine_set_console_callback(JSEngine *engine, JSConsoleCallback cb, void *userdata);

/* Borrow the engine's runtime to define modules in the game's global object.
   Main thread only; valid until shutdown, NULL only for a NULL engine.
   Values follow the handle lifetime rules in docs/js-binding.md. */
struct jsb_runtime *js_engine_get_jsb(JSEngine *engine);

/* Debug aid: interrupt JS that runs longer than limit_ms since the last
   js_engine_watchdog_arm() call, raising an uncatchable InternalError whose
   stack trace shows where the script was stuck. 0 disables (the default).
   The watchdog thread terminates execution; only the main thread enters V8. */
void js_engine_set_watchdog(JSEngine *engine, unsigned limit_ms);

/* Restart the watchdog clock. Eval entry points arm it themselves; the game
   loop arms it once per frame. */
void js_engine_watchdog_arm(JSEngine *engine);

#ifdef __cplusplus
}
#endif

#endif /* RMMZ_JS_ENGINE_H */
