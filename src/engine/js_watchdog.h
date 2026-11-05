/* SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: remove former engine migration contracts.

   The script watchdog's timer thread (spec section 4.1, item 3).

   Why: the former watchdog was an interrupt handler polled while
   JS ran. V8 has no such poll; a script stuck in `while(true){}` can only be
   stopped from another thread, as Chromium's "page unresponsive" check and
   Node's `vm` timeout do. This file is that thread: it knows nothing about
   V8, it only waits for a deadline and calls the owner back once per arm.
   js_engine_v8.cc turns the call into Isolate::RequestInterrupt and
   Isolate::TerminateExecution. See docs/js-binding.md, "Watchdog".

   Rules:
   - One thread per watchdog, created by js_watchdog_start and joined by
     js_watchdog_stop. A limit of 0 never creates one (the engine checks).
   - js_watchdog_start, js_watchdog_arm and js_watchdog_stop are called by
     the owner's thread (the main thread); js_watchdog_generation may be
     called from any thread. The fire callback runs on the watchdog thread.
   - No V8 handles are kept here (spec section 15.8). */

#ifndef SMZ_JS_WATCHDOG_H
#define SMZ_JS_WATCHDOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* An opaque watchdog: one timer thread with its limit and arm state. */
typedef struct js_watchdog js_watchdog;

/* Called on the watchdog thread when `generation` (the value returned by the
   js_watchdog_arm call that started the clock) has been armed for longer
   than the limit. Called at most once per generation. It must be fast and
   thread-safe (for V8: Isolate::RequestInterrupt); `user` is the pointer
   given to js_watchdog_start. */
typedef void (*js_watchdog_fire_fn)(uint64_t generation, void *user);

/* Starts the timer thread for a limit of `limit_ms` milliseconds (> 0).
   Nothing fires until js_watchdog_arm. `user` is passed back to `fire` and
   not owned. Returns NULL (with a message on stderr) if limit_ms is 0, fire
   is NULL or the thread cannot be created. The caller stops the watchdog
   with js_watchdog_stop, which frees it. Main thread. */
js_watchdog *js_watchdog_start(unsigned limit_ms, js_watchdog_fire_fn fire, void *user);

/* Restarts the clock: the limit now counts from this call, and any earlier
   generation will not fire. Returns the new generation (>= 1). Main thread;
   NULL is ignored and returns 0. */
uint64_t js_watchdog_arm(js_watchdog *wd);

/* The generation of the latest js_watchdog_arm (0 before the first, or for
   NULL). Lets the owner drop a fire that a later arm made stale. Any thread. */
uint64_t js_watchdog_generation(js_watchdog *wd);

/* Stops and joins the thread and frees `wd` (NULL is ignored). After it
   returns `fire` is never called again. Main thread. */
void js_watchdog_stop(js_watchdog *wd);

/* Number of watchdog threads alive in the process right now, counted from
   js_watchdog_start until the thread exits inside js_watchdog_stop. For
   tests and diagnostics: it proves that no thread exists while the limit is off. Any thread. */
int js_watchdog_live_threads(void);

#ifdef __cplusplus
}
#endif

#endif /* SMZ_JS_WATCHDOG_H */
