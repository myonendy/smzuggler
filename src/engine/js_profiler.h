/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * The runtime's optional V8 CPU profile (spec section 9, milestone M1).
 *
 * Why: spec section 9 asks M1 for `--cpu-prof`, the V8 counterpart of
 * Chromium's DevTools CPU profiler, so a frame can be attributed to JS
 * functions without an external `perf` run and without JIT symbol maps. The
 * profile is a Chrome DevTools `.cpuprofile` that can be opened directly in
 * the DevTools Performance panel. It stands in for the profile NW.js's
 * Chromium writes when started with `--cpu-prof` and keeps the same file
 * format, but script URLs are relative to the game root as NW.js shows them
 * in DevTools, never absolute host paths. See docs/perf.md, "CPU profiles
 * from the runtime".
 *
 * Rules:
 * - Main thread only: the profiler is created, started, stopped and disposed
 *   on the thread that owns the isolate, before the engine is shut down.
 * - The profiler borrows the runtime only to find the isolate during
 *   js_profiler_start; nothing here keeps a `jsb_val`, a `jsb_ctx` or a
 *   `Local` beyond the call that made it (spec section 15.8).
 * - The handle returned by js_profiler_start is owned by the caller and must
 *   be released with js_profiler_stop, which also writes the file.
 */

#ifndef SMZ_JS_PROFILER_H
#define SMZ_JS_PROFILER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The jsb seam's runtime (src/jsb/jsb.h), declared here so this header stays
   plain C and does not pull in jsb.h. */
struct jsb_runtime;

/* One recording started by js_profiler_start; opaque and caller-owned. */
typedef struct js_profiler js_profiler;

/* Fills `out` with the default profile path for `state_dir`:
   `<state_dir>/profiles/cpu-<pid>-<unix time>.cpuprofile`. Why: each run
   must not overwrite the previous profile, and the state directory keeps
   the game folder read-only (spec section 13). `state_dir` is borrowed, may
   be relative, and is created as needed when the profile is written.
   `out` is caller-owned. Returns false for a NULL or empty argument or when
   the result does not fit in `out_size` bytes (`out` is then ""). Main
   thread (reads the process id and clock). */
bool js_profiler_default_path(const char *state_dir, char *out, size_t out_size);

/* Starts V8's CpuProfiler on `rt`'s isolate and records every JS sample
 * until js_profiler_stop. `path` (borrowed, copied) is where the Chrome
 * DevTools `.cpuprofile` is written on stop; missing parent directories are
 * created. `game_root` (borrowed, copied, may be NULL) is resolved with
 * realpath/_fullpath and stripped from script URLs so the profile names game
 * files as `js/main.js`, not as an absolute host path; if the directory
 * cannot be resolved, pass an absolute path. Returns NULL, after a message
 * on stderr, when the runtime, path or isolate is unusable or a profile is
 * already running. Main thread; the runtime must outlive the returned
 * handle. */
js_profiler *js_profiler_start(struct jsb_runtime *rt, const char *path,
                               const char *game_root);

/* Stops the recording, serializes it to the path given to
 * js_profiler_start, disposes the profiler and frees `profiler` (NULL is
 * ignored). Returns true when the file was written completely; a failed
 * write is reported on stderr and leaves no partial file. Call before
 * js_engine_shutdown. Main thread. */
bool js_profiler_stop(js_profiler *profiler);

#ifdef __cplusplus
}
#endif

#endif /* SMZ_JS_PROFILER_H */
