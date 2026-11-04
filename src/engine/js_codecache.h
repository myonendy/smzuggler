/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * The on-disk store of V8 code caches (spec sections 4.1 item 4 and 5.6).
 *
 * Why: Outsider kept a QuickJS `.jsc` next to each script, which writes into
 * the game folder. In v8 mode a script's compiled code is kept instead as
 * `<dir>/<sha256>.bin` under `$XDG_CACHE_HOME/smzuggler-rt/v8cache/`, so the
 * game folder can be read-only. It stands in for Chromium's code cache, which
 * NW.js keeps in its profile directory, never beside the page. The key hashes
 * the source together with the V8 version and V8's flag hash, so a source
 * edit, a V8 upgrade or a flag change never reaches a stale cache.
 * This file holds no V8 types: js_engine_v8.cc compiles and consumes the
 * caches, and this module only names, reads, writes and drops them. See
 * docs/js-binding.md, "Code cache", and docs/data-dirs.md.
 *
 * Rules:
 * - Main thread only (the counters are plain integers).
 * - Nothing here ever writes outside the directory it is given.
 * - Buffers returned by js_codecache_read are freed with js_codecache_free.
 */

#ifndef SMZ_JS_CODECACHE_H
#define SMZ_JS_CODECACHE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Hex digits of a cache key (a sha256), without the terminating NUL. */
#define JS_CODECACHE_KEY_LEN 64

/* Computes the cache key of a script: the lowercase hex sha256 of `source`
   (`len` bytes, any content), a NUL, `engine_version` (V8's version string),
   a NUL, then `flag_hash` as 4 little-endian bytes (V8's
   ScriptCompiler::CachedDataVersionTag, which changes with the flags).
   Why: one name per (source, V8 build, flags), so a cache can never be
   consumed with a different source or by a different V8 configuration.
   `out` receives JS_CODECACHE_KEY_LEN digits and a NUL. Returns false only
   for NULL arguments (`source` may be NULL when `len` is 0). Any thread. */
bool js_codecache_key(const char *source, size_t len, const char *engine_version,
                      uint32_t flag_hash, char out[JS_CODECACHE_KEY_LEN + 1]);

/* Fills `out` with the default code cache directory:
   $XDG_CACHE_HOME/smzuggler-rt/v8cache, or $HOME/.cache/smzuggler-rt/v8cache
   when XDG_CACHE_HOME is unset, empty or relative (XDG base directory spec).
   Linux only; elsewhere returns false, as js_engine_default_cache_dir does,
   and the host must name a directory itself. Returns false when no base is
   usable or the result does not fit in `out_size` bytes (`out` is then ""). */
bool js_codecache_default_dir(char *out, size_t out_size);

/* Fills `out` with `<dir>/<key>.bin`. Returns false for a NULL or empty
   argument, or when the path does not fit in `out_size` bytes. */
bool js_codecache_path(const char *dir, const char *key, char *out, size_t out_size);

/* Reads the cache file at `path` into a new buffer. On success `*data` (free
   with js_codecache_free) and `*len` are set and true is returned; a missing,
   empty or unreadable file returns false and sets `*data` to NULL. */
bool js_codecache_read(const char *path, uint8_t **data, size_t *len);

/* Frees a buffer from js_codecache_read. NULL is ignored. */
void js_codecache_free(uint8_t *data);

/* Stores `len` bytes at `path`, creating missing parent directories. The data
   goes to a temporary file in the same directory that is then renamed over
   `path`, so a crash or a second runtime never leaves a half-written cache
   behind. Returns false, with a message on stderr naming the path, on any
   I/O error; the caller then just runs uncached. */
bool js_codecache_write(const char *path, const void *data, size_t len);

/* Deletes the cache file at `path` (V8 rejected it). Returns true when the
   file is gone afterwards, including when it never existed. */
bool js_codecache_discard(const char *path);

/* What happened to one cache lookup, as counted by js_codecache_note. */
typedef enum {
    JS_CODECACHE_PRODUCED, /* a new cache was compiled and written */
    JS_CODECACHE_ACCEPTED, /* V8 consumed a cache file (cachedData accepted) */
    JS_CODECACHE_REJECTED  /* V8 refused a cache file; it was deleted */
} js_codecache_event;

/* Counters of every lookup since process start (or the last reset). */
typedef struct {
    unsigned produced;
    unsigned accepted;
    unsigned rejected;
} js_codecache_stats;

/* Counts one event. Called by js_engine_v8.cc. Why: whether V8 accepted a
   cache is invisible from JS, so tests and the debug log read it here. */
void js_codecache_note(js_codecache_event event);

/* Copies the counters to `*out` (ignored when NULL). */
void js_codecache_get_stats(js_codecache_stats *out);

/* Sets every counter to 0. */
void js_codecache_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif /* SMZ_JS_CODECACHE_H */
