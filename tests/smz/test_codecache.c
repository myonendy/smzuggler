/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * The V8 code cache in the cache dir (card M1-T012, spec sections 4.1 item 4
 * and 5.6).
 *
 * Why: v8 mode replaces Outsider's `.jsc`-beside-the-script with V8 code
 * caches named by sha256(source, V8 version, flag hash) under
 * $XDG_CACHE_HOME/smzuggler-rt/v8cache/, so the game folder stays read-only
 * and a stale cache can never be consumed. In both engine modes this checks
 * the store (src/engine/js_codecache.h): the key, the directory, the file
 * round trip and the counters. In v8 mode on Linux it also drives
 * js_engine_eval_file_cached through a scratch XDG_CACHE_HOME: the first
 * load creates the file, the second consumes it, a damaged file is rejected
 * and replaced, and nothing is written next to the script. Checks use
 * assert() and the fragment compiles with NDEBUG undefined.
 */

#include "engine/js_codecache.h"
#include "test_paths.h"

#if defined(SMZ_JS_ENGINE_V8) && defined(__linux__)
#include "engine/js_engine.h"
#include <dirent.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Prints what the next assert checks, so a failing run names it even when
   the abort message is cut off. */
static void note(const char *what)
{
    printf("  check: %s\n", what);
    fflush(stdout);
}

/* Writes `len` bytes to `path`; returns 1 on success. */
static int write_bytes(const char *path, const void *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(data, 1, len, f) == len;
    return fclose(f) == 0 && ok;
}

/* The key is the sha256 of source, NUL, version, NUL, LE flag hash. The
   expected digests were computed independently (Python hashlib). */
static void test_key(void)
{
    char a[JS_CODECACHE_KEY_LEN + 1], b[JS_CODECACHE_KEY_LEN + 1];
    note("key of a short script matches sha256");
    assert(js_codecache_key("abc", 3, "13.6.233.8", 0x12345678u, a));
    assert(strcmp(a, "ed401201521c9065de7256c4170e0d0c18e57272ff62c80aee895ca697a2de28") == 0);

    char *big = malloc(1000);
    assert(big != NULL);
    memset(big, 'x', 1000);
    note("key of a multi-block script matches sha256");
    assert(js_codecache_key(big, 1000, "v", 0, b));
    assert(strcmp(b, "d22917052f56c894b5cbaf211f55c1ff70ff70718716765abec15137c647fd79") == 0);

    note("changing the source changes the key");
    big[999] = 'y';
    assert(js_codecache_key(big, 1000, "v", 0, a));
    assert(strcmp(a, b) != 0);
    big[999] = 'x';
    note("changing the version tag changes the key");
    assert(js_codecache_key(big, 1000, "w", 0, a));
    assert(strcmp(a, b) != 0);
    note("changing the flag hash changes the key");
    assert(js_codecache_key(big, 1000, "v", 1, a));
    assert(strcmp(a, b) != 0);
    note("source and version cannot trade bytes");
    assert(js_codecache_key("ab", 2, "c", 0, a) && js_codecache_key("a", 1, "bc", 0, b));
    assert(strcmp(a, b) != 0);
    free(big);

    note("NULL arguments are refused");
    assert(!js_codecache_key(NULL, 1, "v", 0, a));
    assert(!js_codecache_key("a", 1, NULL, 0, a));
    assert(js_codecache_key(NULL, 0, "v", 0, a));
}

/* <dir>/<key>.bin, with trailing separators dropped and overflow refused. */
static void test_path(void)
{
    char out[64], tiny[8];
    note("path is <dir>/<key>.bin");
    assert(js_codecache_path("/c/v8cache/", "k1", out, sizeof out));
    assert(strcmp(out, "/c/v8cache/k1.bin") == 0);
    note("a path that does not fit is refused");
    assert(!js_codecache_path("/c/v8cache", "k1", tiny, sizeof tiny));
    assert(tiny[0] == '\0');
    assert(!js_codecache_path("", "k1", out, sizeof out));
}

#if defined(__linux__) && !defined(__ANDROID__)
/* $XDG_CACHE_HOME/smzuggler-rt/v8cache, else $HOME/.cache/...; a relative
   XDG_CACHE_HOME is ignored as the XDG spec requires. */
static void test_default_dir(void)
{
    char out[TEST_PATH_MAX];
    const char *home = getenv("HOME");
    char saved_home[TEST_PATH_MAX];
    snprintf(saved_home, sizeof saved_home, "%s", home ? home : "");
    setenv("XDG_CACHE_HOME", "/xdg/cache/", 1);
    note("default dir is under XDG_CACHE_HOME");
    assert(js_codecache_default_dir(out, sizeof out));
    assert(strcmp(out, "/xdg/cache/smzuggler-rt/v8cache") == 0);
    setenv("XDG_CACHE_HOME", "rel/cache", 1);
    setenv("HOME", "/h", 1);
    note("a relative XDG_CACHE_HOME falls back to HOME/.cache");
    assert(js_codecache_default_dir(out, sizeof out));
    assert(strcmp(out, "/h/.cache/smzuggler-rt/v8cache") == 0);
    note("too small a buffer is refused");
    assert(!js_codecache_default_dir(out, 8) && out[0] == '\0');
    setenv("HOME", saved_home, 1);
    unsetenv("XDG_CACHE_HOME");
}
#endif

/* Write creates the directory, read returns the bytes, discard removes. */
static void test_file_round_trip(void)
{
    char dir[TEST_PATH_MAX], path[TEST_PATH_MAX];
    snprintf(dir, sizeof dir, "%s", test_tmp_path("smz_codecache_store"));
    test_rm_rf(dir);
    assert(js_codecache_path(dir, "deadbeef", path, sizeof path));
    uint8_t *data = (uint8_t *)1;
    size_t len = 7;
    note("a missing file reads as no cache");
    assert(!js_codecache_read(path, &data, &len) && data == NULL && len == 0);
    note("write creates the missing directory");
    assert(js_codecache_write(path, "\x01\x02\x03\x00\x05", 5));
    assert(test_path_is_dir(dir));
    note("read returns the written bytes");
    assert(js_codecache_read(path, &data, &len));
    assert(len == 5 && memcmp(data, "\x01\x02\x03\x00\x05", 5) == 0);
    js_codecache_free(data);
    note("a rewrite replaces the file");
    assert(js_codecache_write(path, "zz", 2));
    assert(js_codecache_read(path, &data, &len) && len == 2 && memcmp(data, "zz", 2) == 0);
    js_codecache_free(data);
    note("discard removes the file, and is true when it is already gone");
    assert(js_codecache_discard(path));
    assert(!js_codecache_read(path, &data, &len));
    assert(js_codecache_discard(path));
    test_rm_rf(dir);
}

/* The counters count each event and reset to zero. */
static void test_stats(void)
{
    js_codecache_stats st;
    js_codecache_reset_stats();
    js_codecache_note(JS_CODECACHE_PRODUCED);
    js_codecache_note(JS_CODECACHE_ACCEPTED);
    js_codecache_note(JS_CODECACHE_ACCEPTED);
    js_codecache_note(JS_CODECACHE_REJECTED);
    js_codecache_get_stats(&st);
    note("counters count each event");
    assert(st.produced == 1 && st.accepted == 2 && st.rejected == 1);
    js_codecache_reset_stats();
    js_codecache_get_stats(&st);
    note("reset clears the counters");
    assert(st.produced == 0 && st.accepted == 0 && st.rejected == 0);
}

#if defined(SMZ_JS_ENGINE_V8) && defined(__linux__)
/* Counts the entries of `dir` other than "." and "..", and copies the name
   of the last one ending in `.bin` to `bin` (when not NULL). */
static int count_entries(const char *dir, char *bin, size_t bin_size)
{
    DIR *d = opendir(dir);
    if (!d) return 0;
    int n = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        n++;
        size_t len = strlen(e->d_name);
        if (bin && len > 4 && strcmp(e->d_name + len - 4, ".bin") == 0)
            snprintf(bin, bin_size, "%s/%s", dir, e->d_name);
    }
    closedir(d);
    return n;
}

/* Loads `script` through the cache in a fresh engine; returns 1 when the
   script ran and set cacheValue to `want`. */
static int load_gives(const char *script, const char *want)
{
    JSEngine *engine = js_engine_init();
    if (!engine) return 0;
    int ok = 0;
    if (js_engine_eval_file_cached(engine, script)) {
        char *v = js_engine_eval_string(engine, "String(cacheValue)", "test");
        ok = v && strcmp(v, want) == 0;
        js_engine_free_string(v);
    }
    js_engine_shutdown(engine);
    return ok;
}

/* Asserts the counters equal the given values. */
static void expect_stats(unsigned produced, unsigned accepted, unsigned rejected)
{
    js_codecache_stats st;
    js_codecache_get_stats(&st);
    printf("  stats: produced %u accepted %u rejected %u\n", st.produced, st.accepted,
           st.rejected);
    assert(st.produced == produced && st.accepted == accepted && st.rejected == rejected);
}

/* The engine path: produce, consume, reject and replace, rekey on edit,
   all under XDG_CACHE_HOME and never beside the script. */
static void test_engine_cache(void)
{
    char xdg[TEST_PATH_MAX], cache[TEST_PATH_MAX], game[TEST_PATH_MAX];
    char script[TEST_PATH_MAX], bin[TEST_PATH_MAX] = "";
    snprintf(xdg, sizeof xdg, "%s", test_tmp_path("smz_codecache_xdg"));
    snprintf(game, sizeof game, "%s", test_tmp_path("smz_codecache_game"));
    snprintf(cache, sizeof cache, "%s/smzuggler-rt/v8cache", xdg);
    snprintf(script, sizeof script, "%s/A.js", game);
    test_rm_rf(xdg);
    test_rm_rf(game);
    const char *first = "var cacheValue = 'first';\nfunction f() { return 1; }\nf();\n";
    assert(test_mkdir_p(game) && write_bytes(script, first, strlen(first)));
    setenv("XDG_CACHE_HOME", xdg, 1);
    js_codecache_reset_stats();

    note("first load runs the script and creates one cache file");
    assert(load_gives(script, "first"));
    expect_stats(1, 0, 0);
    assert(count_entries(cache, bin, sizeof bin) == 1 && bin[0]);
    note("nothing is written next to the script");
    assert(count_entries(game, NULL, 0) == 1);

    note("second load consumes the cache (cachedData accepted)");
    assert(load_gives(script, "first"));
    expect_stats(1, 1, 0);

    note("a corrupted file is rejected, deleted and replaced");
    uint8_t junk[64];
    memset(junk, 0xab, sizeof junk);
    assert(write_bytes(bin, junk, sizeof junk));
    assert(load_gives(script, "first"));
    expect_stats(2, 1, 1);
    uint8_t *data = NULL;
    size_t len = 0;
    assert(js_codecache_read(bin, &data, &len));
    assert(!(len == sizeof junk && memcmp(data, junk, len) == 0));
    js_codecache_free(data);
    note("the replacement is accepted");
    assert(load_gives(script, "first"));
    expect_stats(2, 2, 1);

    note("an edited source gets its own key and file");
    const char *second = "var cacheValue = 'second';\n";
    assert(write_bytes(script, second, strlen(second)));
    assert(load_gives(script, "second"));
    expect_stats(3, 2, 1);
    assert(count_entries(cache, NULL, 0) == 2);
    note("nothing is written next to the edited script either");
    assert(count_entries(game, NULL, 0) == 1);

    unsetenv("XDG_CACHE_HOME");
    test_rm_rf(xdg);
    test_rm_rf(game);
}
#endif

/* Runs every check; an assert aborts on the first failure. */
int main(void)
{
    printf("test_codecache\n");
    test_key();
    test_path();
#if defined(__linux__) && !defined(__ANDROID__)
    test_default_dir();
#endif
    test_file_round_trip();
    test_stats();
#if defined(SMZ_JS_ENGINE_V8) && defined(__linux__)
    test_engine_cache();
#endif
    printf("test_codecache: all checks passed\n");
    return 0;
}
