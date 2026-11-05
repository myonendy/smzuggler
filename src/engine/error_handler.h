/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: remove the former pending-exception adapter.
 * Modified 2026-11-01: add error_handler_report_js_error and
 *   error_handler_on_jsb_error; all reporting uses engine-neutral strings.
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
   to fetch, so this is the engine-neutral entry point for reporting one.
   engine is a borrowed JSEngine* (NULL: log and dialog only); message is the
   thrown value's String() form (NULL: "Unknown error"); stack may be NULL.
   Message and stack are cut at 4096 bytes each for JS and the dialog. The
   listener gets a new Error carrying message and stack, not the thrown
   object (docs/js-binding.md, "Error handler"). Main thread, outside any JS
   call and never from a jsb error handler. */
void error_handler_report_js_error(void *engine, const char *message,
                                   const char *stack, bool fatal);

/* A jsb_error_fn (src/jsb/jsb.h) for jsb_set_error_handler(rt,
   error_handler_on_jsb_error, NULL): exceptions thrown by jsb callbacks
   (audio onended, video events) are logged here, scrubbed, instead of on
   stderr. They are logged only, not passed to window.onerror: a jsb error
   handler must not run JS. Borrows message (NULL: "Unknown error") and stack
   (may be NULL or empty); user is unused. Main thread. Declared without
   jsb.h so this header and error_handler.c have no seam dependency. */
void error_handler_on_jsb_error(const char *message, const char *stack, void *user);

/* Crash recovery: run the DOM shim's __error_handler_autoSave through
   js_engine.h. Returns true when it returned non-zero; false for a NULL
   engine, a missing function or a throw (each logged). Main thread. */
bool error_handler_attempt_auto_save(void *engine);

/* Get the log level name as a string (e.g., "DEBUG", "INFO"). */
const char *error_handler_level_name(LogLevel level);

#endif /* RMMZ_ERROR_HANDLER_H */
