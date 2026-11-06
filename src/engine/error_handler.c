/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: remove former engine migration contracts.
 * Modified 2026-10-30: filter diagnostic home paths before output.
 * Modified 2026-11-01: window.onerror dispatch and auto-save go through
 *   js_engine.h; jsb callback errors are logged here (card M1-T011).
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Engine diagnostics and crash recovery; see docs/privacy.md and
 * docs/js-binding.md ("Error handler").
 * Logging state belongs to the engine thread; callbacks borrow message buffers.
 * JS is reached only through js_engine.h (and called back by jsb through
 * error_handler_on_jsb_error), never by holding JS values,
 * so no engine handle outlives a call (handle rules: docs/js-binding.md, §15.8).
 */

#include "engine/error_handler.h"
#include "engine/js_engine.h"
#include "smzuggler/log_scrub.h"
#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static FILE *s_log_file = NULL;
static LogLevel s_min_level = LOG_DEBUG;
static LogCallback s_callback = NULL;
static void *s_callback_userdata = NULL;

static const char *s_level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

/** Configure engine-thread logging at min_level (LOG_DEBUG through LOG_ERROR).
 * Borrows path only for this call; NULL disables file output. Open failures are
 * scrubbed and reported to stderr; no allocation is returned to the caller.
 */
void error_handler_init(const char *log_file_path, LogLevel min_level)
{
    s_min_level = min_level;

    if (log_file_path) {
        s_log_file = fopen(log_file_path, "a");
        if (!s_log_file) {
            char safe_path[4096];
            log_scrub(log_file_path, safe_path, sizeof(safe_path));
            fprintf(stderr, "[ERROR] Failed to open log file: %s\n", safe_path);
        }
    }
}

/* Closes the log file and forgets the capture callback.
 *
 * Why: the file and callback are process-wide resources; shutdown releases
 * them deterministically instead of relying on exit. The callback itself
 * stays owned by its installer. Main thread; safe to call twice. */
void error_handler_shutdown(void)
{
    if (s_log_file) {
        fclose(s_log_file);
        s_log_file = NULL;
    }
    s_callback = NULL;
    s_callback_userdata = NULL;
}

/* Returns the fixed spelling of `level`, or "UNKNOWN" outside the enum.
 * The result points at static storage and must not be freed. Any thread. */
const char *error_handler_level_name(LogLevel level)
{
    if (level < LOG_DEBUG || level > LOG_ERROR) return "UNKNOWN";
    return s_level_names[level];
}

/* Sets the process-wide minimum level; lower messages are discarded.
 * `min_level` is borrowed by value and kept until changed again. Main
 * thread, where the level is read. */
void error_handler_set_level(LogLevel min_level)
{
    s_min_level = min_level;
}

/* Returns the current minimum level (LOG_DEBUG when never set). Main thread. */
LogLevel error_handler_get_level(void)
{
    return s_min_level;
}

/* Installs `cb` as an additional log sink and `userdata` as its argument.
 * Why: tests and hosts need to observe log lines without scraping stdio.
 * NULL `cb` removes the sink; the installer owns `userdata`, which is
 * borrowed for each call. Main thread. */
void error_handler_set_callback(LogCallback cb, void *userdata)
{
    s_callback = cb;
    s_callback_userdata = userdata;
}

/** Emit a diagnostic at level (LOG_DEBUG through LOG_ERROR) on the engine thread.
 * Borrows printf fmt/arguments for this call; retains no message pointers.
 * Messages truncate at 4095 bytes and every sink receives scrubbed text. Callers
 * must serialize environment changes; output failures follow stdio semantics.
 */
void error_handler_log(LogLevel level, const char *fmt, ...)
{
    if (level < s_min_level) return;

    char msg[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    char safe_msg[sizeof(msg)];
    log_scrub(msg, safe_msg, sizeof(safe_msg));

    time_t now = time(NULL);
    struct tm tm_buf;
#ifdef _WIN32
    struct tm *tm_info = (localtime_s(&tm_buf, &now) == 0) ? &tm_buf : NULL;
#else
    struct tm *tm_info = localtime_r(&now, &tm_buf);
#endif
    char timestamp[32];
    if (tm_info) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(timestamp, sizeof(timestamp), "<unknown time>");
    }

    const char *level_name = error_handler_level_name(level);

    FILE *out = (level >= LOG_WARN) ? stderr : stdout;
    fprintf(out, "[%s] [%s] %s\n", timestamp, level_name, safe_msg);

    if (s_log_file) {
        fprintf(s_log_file, "[%s] [%s] %s\n", timestamp, level_name, safe_msg);
        fflush(s_log_file);
    }

    if (s_callback) {
        s_callback(level, safe_msg, s_callback_userdata);
    }
}

/* Logs at ERROR and, when SDL is initialised, shows a message box.
 *
 * Why: fatal startup errors have no JS or window owner to report through.
 * `title` and `message` are borrowed for the call and may be NULL (they
 * default to "Error" and ""); the text is scrubbed by the logger. Main
 * thread; the SDL call blocks until the user dismisses it. */
void error_handler_show_error_dialog(const char *title, const char *message)
{
    error_handler_log(LOG_ERROR, "%s: %s", title ? title : "Error", message ? message : "");

    if (SDL_WasInit(0) != 0) {
        SDL_ShowSimpleMessageBox(
            SDL_MESSAGEBOX_ERROR,
            title ? title : "Error",
            message ? message : "An unknown error occurred.",
            NULL
        );
    }
}

/* ---- JS errors ---------------------------------------------------------- */

/* Longest message or stack, in bytes, passed on to JS or a dialog; longer text
   is cut to bound diagnostic allocations. */
#define ERROR_TEXT_MAX 4096

/* Calls window.onerror (through the DOM shim's __error_handler_dispatch) with
   an Error rebuilt from %s (message) and %s (stack), both escaped by js_quote.
   No dispatcher (shim not loaded) means nothing to do, as before. */
static const char k_dispatch_fmt[] =
    "(function (m, s) {\n"
    "  if (typeof __error_handler_dispatch !== 'function') return;\n"
    "  var e = new Error(m);\n"
    "  if (s) e.stack = s;\n"
    "  __error_handler_dispatch(m, e);\n"
    "})(\"%s\", \"%s\");\n";

/* Runs the shim's __error_handler_autoSave and answers "missing", "threw:<why>"
   or its result as ToInt32 digits, so C can tell the three cases apart from
   one string. */
static const char k_autosave_src[] =
    "(function () {\n"
    "  if (typeof __error_handler_autoSave !== 'function') return 'missing';\n"
    "  try { return String(__error_handler_autoSave() | 0); }\n"
    "  catch (e) { return 'threw:' + String(e); }\n"
    "})()";

/** Writes at most ERROR_TEXT_MAX bytes of UTF-8 `s` into `out` as the body
 * of a double-quoted JS string literal and NUL-terminates it.
 * Why: errors reach the shim as script source through js_engine.h, so the
 * text must arrive unchanged and must not be able to end the literal. Quote,
 * backslash and control bytes are escaped; other bytes (UTF-8) pass through.
 * `out` needs 6 * ERROR_TEXT_MAX + 1 bytes; neither pointer is kept. A cut
 * may split a UTF-8 sequence, which JS then reads as U+FFFD. Any thread.
 */
static void js_quote(const char *s, char *out)
{
    static const char hex[] = "0123456789abcdef";
    size_t n = 0;
    for (size_t i = 0; s[i] && i < ERROR_TEXT_MAX; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\') {
            out[n++] = '\\';
            out[n++] = (char)c;
        } else if (c < 0x20) {
            memcpy(out + n, "\\u00", 4);
            out[n + 4] = hex[c >> 4];
            out[n + 5] = hex[c & 15];
            n += 6;
        } else {
            out[n++] = (char)c;
        }
    }
    out[n] = '\0';
}

/** Implements the browser's report-an-exception step for errors that reach C:
 * calls window.onerror and then 'error' listeners (WHATWG HTML "report an
 * exception"; compat via dom_shim.js __error_handler_dispatch).
 * Deviation: the listener gets a new Error whose message is the thrown
 * value's String() form and whose stack is the original stack, not the
 * thrown object itself, because exceptions never cross into C as JS values
 * (docs/js-binding.md, "Error handler"). Borrows all arguments. Main thread,
 * outside any JS call on the engine. A failure (out of memory, the script
 * throwing) is logged; nothing is returned.
 */
static void dispatch_to_window(JSEngine *engine, const char *message, const char *stack)
{
    size_t cap = sizeof(k_dispatch_fmt) + 2 * (6 * ERROR_TEXT_MAX + 1);
    char *quoted_msg = malloc(6 * ERROR_TEXT_MAX + 1);
    char *quoted_stack = malloc(6 * ERROR_TEXT_MAX + 1);
    char *src = malloc(cap);
    if (!quoted_msg || !quoted_stack || !src) {
        error_handler_log(LOG_ERROR, "window.onerror dispatch skipped: out of memory");
    } else {
        js_quote(message, quoted_msg);
        js_quote(stack ? stack : "", quoted_stack);
        snprintf(src, cap, k_dispatch_fmt, quoted_msg, quoted_stack);
        if (!js_engine_eval(engine, src, "error_handler_dispatch.js")) {
            error_handler_log(LOG_WARN, "window.onerror dispatch threw (reported above)");
        }
    }
    free(quoted_msg);
    free(quoted_stack);
    free(src);
}

/** Logs a JS error's message and, when present, its stack at LOG_ERROR.
 * Why: one format for every way an error reaches C (engine-reported
 * exception, jsb callback, string report). Borrows both strings; a NULL or
 * empty stack is not logged. Main thread.
 */
static void log_js_error(const char *message, const char *stack)
{
    error_handler_log(LOG_ERROR, "JS Exception: %s", message);
    if (stack && stack[0]) {
        error_handler_log(LOG_ERROR, "Stack trace:\n%s", stack);
    }
}

/** Reports a JS error given as text: log, window.onerror, then a dialog if
 * fatal. Why: the engine-neutral entry point; V8 never leaves a pending
 * exception for C to fetch, so callers pass String(exception) and its stack.
 * `engine` (a JSEngine*, may be NULL to skip dispatch), `message` (NULL means
 * "Unknown error") and `stack` (may be NULL) are borrowed. Main thread, not
 * from inside a jsb error handler (dispatch runs JS). Errors are logged.
 */
void error_handler_report_js_error(void *engine, const char *message,
                                   const char *stack, bool fatal)
{
    const char *msg = message ? message : "Unknown error";
    log_js_error(msg, stack);
    if (engine) dispatch_to_window((JSEngine *)engine, msg, stack);
    if (!fatal) return;

    char dialog_msg[2 * ERROR_TEXT_MAX + 32];
    if (stack && stack[0]) {
        snprintf(dialog_msg, sizeof(dialog_msg), "%.*s\n\nStack trace:\n%.*s",
                 ERROR_TEXT_MAX, msg, ERROR_TEXT_MAX, stack);
    } else {
        snprintf(dialog_msg, sizeof(dialog_msg), "%.*s", ERROR_TEXT_MAX, msg);
    }
    error_handler_show_error_dialog("RPG Maker MZ - JavaScript Error", dialog_msg);
}

/** A jsb_error_fn (jsb.h) that logs exceptions thrown by jsb callbacks (audio
 * onended, video events); install it with jsb_set_error_handler(rt,
 * error_handler_on_jsb_error, NULL). Why: jsb catches them before they reach
 * C and would print them unscrubbed to stderr. It only logs: a jsb error
 * handler must not run JS on its runtime, so window.onerror is not called
 * (docs/js-binding.md). Borrows both strings for the call; `user` is unused.
 * Main thread, called by jsb before jsb_callback_call returns.
 */
void error_handler_on_jsb_error(const char *message, const char *stack, void *user)
{
    (void)user;
    log_js_error(message ? message : "Unknown error", stack);
}

/** Crash recovery: runs the shim's __error_handler_autoSave (DataManager save
 * to slot 0, dom_shim.js) through js_engine.h. Returns true when it returned
 * a non-zero value; false for a NULL `engine` (borrowed JSEngine*), a missing
 * function or a throw, each logged. Main thread, outside any JS call.
 */
bool error_handler_attempt_auto_save(void *engine_ptr)
{
    JSEngine *engine = (JSEngine *)engine_ptr;
    if (!engine) return false;

    error_handler_log(LOG_INFO, "Attempting crash recovery auto-save...");

    char *out = js_engine_eval_string(engine, k_autosave_src, "error_handler_autosave.js");
    bool saved = false;
    if (!out) {
        error_handler_log(LOG_ERROR, "Auto-save failed: the auto-save script did not run");
    } else if (strcmp(out, "missing") == 0) {
        error_handler_log(LOG_WARN, "Auto-save function not available");
    } else if (strncmp(out, "threw:", 6) == 0) {
        error_handler_log(LOG_ERROR, "Auto-save failed: %s", out + 6);
    } else {
        saved = strcmp(out, "0") != 0;
    }
    js_engine_free_string(out);

    if (saved) {
        error_handler_log(LOG_INFO, "Auto-save succeeded");
    }
    return saved;
}

