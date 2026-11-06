/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * --js-flags validation, pass-through and code-cache sensitivity (card
 * M1-T029, spec sections 4.1 and 6).
 *
 * Why: a profiling run must be able to enable V8's perf, tick and GC traces,
 * while an unknown switch has to stop the process with status 2 instead of
 * reaching V8. The flag hash must also reach the code-cache key, or a cache
 * written under one profiler configuration could be consumed under another.
 * This test links src/main.c with RMMZ_TEST_DATA_DIR for the parser and drives
 * child processes of itself so each one starts V8 with one fixed flag set;
 * V8 freezes its flags after the first isolate. It writes only under the test
 * temp root. Checks use assert() and the fragment keeps NDEBUG undefined.
 */

#include "engine/js_engine.h"
#include "test_paths.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <process.h>
#else
#include <dirent.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/* src/main.c owns the allow-list parser; it is compiled here without main().
   `out` is caller-owned and receives the last --js-flags value. Returns 0 when
   absent or valid, 2 for an unknown or malformed flag. */
int smz_js_flags_parse(int argc, char **argv, char *out, size_t out_size);

/* src/engine/js_engine_v8.cc stores the validated value for js_engine_init.
   NULL or "" clears it; false once V8 has started. */
bool js_engine_set_js_flags(const char *flags);

/* Path of this executable, used to spawn a fresh V8 for one flag set. */
static const char *g_self;

/* Prints what the next assert checks, so a failing run names it even when
   the abort message is cut off. */
static void note(const char *what)
{
    printf("  check: %s\n", what);
    fflush(stdout);
}

/* Runs ARGV as a child and returns its exit status, or 127 when it could not
   start or did not exit normally. Why: V8 flags are process-wide and frozen
   after the first isolate, so each flag set needs a fresh process. ARGV[0] is
   replaced with this executable's full path so the child does not depend on
   the working directory. */
static int run_self(char *argv[])
{
    argv[0] = (char *)g_self;
#ifdef _WIN32
    intptr_t rc = _spawnv(_P_WAIT, argv[0], argv);
    return rc < 0 ? 127 : (int)rc;
#else
    int status = 0;
    pid_t pid = fork();
    if (pid < 0) return 127;
    if (pid == 0) {
        execv(argv[0], argv);
        _exit(127);
    }
    if (waitpid(pid, &status, 0) < 0) return 127;
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128;
#endif
}

/* Reads a small text file into BUF. Returns false when it cannot be read. */
static bool read_text(const char *path, char *buf, size_t cap)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (!f || cap == 0) {
        if (f) fclose(f);
        return false;
    }
    n = fread(buf, 1, cap - 1, f);
    fclose(f);
    buf[n] = '\0';
    return true;
}

/* Writes TEXT to PATH. Returns true on success. */
static bool write_text(const char *path, const char *text)
{
    bool ok;
    FILE *f = fopen(path, "w");
    if (!f) return false;
    ok = fputs(text, f) >= 0;
    return fclose(f) == 0 && ok;
}

/* Copies the first `*.bin` file name in DIR into OUT. Why: the code-cache key
   is private to the V8 build, so the test compares the names two runs chose
   rather than recomputing the hash. Returns false when DIR has no cache. */
static bool first_cache_name(const char *dir, char *out, size_t cap)
{
#ifdef _WIN32
    char pattern[TEST_PATH_MAX];
    WIN32_FIND_DATAA fd;
    HANDLE h;
    snprintf(pattern, sizeof pattern, "%s/*.bin", dir);
    h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    snprintf(out, cap, "%s", fd.cFileName);
    FindClose(h);
    return true;
#else
    DIR *d = opendir(dir);
    struct dirent *ent;
    bool found = false;
    if (!d) return false;
    while ((ent = readdir(d)) != NULL) {
        size_t n = strlen(ent->d_name);
        if (n > 4 && strcmp(ent->d_name + n - 4, ".bin") == 0) {
            snprintf(out, cap, "%s", ent->d_name);
            found = true;
            break;
        }
    }
    closedir(d);
    return found;
#endif
}

/* Child mode: run PARSER on one --js-flags value and return its status. Why:
   the parent must observe the real process exit code 2 for a bad flag. */
static int child_parse(const char *value)
{
    char out[512];
    char *argv[] = {"test_js_flags", "--js-flags", (char *)value, NULL};
    return smz_js_flags_parse(3, argv, out, sizeof out);
}

/* Child mode: start V8 with --trace-gc, force scavenges by allocating and
   dropping objects, and leave the trace in PATH. Returns 0 on success. */
static int child_trace(const char *path)
{
    char parsed[512];
    JSEngine *engine;
    bool ok;
    if (!freopen(path, "w", stdout)) return 3;
    if (smz_js_flags_parse(2,
                           (char *[]){"test_js_flags", "--js-flags=--trace-gc", NULL},
                           parsed, sizeof parsed) != 0)
        return 4;
    if (!js_engine_set_js_flags(parsed)) return 5;
    engine = js_engine_init();
    if (!engine) return 6;
    ok = js_engine_eval(engine,
                        "var keep = [];"
                        "for (var i = 0; i < 300000; i++) {"
                        "  keep.push({a: i, b: 'x' + i});"
                        "  if (keep.length > 2000) keep.length = 0;"
                        "}",
                        "gc-force.js");
    js_engine_shutdown(engine);
    fflush(stdout);
    return ok ? 0 : 7;
}

/* Child mode: start V8 under FLAGS ("" for none), load one script through the
   code cache in DIR, and write the cache file name to RESULT. Returns 0. */
static int child_cache(const char *flags, const char *dir, const char *result)
{
    char script[TEST_PATH_MAX], name[TEST_PATH_MAX];
    char *args[] = {"test_js_flags", "--js-flags", (char *)flags, NULL};
    char parsed[512];
    JSEngine *engine;
    bool ok;
    if (smz_js_flags_parse(3, args, parsed, sizeof parsed) != 0) return 2;
    if (parsed[0] && !js_engine_set_js_flags(parsed)) return 3;
    snprintf(script, sizeof script, "%s/script.js", dir);
    if (!test_mkdir_p(dir) || !write_text(script, "var answer = 6 * 7;\n")) return 4;
    engine = js_engine_init();
    if (!engine) return 5;
    if (!js_engine_set_cache_dir(engine, dir, NULL)) return 6;
    ok = js_engine_eval_file_cached(engine, script);
    js_engine_shutdown(engine);
    if (!ok || !first_cache_name(dir, name, sizeof name)) return 7;
    return write_text(result, name) ? 0 : 8;
}

/* The allow-list accepts the seven documented flags and the last --js-flags
   wins, as --data-dir does. */
static void test_parser_accepts_allow_list(void)
{
    char out[512];
    char *all[] = {"smz", "--js-flags=--perf-prof --perf-basic-prof --prof "
                           "--trace-gc --jitless --max-old-space-size=4096 "
                           "--stack-size=2048", NULL};
    note("all seven allowed flags are accepted");
    assert(smz_js_flags_parse(2, all, out, sizeof out) == 0);
    assert(strstr(out, "--trace-gc") != NULL);
    note("the separate-value form is accepted");
    assert(smz_js_flags_parse(3, (char *[]){"smz", "--js-flags", "--prof", NULL},
                              out, sizeof out) == 0);
    assert(strcmp(out, "--prof") == 0);
    note("numeric flags accept V8's space-separated form");
    assert(smz_js_flags_parse(
               2, (char *[]){"smz", "--js-flags=--max-old-space-size 4096 "
                                       "--stack-size 2048", NULL},
               out, sizeof out) == 0);
    note("the last --js-flags wins");
    assert(smz_js_flags_parse(4,
                              (char *[]){"smz", "--js-flags=--prof",
                                         "--js-flags=--trace-gc", NULL},
                              out, sizeof out) == 0);
    assert(strcmp(out, "--trace-gc") == 0);
    note("no flag leaves the output empty");
    assert(smz_js_flags_parse(1, (char *[]){"smz", NULL}, out, sizeof out) == 0);
    assert(out[0] == '\0');
}

/* Unknown switches and malformed numeric values are refused with status 2. */
static void test_parser_rejects_unknown(void)
{
    char out[512];
    note("an unknown boolean flag is refused");
    assert(smz_js_flags_parse(
               2, (char *[]){"smz", "--js-flags=--not-a-v8-flag", NULL},
               out, sizeof out) == 2);
    assert(out[0] == '\0');
    note("a flag outside the allow-list is refused");
    assert(smz_js_flags_parse(
               2, (char *[]){"smz", "--js-flags=--log-file=out.log", NULL},
               out, sizeof out) == 2);
    note("a missing numeric value is refused");
    assert(smz_js_flags_parse(
               2, (char *[]){"smz", "--js-flags=--max-old-space-size", NULL},
               out, sizeof out) == 2);
    note("a non-numeric size is refused");
    assert(smz_js_flags_parse(
               2, (char *[]){"smz", "--js-flags=--stack-size=large", NULL},
               out, sizeof out) == 2);
    note("a boolean flag with a value is refused");
    assert(smz_js_flags_parse(2, (char *[]){"smz", "--js-flags=--trace-gc=1", NULL},
                              out, sizeof out) == 2);
    note("--js-flags without a value is refused");
    assert(smz_js_flags_parse(2, (char *[]){"smz", "--js-flags", NULL},
                              out, sizeof out) == 2);
}

/* A bad flag makes the CLI process exit with status 2, not 1 or a crash. */
static void test_unknown_flag_exit_code(void)
{
    note("a child with an unknown --js-flags exits 2");
    assert(run_self((char *[]){"test_js_flags", "parse", "--bogus-flag", NULL}) == 2);
    note("a child with an allowed --js-flags exits 0");
    assert(run_self((char *[]){"test_js_flags", "parse", "--trace-gc", NULL}) == 0);
}

/* --trace-gc reaches V8: forced allocation prints GC events on stdout. */
static void test_allowed_flag_reaches_v8(void)
{
    char path[TEST_PATH_MAX], trace[65536];
    snprintf(path, sizeof path, "%s/smz_js_flags_trace.txt", test_tmp_path("smz_js_flags"));
    test_rm_rf(test_tmp_path("smz_js_flags"));
    assert(test_mkdir_p(test_tmp_path("smz_js_flags")));
    note("a child starts V8 with --trace-gc");
    assert(run_self((char *[]){"test_js_flags", "trace", path, NULL}) == 0);
    assert(read_text(path, trace, sizeof trace));
    note("the trace names a scavenge or mark-compact");
    assert(strstr(trace, "Scavenge") != NULL || strstr(trace, "Mark-Compact") != NULL);
    test_rm_rf(test_tmp_path("smz_js_flags"));
}

/* The V8 flag hash feeds the code-cache key: the same script under different
   allowed flags gets a different cache file. */
static void test_codecache_key_changes_with_flags(void)
{
    char plain[TEST_PATH_MAX], jitless[TEST_PATH_MAX];
    char result_a[TEST_PATH_MAX], result_b[TEST_PATH_MAX];
    char a[128], b[128];
    snprintf(plain, sizeof plain, "%s/smz_js_flags_cache_plain", test_tmp_path("smz_js_flags"));
    snprintf(jitless, sizeof jitless, "%s/smz_js_flags_cache_jitless",
             test_tmp_path("smz_js_flags"));
    snprintf(result_a, sizeof result_a, "%s/name.txt", plain);
    snprintf(result_b, sizeof result_b, "%s/name.txt", jitless);
    test_rm_rf(plain);
    test_rm_rf(jitless);
    note("default flags produce a code cache");
    assert(run_self((char *[]){"test_js_flags", "cache", "", plain, result_a, NULL}) == 0);
    assert(read_text(result_a, a, sizeof a));
    note("--jitless produces a different code cache key");
    assert(run_self((char *[]){"test_js_flags", "cache", "--jitless", jitless,
                               result_b, NULL}) == 0);
    assert(read_text(result_b, b, sizeof b));
    assert(a[0] && b[0] && strcmp(a, b) != 0);
    test_rm_rf(plain);
    test_rm_rf(jitless);
}

/* Child entry point: the parent runs one mode per fresh V8 process. */
int main(int argc, char **argv)
{
    g_self = argv[0];
    if (argc >= 3 && strcmp(argv[1], "parse") == 0) return child_parse(argv[2]);
    if (argc >= 3 && strcmp(argv[1], "trace") == 0) return child_trace(argv[2]);
    if (argc >= 5 && strcmp(argv[1], "cache") == 0)
        return child_cache(argv[2], argv[3], argv[4]);
    printf("=== --js-flags tests ===\n");
    test_parser_accepts_allow_list();
    test_parser_rejects_unknown();
    test_unknown_flag_exit_code();
    test_allowed_flag_reaches_v8();
    test_codecache_key_changes_with_flags();
    printf("OK\n");
    return 0;
}
