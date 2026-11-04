/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-01: add js_engine_get_jsb; js_engine_get_context is
 * QuickJS-mode only. The API has two implementations: js_engine.c (QuickJS)
 * and js_engine_v8.cc (SMZ_JS_ENGINE=v8). See docs/js-binding.md.
 * Modified 2026-11-04: js_engine_get_jsb is non-NULL in quickjs mode too.
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

/* Create the JS engine (QuickJS, or V8 in v8 mode) with one context.
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

/* Like js_engine_eval_file(), but caches compiled bytecode in a .jsc file next
   to the source (invalidated by source mtime). Falls back to plain eval. */
bool js_engine_eval_file_cached(JSEngine *engine, const char *filepath);

/* Opt-in read-only game folder (--data-dir): put .jsc caches under cache_dir,
   keyed by the script's path relative to game_root (a script outside the game
   root is keyed by its whole path), instead of next to the script. Missing
   directories are created on first use. Both strings are copied; game_root may
   be NULL. A NULL or empty cache_dir restores the default. Returns false on
   allocation failure (the default is then in effect). */
bool js_engine_set_cache_dir(JSEngine *engine, const char *cache_dir, const char *game_root);

/* Fill out with the platform cache dir for app's bytecode cache:
   $XDG_CACHE_HOME/<app>/jsc, or $HOME/.cache/<app>/jsc when XDG_CACHE_HOME is
   unset. Linux only (no SDL here); elsewhere returns false and the caller
   should use SDL_GetPrefPath. Returns false if the result does not fit. */
bool js_engine_default_cache_dir(const char *app, char *out, size_t out_size);

/* Drain the job queue (promise callbacks) and report unhandled rejections.
   Returns the number of jobs executed, or -1 on error. */
int js_engine_execute_pending_jobs(JSEngine *engine);

/* Route console.log/warn/error to a callback instead of stdout/stderr. */
void js_engine_set_console_callback(JSEngine *engine, JSConsoleCallback cb, void *userdata);

/* The engine's jsb runtime, through which ported bindings define their
   modules in the same global object the game's scripts run in.
   Why: it replaces js_engine_get_context, which exposes QuickJS, for code
   that must work in both engine modes. Borrowed: owned by the engine and
   valid until js_engine_shutdown. Main thread only.
   It is never NULL for a live engine, and NULL for a NULL engine. In
   quickjs mode it is the engine's own JSContext adopted by the
   transitional QuickJS jsb backend (src/jsb/jsb_adopt.h, DEC-SETUP-008,
   removed by M1-T027); see docs/js-binding.md, "Adoption". */
struct jsb_runtime *js_engine_get_jsb(JSEngine *engine);

#ifndef SMZ_JS_ENGINE_V8
/* Get the underlying QuickJS context (NULL if engine is NULL).
   QuickJS mode only (not declared when SMZ_JS_ENGINE_V8 is defined): it is
   the transitional path for code not yet ported to jsb, removed by M1-T027. */
struct JSContext *js_engine_get_context(JSEngine *engine);
#endif

/* Debug aid: interrupt JS that runs longer than limit_ms since the last
   js_engine_watchdog_arm() call, raising an uncatchable InternalError whose
   stack trace shows where the script was stuck. 0 disables (the default).
   In v8 mode the limit is only recorded until card M1-T008 adds the V8
   watchdog; nothing is interrupted there yet. */
void js_engine_set_watchdog(JSEngine *engine, unsigned limit_ms);

/* Restart the watchdog clock. Eval entry points arm it themselves; the game
   loop arms it once per frame. */
void js_engine_watchdog_arm(JSEngine *engine);

#ifdef __cplusplus
}
#endif

#endif /* RMMZ_JS_ENGINE_H */
