/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-01: add error_handler_report_js_error and
 *   error_handler_on_jsb_error; report_js_exception becomes a QuickJS-only
 *   inline (card M1-T011).
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 */

#ifndef RMMZ_ERROR_HANDLER_H
#define RMMZ_ERROR_HANDLER_H

#include <stdbool.h>
#include <stddef.h>

/* Log levels, ordered by severity. */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3
} LogLevel;

/* Callback signature for log output capture (used by tests). */
typedef void (*LogCallback)(LogLevel level, const char *message, void *userdata);

/* Initialize logging. log_file_path may be NULL to disable file output;
   messages below min_level are discarded. */
void error_handler_init(const char *log_file_path, LogLevel min_level);

/* Shut down the error handler and close any open log file. */
void error_handler_shutdown(void);

/* Log a message at the given level. Supports printf-style format strings. */
void error_handler_log(LogLevel level, const char *fmt, ...);

/* Set the minimum log level. Messages below this level are discarded. */
void error_handler_set_level(LogLevel min_level);

/* Get the current minimum log level. */
LogLevel error_handler_get_level(void);

/* Set a log callback for capturing output (primarily for testing).
   When set, log output goes to the callback in addition to file/console. */
void error_handler_set_callback(LogCallback cb, void *userdata);

/* Log the message at ERROR level and show an SDL message box (if SDL is up). */
void error_handler_show_error_dialog(const char *title, const char *message);

/* Report a JS error given as text: log message and stack (scrubbed), call
   window.onerror and 'error' listeners through the DOM shim, and show an
   error dialog when fatal is true. Why: V8 leaves no pending exception for C
   to fetch, so this is the entry point that works in both engine modes.
   engine is a borrowed JSEngine* (NULL: log and dialog only); message is the
   thrown value's String() form (NULL: "Unknown error"); stack may be NULL.
   Message and stack are cut at 4096 bytes each for JS and the dialog. The
   listener gets a new Error carrying message and stack, not the thrown
   object (docs/js-binding.md, "Error handler"). Main thread, outside any JS
   call and never from a jsb error handler. */
void error_handler_report_js_error(void *engine, const char *message,
                                   const char *stack, bool fatal);

#ifndef SMZ_JS_ENGINE_V8
#include "engine/js_engine.h"
#include <quickjs.h>

/* Take QuickJS's pending exception and report it as
   error_handler_report_js_error does. engine is a borrowed JSEngine* (NULL
   does nothing). The exception is consumed, also when it cannot be converted
   to a string. Main thread.
   Why it is an inline here: V8 has no pending exception, so no engine-neutral
   code calls this; only the QuickJS-era tests (tests/test_error_handler.c)
   do. Keeping it out of error_handler.c keeps that file free of QuickJS
   (DEC-M1-T011). QuickJS mode only (not declared when SMZ_JS_ENGINE_V8 is
   defined); removed with QuickJS by M1-T027. */
static inline void error_handler_report_js_exception(void *engine, bool fatal)
{
    JSContext *ctx = engine ? js_engine_get_context((JSEngine *)engine) : NULL;
    if (!ctx) return;

    JSValue exc = JS_GetException(ctx);
    const char *msg = JS_ToCString(ctx, exc);
    const char *stack = NULL;
    if (JS_IsError(ctx, exc)) {
        JSValue stack_val = JS_GetPropertyStr(ctx, exc, "stack");
        if (!JS_IsUndefined(stack_val)) stack = JS_ToCString(ctx, stack_val);
        JS_FreeValue(ctx, stack_val);
    }
    JS_FreeValue(ctx, exc);
    /* A failed conversion throws again; drop that so dispatch can run. */
    JS_FreeValue(ctx, JS_GetException(ctx));

    error_handler_report_js_error(engine, msg, stack, fatal);
    if (msg) JS_FreeCString(ctx, msg);
    if (stack) JS_FreeCString(ctx, stack);
}
#endif

/* A jsb_error_fn (src/jsb/jsb.h) for jsb_set_error_handler(rt,
   error_handler_on_jsb_error, NULL): exceptions thrown by jsb callbacks
   (audio onended, video events) are logged here, scrubbed, instead of on
   stderr. They are logged only, not passed to window.onerror: a jsb error
   handler must not run JS. Borrows message (NULL: "Unknown error") and stack
   (may be NULL or empty); user is unused. Main thread. Declared without
   jsb.h so this header and error_handler.c build in both engine modes. */
void error_handler_on_jsb_error(const char *message, const char *stack, void *user);

/* Crash recovery: run the DOM shim's __error_handler_autoSave through
   js_engine.h. Returns true when it returned non-zero; false for a NULL
   engine, a missing function or a throw (each logged). Main thread. */
bool error_handler_attempt_auto_save(void *engine);

/* Get the log level name as a string (e.g., "DEBUG", "INFO"). */
const char *error_handler_level_name(LogLevel level);

#endif /* RMMZ_ERROR_HANDLER_H */
