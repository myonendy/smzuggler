/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-10-30: use the configurable runtime identity.
 * Modified 2026-10-31: dispatch headless surface inventory before startup.
 * Modified 2026-10-31: dispatch --share-bundle to the packager.
 * Modified 2026-10-31: return process.exitCode after a clean quit.
 * Modified 2026-11-01: default logs, .jsc cache and config to host data dirs.
 *
 * Modified 2026-11-04: keep runtime services on the process main thread.
 * Modified 2026-11-05: validate and forward --js-flags to V8 before startup
 * (card M1-T029).
 * Modified 2026-11-05: start an optional V8 CPU profile from --cpu-prof.
 * Modified 2026-11-05: enable --frame-stats JSON Lines and summary output.
 * Modified 2026-11-06: add the §14 perf-suite and fixture-import harness.
 *
 * Runtime entry point; owns startup and shutdown on the main thread.
 * See docs/DECISIONS.md (DEC-M0-T001) for identity ownership.
 */

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(_WIN32)
#include <direct.h>
#endif
#if !defined(__linux__) || defined(__ANDROID__)
#include <SDL3/SDL.h>
#endif
#include "smzuggler/perf_suite.h"

/* The game's fixture map ids live only in game_profile.c (AGENTS.md §4).
   The root CMake source list is outside this card's owns globs, so the
   profile translation unit is included here instead of linked separately. */
#include "smzuggler/game_profile.c"

#ifndef RMMZ_DATA_APP
#define RMMZ_DATA_APP "outsider"
#endif

/** Drop a trailing slash so later joins do not produce empty path segments. */
static void rmmz_trim_slash(char *path)
{
    size_t n;
    if (!path) return;
    n = strlen(path);
    while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\'))
        path[--n] = '\0';
}

/** Fill OUT with the platform state dir used by --data-dir without PATH.
 * Linux (not Android) uses $XDG_STATE_HOME/<app> or $HOME/.local/state/<app>
 * when XDG_STATE_HOME is unset; other platforms use SDL_GetPrefPath.
 * out is caller-owned. Returns false if the environment/pref path is missing
 * or the result does not fit. */
static bool rmmz_platform_state_dir(char *out, size_t out_size)
{
#if defined(__linux__) && !defined(__ANDROID__)
    char prefix[1024];
    const char *base = getenv("XDG_STATE_HOME");
    int n;
    if (base && base[0]) {
        n = snprintf(prefix, sizeof(prefix), "%s", base);
        if (n <= 0 || (size_t)n >= sizeof(prefix)) return false;
        rmmz_trim_slash(prefix);
        n = snprintf(out, out_size, "%s/%s", prefix, RMMZ_DATA_APP);
        return n > 0 && (size_t)n < out_size;
    }
    base = getenv("HOME");
    if (!base || !base[0]) return false;
    n = snprintf(prefix, sizeof(prefix), "%s", base);
    if (n <= 0 || (size_t)n >= sizeof(prefix)) return false;
    rmmz_trim_slash(prefix);
    n = snprintf(out, out_size, "%s/.local/state/%s", prefix, RMMZ_DATA_APP);
    return n > 0 && (size_t)n < out_size;
#else
    char *pref = SDL_GetPrefPath("", RMMZ_DATA_APP);
    int n;
    if (!pref || !out || out_size == 0) {
        if (pref) SDL_free(pref);
        return false;
    }
    n = snprintf(out, out_size, "%s", pref);
    SDL_free(pref);
    if (n <= 0 || (size_t)n >= out_size) return false;
    rmmz_trim_slash(out);
    return true;
#endif
}

/** Resolve the directory that debug.log and console.log are written to.
 * argv is borrowed and scanned for --data-dir / --data-dir=PATH (last one
 * wins). game_dir is the already-resolved --game path, or NULL. Without the
 * flag the game directory is used and an empty result means logs stay in
 * the working directory, matching the historical default. out is
 * caller-owned. Returns false if the path does not fit in out_size. */
bool rmmz_resolve_log_dir(int argc, char **argv, const char *game_dir,
                          char *out, size_t out_size)
{
    bool use_data_dir = false;
    const char *data_dir_path = NULL;
    int i, n;
    if (!out || out_size == 0) return false;
    out[0] = '\0';
    for (i = 1; argv && i < argc; i++) {
        if (!argv[i]) continue;
        if (strcmp(argv[i], "--data-dir") == 0) {
            use_data_dir = true;
            data_dir_path = NULL;
        } else if (strncmp(argv[i], "--data-dir=", 11) == 0) {
            use_data_dir = true;
            data_dir_path = argv[i] + 11;
        }
    }
    if (use_data_dir) {
        if (data_dir_path && data_dir_path[0]) {
            n = snprintf(out, out_size, "%s", data_dir_path);
            if (n <= 0 || (size_t)n >= out_size) return false;
            rmmz_trim_slash(out);
            return true;
        }
        return rmmz_platform_state_dir(out, out_size);
    }
    if (game_dir && game_dir[0]) {
        n = snprintf(out, out_size, "%s", game_dir);
        return n > 0 && (size_t)n < out_size;
    }
    return true;
}

/* Largest --js-flags value accepted, including its trailing NUL. V8 accepts
 * more, but a diagnostic run needs only a handful of words, and a bound keeps
 * the parser's stack buffer small. */
#define SMZ_JS_FLAGS_MAX 512

/** True when FLAG is a boolean flag on the --js-flags allow-list.
 * Why: a user must not reach arbitrary V8 switches (some write files or change
 * security-relevant behaviour), so the runtime names the profiler and
 * measurement flags of spec section 6.1 and accepts nothing else. FLAG is
 * borrowed and NUL-terminated; no state is kept. */
static bool js_flag_is_boolean(const char *flag)
{
    static const char *const allowed[] = {
        "--perf-prof", "--perf-basic-prof", "--prof", "--trace-gc", "--jitless",
    };
    size_t i;
    for (i = 0; i < sizeof(allowed) / sizeof(allowed[0]); i++)
        if (strcmp(flag, allowed[i]) == 0) return true;
    return false;
}

/** True when FLAG is `--name=<digits>` for an allowed numeric V8 flag.
 * `name` is borrowed ("--max-old-space-size" or "--stack-size"); an empty,
 * signed or non-numeric value is refused so V8 never sees a malformed switch.
 * `flag` is borrowed and NUL-terminated. */
static bool js_flag_has_uint(const char *flag, const char *name)
{
    size_t n = strlen(name);
    const char *p;
    if (strncmp(flag, name, n) != 0 || flag[n] != '=') return false;
    p = flag + n + 1;
    if (*p == '\0') return false;
    for (; *p; p++)
        if (!isdigit((unsigned char)*p)) return false;
    return true;
}

/** True when FLAG is one of the seven V8 flags the runtime allows, with a
 * numeric value attached as `=N`. The profiler and measurement flags are
 * documented in docs/perf.md; the two numeric ones keep their V8 units
 * (old-space MiB, stack KiB). FLAG is borrowed. */
static bool js_flag_allowed(const char *flag)
{
    return js_flag_is_boolean(flag) ||
           js_flag_has_uint(flag, "--max-old-space-size") ||
           js_flag_has_uint(flag, "--stack-size");
}

/** True when TOKEN is an allowed numeric flag name without its value; V8 also
 * accepts the documented `--flag value` form, which this lets the caller see.
 * TOKEN is borrowed. */
static bool js_flag_wants_uint(const char *token)
{
    return strcmp(token, "--max-old-space-size") == 0 ||
           strcmp(token, "--stack-size") == 0;
}

/** True when TOKEN is one or more decimal digits and so a valid value for a
 * numeric V8 flag. TOKEN is borrowed and NUL-terminated. */
static bool js_flag_is_uint(const char *token)
{
    if (!token[0]) return false;
    for (; *token; token++)
        if (!isdigit((unsigned char)*token)) return false;
    return true;
}

/** Returns the first token in WORK that is not on the allow-list, or NULL when
 * every token is valid; a numeric flag name with no following value returns
 * that name. Why: the caller reports one bad token, and V8 accepts both
 * `--flag=N` and `--flag N`, so the check has to look one token ahead. WORK is
 * NUL-terminated and is modified in place by the token split; its bytes stay
 * valid for the caller's error message. Main thread only. */
static const char *js_flags_bad_token(char *work)
{
    char *p = work;
    const char *pending = NULL;
    while (*p) {
        char *start;
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        start = p;
        while (*p && !isspace((unsigned char)*p)) p++;
        if (*p) *p++ = '\0';
        if (pending) {
            if (!js_flag_is_uint(start))
                return start[0] == '-' ? pending : start;
            pending = NULL;
        } else if (js_flag_wants_uint(start)) {
            pending = start;
        } else if (!js_flag_allowed(start)) {
            return start;
        }
    }
    return pending;
}

/** Copies the last `--js-flags=VALUE` or `--js-flags VALUE` from ARGV into OUT.
 * Why: V8 flags must be set before the isolate exists, and an unknown switch
 * must stop the process with status 2 rather than reach V8. The last occurrence
 * wins, as --data-dir does. `out` is caller-owned and always NUL-terminated on
 * success; returns 0 when there is no flag or it is valid, 2 after printing the
 * first bad token to stderr. ARGV is borrowed; main thread only. */
int smz_js_flags_parse(int argc, char **argv, char *out, size_t out_size)
{
    const char *value = NULL;
    const char *bad;
    char work[SMZ_JS_FLAGS_MAX];
    int i;
    if (out && out_size) out[0] = '\0';
    for (i = 1; argv && i < argc; i++) {
        if (!argv[i]) continue;
        if (strcmp(argv[i], "--js-flags") == 0) {
            if (i + 1 >= argc || !argv[i + 1]) {
                fprintf(stderr, "Invalid --js-flags: missing value\n");
                return 2;
            }
            value = argv[++i];
        } else if (strncmp(argv[i], "--js-flags=", 11) == 0) {
            value = argv[i] + 11;
        }
    }
    if (!value || !value[0]) return 0;
    if (!out || out_size == 0 || strlen(value) >= out_size ||
        strlen(value) >= sizeof(work)) {
        fprintf(stderr, "Invalid --js-flags: value is too long\n");
        return 2;
    }
    snprintf(out, out_size, "%s", value);
    snprintf(work, sizeof(work), "%s", value);
    bad = js_flags_bad_token(work);
    if (bad) {
        fprintf(stderr, "Invalid --js-flags flag: %s\n", bad);
        out[0] = '\0';
        return 2;
    }
    return 0;
}

/* -------------------------------------------------------------------------
 * §14 perf harness helpers. Above the RMMZ_TEST_DATA_DIR seam so tests link
 * the real parser and fixture rules without SDL, V8 or a window.
 * ------------------------------------------------------------------------- */
#define SMZ_PERF_PATH_MAX 1024
#define SMZ_PERF_SAVE_MAX ((size_t)64 * 1024 * 1024)
/** Existing directory. */
static bool perf_is_dir(const char *path)
{
    struct stat info;
    return path && path[0] && stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}
/** Existing regular file. */
static bool perf_is_file(const char *path)
{
    struct stat info;
    return path && path[0] && stat(path, &info) == 0 && S_ISREG(info.st_mode);
}
/** Create one directory; an existing directory counts as success. */
static bool perf_mkdir_one(const char *path)
{
#ifdef _WIN32
    return _mkdir(path) == 0 || errno == EEXIST;
#else
    return mkdir(path, 0777) == 0 || errno == EEXIST;
#endif
}
/** Create PATH and every missing parent; PATH is modified and restored. */
static bool perf_mkdirs(char *path)
{
    char *p;
    if (!path || !path[0]) return false;
    for (p = path + 1; *p; p++) {
        if (*p != '/' && *p != '\\') continue;
        *p = '\0';
        if (!perf_is_dir(path) && !perf_mkdir_one(path)) { *p = '/'; return false; }
        *p = '/';
    }
    return perf_is_dir(path) || perf_mkdir_one(path);
}
/** Join BASE and LEAF into OUT (caller-owned). */
static bool perf_join(char *out, size_t size, const char *base, const char *leaf)
{
    int n = snprintf(out, size, "%s/%s", base, leaf);
    return n > 0 && (size_t)n < size;
}
/** XDG data base or ~/.local/share. */
static bool perf_data_home(char *out, size_t size)
{
    const char *base = getenv("XDG_DATA_HOME");
    int n;
    if (!base || !base[0]) {
        base = getenv("HOME");
        if (!base || !base[0]) return false;
        n = snprintf(out, size, "%s/.local/share", base);
    } else {
        n = snprintf(out, size, "%s", base);
    }
    if (n <= 0 || (size_t)n >= size) return false;
    rmmz_trim_slash(out);
    return out[0] != '\0';
}
/** Fixture entry for NAME, or NULL. */
static const PerfFixture *perf_fixture(const char *name)
{
    size_t i;
    if (!name) return NULL;
    for (i = 0; i < smz_game_fixture_count; i++)
        if (strcmp(smz_game_fixtures[i].name, name) == 0)
            return &smz_game_fixtures[i];
    return NULL;
}
/** True when SAVE_MAP is one of the maps FIXTURE accepts. */
static bool perf_accepts(const PerfFixture *fixture, int save_map)
{
    return save_map == fixture->maps[0] ||
           (fixture->maps[1] != 0 && save_map == fixture->maps[1]);
}
/** Copy SRC to DST byte for byte. */
static bool perf_copy_file(const char *src, const char *dst)
{
    FILE *in = fopen(src, "rb"), *out;
    char chunk[8192];
    size_t got;
    bool ok = true;
    if (!in) return false;
    if (!(out = fopen(dst, "wb"))) { fclose(in); return false; }
    while ((got = fread(chunk, 1, sizeof chunk, in)) > 0)
        if (fwrite(chunk, 1, got, out) != got) { ok = false; break; }
    if (ferror(in) || fclose(out) != 0) ok = false;
    fclose(in);
    return ok;
}
/** Set *ERR to one clear line; ERR may be NULL or zero-sized. */
static void perf_error(char *err, size_t size, const char *fmt, ...)
{
    va_list args;
    if (!err || size == 0) return;
    va_start(args, fmt);
    vsnprintf(err, size, fmt, args);
    va_end(args);
    err[size - 1] = '\0';
}
/** Map a profile spelling; false for an unknown value. */
static bool perf_profile_parse(const char *value, SmzPerfProfile *profile)
{
    if (!strcmp(value, "dev-laptop") || !strcmp(value, "laptop"))
        *profile = SMZ_PERF_PROFILE_DEV_LAPTOP;
    else if (!strcmp(value, "steam-deck") || !strcmp(value, "deck"))
        *profile = SMZ_PERF_PROFILE_STEAM_DECK;
    else if (!strcmp(value, "deck-proxy") || !strcmp(value, "proxy"))
        *profile = SMZ_PERF_PROFILE_DECK_PROXY;
    else if (!strcmp(value, "extra") || !strcmp(value, "ac-balanced"))
        *profile = SMZ_PERF_PROFILE_EXTRA;
    else
        return false;
    return true;
}
/** Positive decimal slot with no trailing junk. */
static bool perf_slot_parse(const char *text, int *slot)
{
    long value;
    char *end;
    if (!text || !text[0]) return false;
    value = strtol(text, &end, 10);
    if (*end || value < 1 || value > 999) return false;
    *slot = (int)value;
    return true;
}
/** A directory path, or a path deliberately written with a trailing slash. */
static bool perf_stats_dir_ok(const char *path)
{
    size_t len;
    if (!path || !path[0]) return false;
    len = strlen(path);
    if (path[len - 1] == '/' || path[len - 1] == '\\') return true;
    return perf_is_dir(path);
}
/** See perf_suite.h. Unknown argv entries stay for the runtime parser. */
int smz_perf_suite_parse_args(int argc, char **argv, SmzPerfSuiteOptions *out,
                              char *err, size_t err_size)
{
    bool control = false, frames = false;
    int i;
    if (!out) return 2;
    memset(out, 0, sizeof *out);
    out->profile = SMZ_PERF_PROFILE_DEV_LAPTOP;
    if (err && err_size) err[0] = '\0';
    for (i = 1; argv && i < argc; i++) {
        if (!argv[i]) continue;
        if (!strcmp(argv[i], "--control")) control = true;
        else if (!strcmp(argv[i], "--frames")) frames = true;
        else if (!strcmp(argv[i], "--perf-suite")) out->requested = true;
        else if (!strncmp(argv[i], "--perf-suite=", 13)) {
            out->requested = true;
            if (!perf_profile_parse(argv[i] + 13, &out->profile)) {
                perf_error(err, err_size,
                    "perf-suite: unknown profile '%s' (use dev-laptop, "
                    "steam-deck, deck-proxy or extra)", argv[i] + 13);
                return 2;
            }
        } else if (!strcmp(argv[i], "--import-fixture")) {
            out->import_requested = true;
            if (i + 2 >= argc) {
                perf_error(err, err_size,
                    "import-fixture: expected --import-fixture <slot> <fixture>");
                return 2;
            }
            if (!perf_slot_parse(argv[i + 1], &out->import_slot)) {
                perf_error(err, err_size,
                    "import-fixture: slot must be 1-999, got '%s'", argv[i + 1]);
                return 2;
            }
            out->import_fixture = argv[i + 2];
            if (!perf_fixture(out->import_fixture)) {
                perf_error(err, err_size,
                    "import-fixture: unknown fixture '%s' (expected fx-map92, "
                    "fx-trade or fx-combat)", out->import_fixture);
                return 2;
            }
            i += 2;
        } else if (!strncmp(argv[i], "--import-fixture=", 17)) {
            perf_error(err, err_size,
                "import-fixture: use --import-fixture <slot> <fixture>");
            return 2;
        } else if (!strcmp(argv[i], "--frame-stats")) {
            if (i + 1 >= argc || !argv[i + 1] || argv[i + 1][0] == '-') {
                perf_error(err, err_size,
                    "frame-stats: --frame-stats needs a file or directory path");
                return 2;
            }
            out->frame_stats_dir = argv[++i];
        } else if (!strncmp(argv[i], "--frame-stats=", 14)) {
            if (!argv[i][14]) {
                perf_error(err, err_size, "frame-stats: --frame-stats= needs a path");
                return 2;
            }
            out->frame_stats_dir = argv[i] + 14;
        }
    }
    if (out->requested && out->import_requested) {
        perf_error(err, err_size,
            "perf-suite: --perf-suite and --import-fixture cannot be combined");
        return 2;
    }
    if (out->import_requested || !out->requested) return 0;
    if (!out->frame_stats_dir) {
        perf_error(err, err_size,
            "perf-suite: --frame-stats=<directory> is required");
        return 2;
    }
    if (!perf_stats_dir_ok(out->frame_stats_dir)) {
        perf_error(err, err_size,
            "perf-suite: --frame-stats must name a directory; end it with / "
            "or create it first");
        return 2;
    }
    if (control) {
        perf_error(err, err_size,
            "perf-suite: --control cannot be combined with scripted input replay");
        return 2;
    }
    if (frames) {
        perf_error(err, err_size,
            "perf-suite: --frames would cut the 10 s + 120 s windows");
        return 2;
    }
    return 0;
}
/** See perf_suite.h. */
const char *smz_perf_profile_name(SmzPerfProfile profile)
{
    static const char *const names[] = {
        "dev-laptop", "steam-deck", "deck-proxy", "extra"};
    return (unsigned)profile < 4 ? names[profile] : "unknown";
}
/** See perf_suite.h. */
bool smz_perf_fixture_name_valid(const char *name)
{
    return perf_fixture(name) != NULL;
}
/** See perf_suite.h. */
bool smz_perf_fixtures_dir(char *out, size_t size)
{
    char base[SMZ_PERF_PATH_MAX];
    if (!out || size == 0) return false;
    out[0] = '\0';
    return perf_data_home(base, sizeof base) &&
           perf_join(out, size, base, "smzuggler-rt/fixtures");
}
/** See perf_suite.h. */
bool smz_perf_scratch_dir(char *out, size_t size)
{
    char base[SMZ_PERF_PATH_MAX];
    if (!out || size == 0) return false;
    out[0] = '\0';
    return perf_data_home(base, sizeof base) &&
           perf_join(out, size, base, "smzuggler-rt/scratch");
}
/** SMZUGGLER_SAVE_DIR, or the host saves directory as a fallback. */
static bool perf_save_dir(char *out, size_t size)
{
    char base[SMZ_PERF_PATH_MAX];
    const char *explicit_dir = getenv("SMZUGGLER_SAVE_DIR");
    int n;
    if (explicit_dir && explicit_dir[0]) {
        n = snprintf(out, size, "%s", explicit_dir);
        if (n <= 0 || (size_t)n >= size) return false;
        rmmz_trim_slash(out);
        return true;
    }
    return perf_data_home(base, sizeof base) &&
           perf_join(out, size, base, "smzuggler-rt/saves");
}
/** Find `"_mapId": N` (or `"mapId": N`) in a NUL-terminated JSON document. */
static bool perf_json_map_id(const char *text, int *map_id)
{
    static const char *const keys[] = {"\"_mapId\"", "\"mapId\""};
    size_t k;
    for (k = 0; k < sizeof(keys) / sizeof(keys[0]); k++) {
        const char *p = text;
        while ((p = strstr(p, keys[k])) != NULL) {
            long value = 0;
            int digits = 0;
            p += strlen(keys[k]);
            while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
            if (*p++ != ':') continue;
            while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
            while (*p >= '0' && *p <= '9') {
                value = value * 10 + (*p++ - '0');
                digits++;
            }
            if (digits > 0 && value <= 2147483647L) {
                *map_id = (int)value;
                return true;
            }
        }
    }
    return false;
}
/** See perf_suite.h. */
int smz_perf_read_save_map_id(const char *path, int *map_id, char *err,
                              size_t err_size)
{
    FILE *stream;
    char *raw;
    char *text;
    long size;
    if (!path || !map_id) return 1;
    if (!(stream = fopen(path, "rb"))) {
        perf_error(err, err_size, "cannot read save file");
        return 1;
    }
    if (fseek(stream, 0, SEEK_END) != 0 || (size = ftell(stream)) <= 0 ||
        (size_t)size > SMZ_PERF_SAVE_MAX || fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        perf_error(err, err_size, "save file is empty or too large");
        return 1;
    }
    raw = malloc((size_t)size + 1);
    if (!raw || fread(raw, 1, (size_t)size, stream) != (size_t)size) {
        free(raw);
        fclose(stream);
        perf_error(err, err_size, "cannot read save file");
        return 1;
    }
    fclose(stream);
    raw[size] = '\0';
    text = raw;
    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') text++;
    if (*text != '{' && *text != '[') {
        free(raw);
        perf_error(err, err_size, "save file is not a JSON RPG Maker MZ save");
        return 1;
    }
    if (!perf_json_map_id(text, map_id)) {
        free(raw);
        perf_error(err, err_size, "save file has no map id");
        return 1;
    }
    free(raw);
    return 0;
}
/** See perf_suite.h. Refuses before writing so no bad fixture appears. */
int smz_perf_import_fixture(const SmzPerfSuiteOptions *options, char *err,
                            size_t err_size)
{
    const PerfFixture *fixture;
    char saves[SMZ_PERF_PATH_MAX], source[SMZ_PERF_PATH_MAX];
    char fixtures[SMZ_PERF_PATH_MAX], target[SMZ_PERF_PATH_MAX];
    char slot[64], detail[256], alternate[32] = "";
    int map_id;
    if (!options || !options->import_fixture) return 2;
    if (!(fixture = perf_fixture(options->import_fixture))) {
        perf_error(err, err_size, "import-fixture: unknown fixture name");
        return 2;
    }
    if (!perf_save_dir(saves, sizeof saves)) {
        perf_error(err, err_size,
            "import-fixture: set SMZUGGLER_SAVE_DIR to the game's save folder");
        return 1;
    }
    snprintf(slot, sizeof slot, "file%d.rmmzsave", options->import_slot);
    if (!perf_join(source, sizeof source, saves, slot) || !perf_is_file(source)) {
        perf_error(err, err_size,
            "import-fixture: save slot %d not found in the save directory",
            options->import_slot);
        return 1;
    }
    if (smz_perf_read_save_map_id(source, &map_id, detail, sizeof detail)) {
        perf_error(err, err_size, "import-fixture: slot %d: %s",
                   options->import_slot, detail);
        return 1;
    }
    if (!perf_accepts(fixture, map_id)) {
        if (fixture->maps[1])
            snprintf(alternate, sizeof alternate, " or %d", fixture->maps[1]);
        perf_error(err, err_size,
            "import-fixture: %s requires map %d%s, but slot %d is on map %d",
            fixture->name, fixture->maps[0], alternate, options->import_slot,
            map_id);
        return 1;
    }
    if (!smz_perf_fixtures_dir(fixtures, sizeof fixtures) ||
        !perf_mkdirs(fixtures) ||
        !perf_join(target, sizeof target, fixtures, fixture->name) ||
        !perf_copy_file(source, target)) {
        perf_error(err, err_size, "import-fixture: cannot write %s", fixture->name);
        return 1;
    }
    return 0;
}
/** See perf_suite.h. */
int smz_perf_validate_fixture(const char *name, char *err, size_t err_size)
{
    const PerfFixture *fixture = perf_fixture(name);
    char fixtures[SMZ_PERF_PATH_MAX], path[SMZ_PERF_PATH_MAX];
    char detail[256], alternate[32] = "";
    int map_id;
    if (!fixture) {
        perf_error(err, err_size, "perf-suite: unknown fixture '%s'", name);
        return 1;
    }
    if (!smz_perf_fixtures_dir(fixtures, sizeof fixtures)) {
        perf_error(err, err_size, "perf-suite: no XDG_DATA_HOME or HOME");
        return 1;
    }
    if (!perf_join(path, sizeof path, fixtures, fixture->name) ||
        !perf_is_file(path)) {
        perf_error(err, err_size,
            "perf-suite: missing fixture %s; create it with --import-fixture "
            "(see docs/testing-fixtures.md)", fixture->name);
        return 1;
    }
    if (smz_perf_read_save_map_id(path, &map_id, detail, sizeof detail)) {
        perf_error(err, err_size, "perf-suite: fixture %s: %s", fixture->name,
                   detail);
        return 1;
    }
    if (!perf_accepts(fixture, map_id)) {
        if (fixture->maps[1])
            snprintf(alternate, sizeof alternate, " or %d", fixture->maps[1]);
        perf_error(err, err_size,
            "perf-suite: fixture %s is on map %d but the scene needs map %d%s",
            fixture->name, map_id, fixture->maps[0], alternate);
        return 1;
    }
    return 0;
}
/* Compile-time seam so tests/test_data_dir.c can link this helper without
 * pulling the rest of the runtime or a second main(). */
#ifndef RMMZ_TEST_DATA_DIR

#include "platform/platform.h"
#ifdef SMZUGGLER_NAME
#include "smzuggler/data_dirs.h"
#endif
#include "smzuggler/log_scrub.h"
#include "smzuggler/dump_surface.h"
#include "engine/js_engine.h"
#include "engine/js_profiler.h"
#include "engine/game_loop.h"
#include "engine/script_loader.h"
#include "bindings/bind_io.h"
#include "bindings/bind_image.h"
#include "bindings/bind_canvas2d.h"
#include "bindings/bind_renderer.h"
#include "bindings/bind_font.h"
#include "bindings/bind_tilemap.h"
#include "bindings/bind_audio.h"
#include "bindings/bind_input.h"
#include "bindings/bind_effekseer.h"
#include "bindings/bind_video.h"
#include "bindings/bind_filters.h"
#include "bindings/bind_platform.h"
#include "engine/error_handler.h"
#include "audio/audio_engine.h"
#include "effects/effekseer_backend.h"
#include "video/video_player.h"
#include "input/input_manager.h"
#include "rendering/image_loader.h"
#include "rendering/canvas2d.h"
#include "rendering/font_manager.h"
#include "rendering/renderer.h"
#include "rendering/filters.h"
#include "io/file_io.h"
#include "platform/win_console.h"
#include "platform/android_support.h"

/* game_loop.h is outside this card's owns list, so the new --frame-stats
 * entry point is declared here instead of in the shared header. */
bool game_loop_enable_frame_stats(GameLoop *loop, const char *path,
                                  double refresh_hz);

/* js_engine_v8.cc stores the validated --js-flags value for js_engine_init.
   js_engine.h is outside this card's file list, so the entry point is declared
   here, where the only caller lives (card M1-T029). */
bool js_engine_set_js_flags(const char *flags);

#include <SDL3/SDL.h>
/* SDL3 dropped the SDL2main library: including this header in the file that
   defines main() supplies the Windows GUI-subsystem WinMain entry point. */
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <direct.h>
#include <process.h>
#define smz_getcwd _getcwd
#else
#include <unistd.h>
#include <sys/wait.h>
#define smz_getcwd getcwd
#endif

#ifndef RMMZ_DEFAULT_WIDTH
#define RMMZ_DEFAULT_WIDTH 816
#endif
#ifndef RMMZ_DEFAULT_HEIGHT
#define RMMZ_DEFAULT_HEIGHT 624
#endif

#define APP_TITLE RMMZ_APP_NAME

/* Report a fatal startup error: log it and, where the platform provides one,
   show a dialog so the message is visible even without a console. */
static void fatal_error(const char *fmt, ...)
{
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    error_handler_log(LOG_ERROR, "%s", msg);
    platform_show_error_dialog(APP_TITLE, msg);
}

/** True when DIR contains the packager; copies its path into OUT (caller-owned). */
static bool share_try_script(const char *dir, char *out, size_t n)
{
    int k = snprintf(out, n, "%s/tools/share/make_bundle.py", dir);
    if (k > 0 && (size_t)k < n && file_io_exists(out)) return true;
    k = snprintf(out, n, "%s/share/make_bundle.py", dir);
    return k > 0 && (size_t)k < n && file_io_exists(out);
}

/** Strip a trailing slash so parent walking does not stall on SDL_GetBasePath. */
static void share_trim(char *dir)
{
    size_t n = strlen(dir);
    while (n > 1 && (dir[n - 1] == '/' || dir[n - 1] == '\\'))
        dir[--n] = '\0';
}

/** Walk START and its parents looking for the packager; OUT is caller-owned. */
static bool share_walk(const char *start, char *out, size_t n)
{
    char dir[1024];
    if (!start || !start[0]) return false;
    snprintf(dir, sizeof(dir), "%s", start);
    share_trim(dir);
    for (int i = 0; i < 8; i++) {
        char *slash;
        if (share_try_script(dir, out, n)) return true;
        slash = strrchr(dir, '/');
#ifdef _WIN32
        {
            char *alt = strrchr(dir, '\\');
            if (!slash || (alt && alt > slash)) slash = alt;
        }
#endif
        if (!slash || slash == dir) break;
        *slash = '\0';
        if (!dir[0]) break;
    }
    return false;
}

/** Locate make_bundle.py from the executable, argv0 or cwd; OUT is caller-owned. */
static bool share_find(const char *argv0, char *out, size_t n)
{
    const char *base = SDL_GetBasePath();
    char cwd[1024];
    if (base && share_walk(base, out, n)) return true;
    if (smz_getcwd(cwd, sizeof(cwd)) && share_walk(cwd, out, n)) return true;
    if (argv0 && strpbrk(argv0, "/\\")) {
        char *slash;
        snprintf(cwd, sizeof(cwd), "%s", argv0);
        share_trim(cwd);
        slash = strrchr(cwd, '/');
#ifdef _WIN32
        {
            char *alt = strrchr(cwd, '\\');
            if (!slash || (alt && alt > slash)) slash = alt;
        }
#endif
        if (slash) {
            *slash = '\0';
            if (share_walk(cwd, out, n)) return true;
        }
    }
    return false;
}

/** Run ARGS as python3 and return its exit status; 127 if the spawn itself fails. */
static int share_spawn(char *const *args)
{
#ifdef _WIN32
    int rc = _spawnvp(_P_WAIT, args[0], args);
    return rc < 0 ? 127 : rc;
#else
    pid_t pid = fork();
    int status = 0;
    if (pid < 0) return 127;
    if (pid == 0) {
        execvp(args[0], args);
        _exit(127);
    }
    if (waitpid(pid, &status, 0) < 0) return 127;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return 1;
#endif
}

/** Dispatch --share-bundle to the Python packager for the last state-dir run.
 * out_path may be NULL (default zip name). argv0 locates the source tree.
 * The calling thread owns the strings; this function does not keep them. */
static int share_bundle_run(const char *out_path, const char *argv0)
{
    char script[1024];
    const char *out;
    char *args[6];
    if (!share_find(argv0, script, sizeof(script))) {
        fprintf(stderr, "share-bundle: tools/share/make_bundle.py not found\n");
        return 1;
    }
    out = (out_path && out_path[0]) ? out_path : "share-bundle.zip";
    args[0] = "python3";
    args[1] = script;
    args[2] = "--last-run";
    args[3] = "-o";
    args[4] = (char *)out;
    args[5] = NULL;
    return share_spawn(args);
}

/* If a directory exists at "<exe dir>/<name>", copy its path into out.
   Used to locate a packaged game/shims folder next to the executable so
   the runtime can be launched without arguments (e.g. by double-click). */
static bool find_dir_next_to_exe(const char *name, char *out, size_t out_size)
{
    /* SDL3 owns this string; it must not be freed. */
    const char *base = SDL_GetBasePath();
    if (!base) return false;
    int n = snprintf(out, out_size, "%s%s", base, name);
    if (n <= 0 || (size_t)n >= out_size) return false;
    return file_io_is_directory(out);
}

/** Read process.exitCode after the loop stops so process.exit(code) becomes
 * the process status. Borrows js on the main thread; 0 when the code is
 * absent or evaluation fails. Node process.exit; compat process.exit. */
static int process_exit_status(JSEngine *js)
{
    char *text;
    int code;
    if (!js) return 0;
    text = js_engine_eval_string(js,
        "(typeof process === 'object' && process &&"
        " typeof process.exitCode === 'number') ? String(process.exitCode | 0) : '0'",
        "<exit-code>");
    if (!text) return 0;
    code = atoi(text);
    js_engine_free_string(text);
    return code;
}

/** Starts the optional V8 CPU profile requested by `--cpu-prof`.
 *
 * Why: path selection and start-up logging are one decision, so keeping them
 * out of runtime_main leaves the entry point readable. `path` is borrowed
 * from argv and may be NULL or empty, in which case the profile goes under
 * `<state_dir>/profiles`; `state_dir` and `game_dir` are borrowed and may be
 * NULL. Returns the caller-owned handle to pass to js_profiler_stop, or NULL
 * after logging when no path can be chosen or V8 refuses to start. Main
 * thread, before the first script is evaluated. */
static js_profiler *start_cpu_profile(jsb_runtime *jsb, const char *path,
                                      const char *state_dir, const char *game_dir)
{
    char default_path[1024];
    const char *out = path;
    if (!out || !out[0]) {
        if (!js_profiler_default_path(state_dir, default_path, sizeof default_path)) {
            error_handler_log(LOG_ERROR,
                              "--cpu-prof needs a state directory; pass --cpu-prof=FILE");
            return NULL;
        }
        out = default_path;
    }
    js_profiler *profiler = js_profiler_start(jsb, out, game_dir);
    if (profiler)
        error_handler_log(LOG_INFO, "CPU profile will be written to %s", out);
    else
        error_handler_log(LOG_ERROR, "Could not start the V8 CPU profiler");
    return profiler;
}

/* -------------------------------------------------------------------------
 * §14 perf-suite execution: fixed 10 s + 120 s windows, three runs, and the
 * same InputEvents a keyboard produces. Main thread only.
 * ------------------------------------------------------------------------- */
#define SMZ_PERF_WARMUP_SECONDS 10
#define SMZ_PERF_SCORED_SECONDS 120
#define SMZ_PERF_RUNS 3
#define SMZ_PERF_REFRESH_HZ 60.0
#define SMZ_PERF_SCENE_WAIT 1800 /* 30 s to reach a scripted start state */
typedef struct { const char *id; const char *fixture; } PerfScene;
typedef struct { int keycode; const char *key; int release_frame; } PerfHeldKey;
static const PerfScene kPerfScenes[] = {
    {"S1", NULL}, {"S2", "fx-map92"}, {"S3", "fx-map92"}
};
/** Push one DOM-style key event; input_manager copies the event. */
static void perf_send_key(int keycode, const char *key, bool down)
{
    InputEvent event;
    memset(&event, 0, sizeof event);
    event.type = down ? INPUT_KEY_DOWN : INPUT_KEY_UP;
    event.keyCode = keycode;
    snprintf(event.key, sizeof event.key, "%s", key);
    snprintf(event.code, sizeof event.code, "%s", key);
    input_manager_push_event(&event);
}
/** Press KEY with a hold time unless one is already held. */
static void perf_hold_key(PerfHeldKey *held, int frame, int keycode,
                          const char *key, int hold)
{
    perf_send_key(keycode, key, true);
    held->keycode = keycode;
    held->key = key;
    held->release_frame = frame + hold;
}
/** Fixed §14.6 routes: cursor on S1, a walk loop on S2, submenus on S3. */
static void perf_route_tick(const char *scene, int frame, PerfHeldKey *held)
{
    static const int dirs[4] = {39, 40, 37, 38};
    static const char *const names[4] = {
        "ArrowRight", "ArrowDown", "ArrowLeft", "ArrowUp"};
    int code = 0;
    const char *key = NULL;
    int hold = 3;
    if (held->keycode && frame >= held->release_frame) {
        perf_send_key(held->keycode, held->key, false);
        held->keycode = 0;
    }
    if (held->keycode) return;
    if (strcmp(scene, "S1") == 0) {
        if (frame % 300 == 0) {
            code = (frame / 300) % 2 ? 38 : 40;
            key = code == 38 ? "ArrowUp" : "ArrowDown";
        }
    } else if (strcmp(scene, "S2") == 0) {
        if (frame % 480 == 0) {
            int phase = (frame / 480) % 4;
            code = dirs[phase];
            key = names[phase];
            hold = 120;
        } else if (frame % 480 == 240) {
            code = 13;
            key = "Enter";
        }
    } else if (strcmp(scene, "S3") == 0) {
        int phase = frame % 720;
        if (phase == 0) { code = 13; key = "Enter"; }
        else if (phase == 540) { code = 27; key = "Escape"; }
        else if (phase == 570) { code = 40; key = "ArrowDown"; }
    }
    if (code) perf_hold_key(held, frame, code, key, hold);
}
/** Call one method on the installed JS bridge; result is caller-owned. */
static char *perf_call(JSEngine *js, const char *method)
{
    char source[256];
    snprintf(source, sizeof source, "__smz_perf_bridge.%s", method);
    return js_engine_eval_string(js, source, "<perf-suite>");
}
/** Escape PATH for a double-quoted JS string literal. */
static bool perf_js_escape(const char *path, char *out, size_t out_size)
{
    size_t used = 0;
    const unsigned char *p = (const unsigned char *)path;
    while (p && *p) {
        const char *piece = NULL;
        char escaped[8];
        if (*p == '\\' || *p == '"') {
            escaped[0] = '\\'; escaped[1] = (char)*p; escaped[2] = '\0';
            piece = escaped;
        } else if (*p == '\n') {
            piece = "\\n";
        } else {
            escaped[0] = (char)*p; escaped[1] = '\0'; piece = escaped;
        }
        if (used + strlen(piece) + 1 > out_size) return false;
        memcpy(out + used, piece, strlen(piece));
        used += strlen(piece);
        out[used] = '\0';
        p++;
    }
    if (out_size == 0) return false;
    out[used] = '\0';
    return true;
}
/** Install the JS bridge: scene name, map id, scratch save dir, fixture load,
 * title reset and menu push. Game globals exist after script_loader_load_all. */
static bool perf_install_bridge(JSEngine *js, const char *scratch_dir,
                                char *err, size_t err_size)
{
    char escaped[SMZ_PERF_PATH_MAX * 2];
    char source[SMZ_PERF_PATH_MAX * 3];
    int n;
    if (!perf_js_escape(scratch_dir, escaped, sizeof escaped)) {
        perf_error(err, err_size, "perf-suite: scratch path is too long");
        return false;
    }
    n = snprintf(source, sizeof source,
        "globalThis.__smz_perf_bridge={scene:function(){try{return"
        "(typeof SceneManager!=='undefined'&&SceneManager._scene)?"
        "String(SceneManager._scene.constructor.name):'';}catch(e){return '';}},"
        "map:function(){try{return(typeof $gameMap!=='undefined'&&$gameMap)?"
        "String($gameMap.mapId()):'';}catch(e){return '';}},"
        "saves:function(d){try{if(typeof StorageManager==='undefined')return"
        "'no StorageManager';StorageManager.localFileDirectoryPath=function(){"
        "return d;};StorageManager.fileDirectoryPath=function(){return d;};"
        "return 'ok';}catch(e){return String(e);}},"
        "load:function(s){try{if(typeof DataManager==='undefined')return"
        "'no DataManager';DataManager.loadGame(s);return 'ok';}"
        "catch(e){return String(e);}},"
        "title:function(){try{if(typeof Scene_Title==='undefined')return"
        "'no Scene_Title';if(this.scene()!=='Scene_Title')"
        "SceneManager.goto(Scene_Title);return 'ok';}catch(e){return String(e);}},"
        "push:function(n){try{var c=globalThis[n];if(typeof c!=='function')"
        "return 'missing '+n;if(this.scene()!==n)SceneManager.push(c);"
        "return 'ok';}catch(e){return String(e);}},"
        "quit:function(){try{if(typeof SceneManager!=='undefined'&&"
        "typeof Scene_Title!=='undefined')SceneManager.goto(Scene_Title);"
        "return 'ok';}catch(e){return String(e);}}};"
        "__smz_perf_bridge.saves(\"%s\");\n", escaped);
    if (n <= 0 || (size_t)n >= sizeof source ||
        !js_engine_eval(js, source, "<perf-suite-bridge>")) {
        perf_error(err, err_size, "perf-suite: could not install the JS bridge");
        return false;
    }
    return true;
}
/** Compare and free a bridge result. */
static bool perf_bridge_is(char *result, const char *want)
{
    bool same = result && strcmp(result, want) == 0;
    js_engine_free_string(result);
    return same;
}
/** True when the scene and (when want_map > 0) map match. */
static bool perf_scene_matches(JSEngine *js, const char *want_scene,
                               int want_map)
{
    bool match = perf_bridge_is(perf_call(js, "scene()"), want_scene);
    char expected[16];
    if (!match || want_map <= 0) return match;
    snprintf(expected, sizeof expected, "%d", want_map);
    return perf_bridge_is(perf_call(js, "map()"), expected);
}
/** Step until the scene/map matches, the game quits, or 30 s expires. */
static bool perf_wait_for_scene(GameLoop *loop, JSEngine *js,
                                const char *want_scene, int want_map,
                                char *err, size_t err_size)
{
    int frame;
    for (frame = 0; frame < SMZ_PERF_SCENE_WAIT; frame++) {
        if (perf_scene_matches(js, want_scene, want_map)) return true;
        if (!game_loop_step(loop)) {
            perf_error(err, err_size,
                       "perf-suite: game exited while waiting for %s",
                       want_scene);
            return false;
        }
    }
    perf_error(err, err_size, "perf-suite: timed out waiting for %s",
               want_scene);
    return false;
}
/** Put one scene on its scripted start state. */
static bool perf_setup_scene(GameLoop *loop, JSEngine *js,
                             const PerfScene *scene, char *err, size_t err_size)
{
    char call[64];
    char *result;
    int map_id = 0;
    if (strcmp(scene->id, "S1") == 0) {
        if (!perf_bridge_is(perf_call(js, "title()"), "ok")) {
            perf_error(err, err_size, "perf-suite: could not reach the title");
            return false;
        }
        return perf_wait_for_scene(loop, js, "Scene_Title", 0, err, err_size);
    }
    if (scene->fixture) {
        const PerfFixture *fixture = perf_fixture(scene->fixture);
        if (fixture) map_id = fixture->maps[0];
    }
    snprintf(call, sizeof call, "load(%d)", 1); /* scratch copy of the fixture */
    if (!perf_bridge_is(perf_call(js, call), "ok")) {
        perf_error(err, err_size, "perf-suite: fixture load failed for %s",
                   scene->id);
        return false;
    }
    if (!perf_wait_for_scene(loop, js, "Scene_Map", map_id, err, err_size))
        return false;
    if (strcmp(scene->id, "S3") == 0) {
        if (!perf_bridge_is(perf_call(js, "push(\"Scene_Menu\")"), "ok")) {
            perf_error(err, err_size, "perf-suite: could not open the menu");
            return false;
        }
        return perf_wait_for_scene(loop, js, "Scene_Menu", 0, err, err_size);
    }
    return true;
}
/** Open one frame-stats file, closing any previous one. */
static bool perf_open_stats(GameLoop *loop, const char *dir,
                            const char *profile, const char *name,
                            char *err, size_t err_size)
{
    char file[64];
    char path[SMZ_PERF_PATH_MAX];
    snprintf(file, sizeof file, "frame-stats-%s-%s.jsonl", profile, name);
    if (!perf_join(path, sizeof path, dir, file) ||
        !game_loop_enable_frame_stats(loop, path, SMZ_PERF_REFRESH_HZ)) {
        perf_error(err, err_size, "perf-suite: cannot write %s", file);
        return false;
    }
    return true;
}
/** Run one window with the fixed input route. */
static bool perf_run_frames(GameLoop *loop, const char *scene_id, int frames,
                            char *err, size_t err_size)
{
    PerfHeldKey held = {0, NULL, 0};
    int frame;
    for (frame = 0; frame < frames; frame++) {
        perf_route_tick(scene_id, frame, &held);
        if (!game_loop_step(loop)) {
            perf_error(err, err_size, "perf-suite: game exited during %s",
                       scene_id);
            return false;
        }
    }
    if (held.keycode) perf_send_key(held.keycode, held.key, false);
    return true;
}
/** Copy fx-map92 into the scratch save directory as slot 1. */
static bool perf_prepare_scratch(char *err, size_t err_size)
{
    char fixtures[SMZ_PERF_PATH_MAX], scratch[SMZ_PERF_PATH_MAX];
    char source[SMZ_PERF_PATH_MAX], target[SMZ_PERF_PATH_MAX];
    if (smz_perf_validate_fixture("fx-map92", err, err_size) != 0) return false;
    if (!smz_perf_fixtures_dir(fixtures, sizeof fixtures) ||
        !smz_perf_scratch_dir(scratch, sizeof scratch) || !perf_mkdirs(scratch) ||
        !perf_join(source, sizeof source, fixtures, "fx-map92") ||
        !perf_join(target, sizeof target, scratch, "file1.rmmzsave")) {
        perf_error(err, err_size, "perf-suite: cannot prepare scratch saves");
        return false;
    }
    if (!perf_copy_file(source, target)) {
        perf_error(err, err_size, "perf-suite: cannot copy fx-map92 to scratch");
        return false;
    }
    return true;
}
/** Run S1-S3 three times, one frame-stats file per scene and run. */
static bool perf_suite_run(GameLoop *loop, JSEngine *js,
                           const SmzPerfSuiteOptions *options, char *err,
                           size_t err_size)
{
    const char *profile = smz_perf_profile_name(options->profile);
    char dir[SMZ_PERF_PATH_MAX], scratch[SMZ_PERF_PATH_MAX];
    char setup[SMZ_PERF_PATH_MAX];
    int run, warmup = SMZ_PERF_WARMUP_SECONDS * (int)SMZ_PERF_REFRESH_HZ;
    int scored = SMZ_PERF_SCORED_SECONDS * (int)SMZ_PERF_REFRESH_HZ;
    size_t scene;
    if (!options->frame_stats_dir) {
        perf_error(err, err_size, "perf-suite: bad frame-stats directory");
        return false;
    }
    snprintf(dir, sizeof dir, "%s", options->frame_stats_dir);
    rmmz_trim_slash(dir);
    if (!perf_mkdirs(dir)) {
        perf_error(err, err_size, "perf-suite: cannot create the report directory");
        return false;
    }
    if (!smz_perf_scratch_dir(scratch, sizeof scratch) ||
        !perf_install_bridge(js, scratch, err, err_size))
        return false;
    if (!perf_join(setup, sizeof setup, dir, "setup.jsonl") ||
        !game_loop_enable_frame_stats(loop, setup, SMZ_PERF_REFRESH_HZ)) {
        perf_error(err, err_size, "perf-suite: cannot write setup.jsonl");
        return false;
    }
    for (run = 1; run <= SMZ_PERF_RUNS; run++) {
        for (scene = 0; scene < sizeof kPerfScenes / sizeof kPerfScenes[0];
             scene++) {
            char name[64];
            if (!perf_setup_scene(loop, js, &kPerfScenes[scene], err, err_size))
                return false;
            snprintf(name, sizeof name, "%s-run%d", kPerfScenes[scene].id, run);
            if (!perf_open_stats(loop, dir, profile, name, err, err_size) ||
                !perf_run_frames(loop, kPerfScenes[scene].id, warmup + scored,
                                 err, err_size))
                return false;
            game_loop_enable_frame_stats(loop, "", SMZ_PERF_REFRESH_HZ);
        }
    }
    game_loop_enable_frame_stats(loop, "", SMZ_PERF_REFRESH_HZ);
    remove(setup);
    perf_bridge_is(perf_call(js, "quit()"), "ok");
    return true;
}
/** Start the runtime on the main thread using borrowed CLI arguments; return
 * an exit status on startup failure or after releasing runtime resources. */
static int runtime_main(int argc, char *argv[])
{
    /* Android: printf output is lost unless routed to logcat. No-op elsewhere. */
    android_support_init_logging();

    /* V8 flags must be validated before the window or the isolate exists, so
       a bad --js-flags is a usage error (status 2), not a half-started run. */
    char js_flags[SMZ_JS_FLAGS_MAX];
    int js_flags_status = smz_js_flags_parse(argc, argv, js_flags, sizeof js_flags);
    if (js_flags_status != 0) return js_flags_status;

    /* The §14 harness options are validated before a window or isolate exists,
       so a bad combination or fixture is a clear usage error. */
    SmzPerfSuiteOptions perf_options;
    char perf_err[512];
    int perf_status = smz_perf_suite_parse_args(argc, argv, &perf_options,
                                                perf_err, sizeof perf_err);
    if (perf_status != 0) {
        fprintf(stderr, "%s\n", perf_err);
        return perf_status;
    }
    if (perf_options.import_requested) {
        if (smz_perf_import_fixture(&perf_options, perf_err,
                                    sizeof perf_err) != 0) {
            fprintf(stderr, "%s\n", perf_err);
            return 1;
        }
        printf("Imported save slot %d as %s\n", perf_options.import_slot,
               perf_options.import_fixture);
        return 0;
    }

    /* Parse command-line arguments. */
    const char *game_dir = NULL;
    const char *shim_dir = "src/shims";
    int win_w = RMMZ_DEFAULT_WIDTH;
    int win_h = RMMZ_DEFAULT_HEIGHT;
    bool explicit_resolution = false;
    int no_audio = 0;
    int max_frames = 0;
    const char *screenshot_path = NULL;
    const char *probe_expr = NULL;
    int watchdog_ms = 0;
    int perf_every = 0;
    bool control_mode = false;
    bool dump_requested = false;
    const char *dump_path = NULL;
    bool share_requested = false;
    const char *share_path = NULL;
    bool cpu_prof_requested = false;
    const char *cpu_prof_path = NULL;
    const char *frame_stats_path = NULL;
#ifdef SMZUGGLER_NAME
    SmzHostDirs host_dirs = {0};
#else
    char log_path_buf[1024];
    char console_log_buf[1024];
    char log_dir_buf[1024];
#endif

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dump-surface") == 0) {
            dump_requested = true;
        } else if (strncmp(argv[i], "--dump-surface=", 15) == 0) {
            dump_requested = true;
            dump_path = argv[i] + 15;
        } else if (strcmp(argv[i], "--share-bundle") == 0) {
            share_requested = true;
        } else if (strncmp(argv[i], "--share-bundle=", 15) == 0) {
            share_requested = true;
            share_path = argv[i] + 15;
        } else if (strcmp(argv[i], "--game") == 0 && i + 1 < argc) {
            game_dir = argv[++i];
        } else if (strcmp(argv[i], "--shims") == 0 && i + 1 < argc) {
            shim_dir = argv[++i];
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            /* Debug: exit cleanly after N frames. */
            max_frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            /* Debug: write the last frame (see --frames) as a PPM file. */
            screenshot_path = argv[++i];
        } else if (strcmp(argv[i], "--probe") == 0 && i + 1 < argc) {
            /* Debug: JS expression logged when --frames stops the loop. */
            probe_expr = argv[++i];
        } else if (strcmp(argv[i], "--perf") == 0 && i + 1 < argc) {
            /* Debug: log average per-phase frame times every N frames. */
            perf_every = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--watchdog") == 0 && i + 1 < argc) {
            /* Debug: interrupt JS that blocks a frame or eval for longer than
               this many milliseconds, logging where it was. */
            watchdog_ms = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--resolution") == 0 && i + 1 < argc) {
            if (sscanf(argv[++i], "%dx%d", &win_w, &win_h) != 2) {
                fatal_error("Invalid resolution format, expected WIDTHxHEIGHT");
                return 1;
            }
            explicit_resolution = true;
        } else if (strcmp(argv[i], "--control") == 0) {
            /* Automation: take eval/screenshot commands from stdin (see
               game_loop_enable_control and tools/visual_eval.py). */
            control_mode = true;
        } else if (strcmp(argv[i], "--cpu-prof") == 0) {
            /* Record a V8 CPU profile; the default path is under the state
               dir (see js_profiler_default_path). */
            cpu_prof_requested = true;
        } else if (strncmp(argv[i], "--cpu-prof=", 11) == 0) {
            cpu_prof_requested = true;
            cpu_prof_path = argv[i] + 11;
        } else if (strcmp(argv[i], "--perf-suite") == 0 ||
                   strncmp(argv[i], "--perf-suite=", 13) == 0) {
            /* Validated with the other §14 harness options above. */
            continue;
        } else if (strcmp(argv[i], "--import-fixture") == 0) {
            /* Already consumed and dispatched before the window. */
            i += 2;
            continue;
        } else if (strncmp(argv[i], "--import-fixture=", 17) == 0) {
            continue;
        } else if (strcmp(argv[i], "--frame-stats") == 0 && i + 1 < argc) {
            /* Per-frame timing JSON Lines and summary (spec §14.3). */
            frame_stats_path = argv[++i];
        } else if (strncmp(argv[i], "--frame-stats=", 14) == 0) {
            frame_stats_path = argv[i] + 14;
        } else if (strcmp(argv[i], "--no-audio") == 0) {
            no_audio = 1;
        } else if (strcmp(argv[i], "--js-flags") == 0 && i + 1 < argc) {
            /* Validated above; skip the value so it is never the game path. */
            i++;
        } else if (strncmp(argv[i], "--js-flags=", 11) == 0) {
            /* Validated above. */
            continue;
        } else if (strcmp(argv[i], "--data-dir") == 0 ||
                   strncmp(argv[i], "--data-dir=", 11) == 0) {
            /* Log dir. smz_host_dirs / rmmz_resolve_log_dir read argv later. */
            continue;
        } else if (argv[i][0] != '-') {
            /* Positional argument: treat as game directory. */
            game_dir = argv[i];
        }
    }

    if (share_requested) {
        return share_bundle_run(share_path, argv[0]);
    }

    if (dump_requested) {
        if (strcmp(shim_dir, "src/shims") == 0) shim_dir = SMZ_SURFACE_SHIM_DIR;
        return dump_surface_run(dump_path, shim_dir);
    }

    /* No explicit paths: look for a packaged layout next to the executable
       (build_game.py produces <exe dir>/game/ with shims inside it). */
    static char default_game_dir[1024];
    static char default_shim_dir[1024];
    if (!game_dir && find_dir_next_to_exe("game", default_game_dir, sizeof(default_game_dir))) {
        game_dir = default_game_dir;
    }
#ifdef __ANDROID__
    /* Android: the game lives in the app's external files directory
       (<storage>/Android/data/<package>/files/game) and the shims are
       unpacked there from the APK's assets on every launch. */
    static char android_root[1024];
    if (android_support_files_dir(android_root, sizeof(android_root))) {
        if (!game_dir) {
            snprintf(default_game_dir, sizeof(default_game_dir), "%s/game", android_root);
            if (file_io_is_directory(default_game_dir)) game_dir = default_game_dir;
        }
        if (android_support_extract_shims(android_root, script_loader_shim_files())) {
            snprintf(default_shim_dir, sizeof(default_shim_dir), "%s/shims", android_root);
            shim_dir = default_shim_dir;
        }
    }
#endif
    if (strcmp(shim_dir, "src/shims") == 0 && !file_io_is_directory(shim_dir)) {
        if (game_dir) {
            snprintf(default_shim_dir, sizeof(default_shim_dir), "%s/shims", game_dir);
            if (file_io_is_directory(default_shim_dir)) shim_dir = default_shim_dir;
        }
        if (shim_dir != default_shim_dir &&
            find_dir_next_to_exe("shims", default_shim_dir, sizeof(default_shim_dir))) {
            shim_dir = default_shim_dir;
        }
    }

    /* Determine log file paths. smzuggler always uses the host state dir
       (or --data-dir=PATH). Upstream without SMZUGGLER_NAME keeps logs in
       the game directory unless --data-dir is passed. */
    const char *log_path = NULL;
    const char *console_log_path = NULL;
#ifdef SMZUGGLER_NAME
    smz_host_dirs(argc, argv, game_dir, &host_dirs);
    if (host_dirs.log_dir[0]) {
        file_io_mkdir(host_dirs.log_dir);
        log_path = host_dirs.debug_log;
        console_log_path = host_dirs.console_log;
    }
#else
    log_path = "debug.log";
    console_log_path = "console.log";
    if (rmmz_resolve_log_dir(argc, argv, game_dir, log_dir_buf,
                             sizeof(log_dir_buf)) &&
        log_dir_buf[0]) {
        /* The game dir already exists; the state dir or explicit PATH may not. */
        if (!game_dir || strcmp(log_dir_buf, game_dir) != 0)
            file_io_mkdir(log_dir_buf);
        snprintf(log_path_buf, sizeof(log_path_buf), "%s/debug.log", log_dir_buf);
        log_path = log_path_buf;
        snprintf(console_log_buf, sizeof(console_log_buf), "%s/console.log", log_dir_buf);
        console_log_path = console_log_buf;
    }
#endif

    /* Directory for a default --cpu-prof file: next to the logs, which are
       already the host state directory (or an explicit --data-dir). */
#ifdef SMZUGGLER_NAME
    const char *state_dir = host_dirs.log_dir;
#else
    const char *state_dir = log_dir_buf[0] ? log_dir_buf : NULL;
#endif

    /* GUI-subsystem builds start without stdout/stderr; attach to the parent
       console if there is one, otherwise capture raw output to console.log. */
    platform_setup_console(console_log_path);
    /* Progress messages are few and matter most when the process dies before a
       normal exit would flush them, so do not buffer stdout. */
    setvbuf(stdout, NULL, _IONBF, 0);

    error_handler_init(log_path, LOG_INFO);
    error_handler_log(LOG_INFO, SMZUGGLER_NAME " (RPG Maker MZ native runtime) starting");
#ifdef SMZUGGLER_NAME
    if (host_dirs.config_dir[0])
        error_handler_log(LOG_INFO, "Config directory %s", host_dirs.config_dir);
#endif

    /* Validate the game directory before opening a window, so a bad launch
       (wrong path, bare double-click) produces a clear error instead of a
       black screen. */
    if (!game_dir) {
        fatal_error("No game directory specified.\n\n"
                    "Run with:  " SMZUGGLER_BIN " --game <path to RPG Maker MZ game>\n"
                    "or place a \"game\" folder next to the executable.");
        error_handler_shutdown();
        return 1;
    }
    {
        char main_js[1024];
        snprintf(main_js, sizeof(main_js), "%s/js/main.js", game_dir);
        if (!file_io_exists(main_js)) {
            fatal_error("Not an RPG Maker MZ game directory (js/main.js not found):\n%s",
                        game_dir);
            error_handler_shutdown();
            return 1;
        }
    }

    if (perf_options.requested) {
        if (smz_perf_validate_fixture("fx-map92", perf_err,
                                      sizeof perf_err) != 0) {
            fatal_error("%s", perf_err);
            error_handler_shutdown();
            return 1;
        }
        if (!perf_prepare_scratch(perf_err, sizeof perf_err)) {
            fatal_error("%s", perf_err);
            error_handler_shutdown();
            return 1;
        }
    }

    Platform *platform = platform_init(APP_TITLE, win_w, win_h);
    if (!platform) {
        fatal_error("Failed to initialize window or OpenGL 4.5 context.\n"
                    "Check that your graphics driver is up to date.");
        error_handler_shutdown();
        return 1;
    }

    /* The window opens in raw pixels (DPI-aware), while NW.js scales the
       game with the desktop. Match that so the default window looks the
       same size as the original player's; --resolution is taken literally. */
    if (!explicit_resolution) {
        float desktop_scale = platform_get_desktop_scale(platform);
        if (desktop_scale > 1.0f) {
            win_w = (int)(win_w * desktop_scale + 0.5f);
            win_h = (int)(win_h * desktop_scale + 0.5f);
            platform_set_window_size(platform, win_w, win_h);
            error_handler_log(LOG_INFO, "Desktop scale %.2f: window sized to %dx%d",
                              desktop_scale, win_w, win_h);
        }
    }

    error_handler_log(LOG_INFO, "Window created (%dx%d). %s ready.",
                      win_w, win_h,
#ifdef RMMZ_RENDER_GPU
                      "GPU device"
#else
                      "OpenGL"
#endif
                      );

    JSEngine *js;
    if (js_flags[0] && !js_engine_set_js_flags(js_flags)) {
        fatal_error("Failed to set --js-flags before V8 started");
        platform_shutdown(platform);
        error_handler_shutdown();
        return 1;
    }
    js = js_engine_init();
    if (!js) {
        fatal_error("Failed to initialize JS engine");
        platform_shutdown(platform);
        error_handler_shutdown();
        return 1;
    }
    error_handler_log(LOG_INFO, "QuickJS engine initialized");
#ifdef SMZUGGLER_NAME
    if (host_dirs.jsc_dir[0])
        js_engine_set_cache_dir(js, host_dirs.jsc_dir, game_dir);
#endif
    if (watchdog_ms > 0) {
        js_engine_set_watchdog(js, (unsigned)watchdog_ms);
        error_handler_log(LOG_INFO, "Script watchdog enabled: %d ms", watchdog_ms);
    }

    jsb_runtime *jsb = js_engine_get_jsb(js);

    /* V8 samples only scripts once the profiler runs, so it starts before
       the first shim or game script is evaluated and stops before V8's
       isolate is destroyed. */
    js_profiler *profiler = cpu_prof_requested
                                ? start_cpu_profile(jsb, cpu_prof_path, state_dir, game_dir)
                                : NULL;

    bind_io_define(jsb);

    /* Initialize image loader (GL enabled since we have a context). */
    image_loader_init(true);
    bind_image_define(jsb);

    canvas2d_init();
    bind_canvas2d_define(jsb);

    font_manager_init();
    bind_font_define(jsb);

    bind_renderer_define(jsb);

    bind_filters_define(jsb);

    /* Register tilemap bindings (depends on renderer). */
    bind_tilemap_define(jsb);

    if (no_audio) {
        audio_engine_set_disabled(true);
    } else {
        audio_engine_init();
    }
    bind_audio_define(jsb);

    input_manager_init();
    bind_input_define(jsb);

    bind_platform_define(jsb, platform);

    effekseer_init(win_w, win_h, 8000);
    bind_effekseer_define(jsb);

    {
        extern Renderer *s_renderer;
        video_player_init(&s_renderer);
    }
    bind_video_define(jsb);

    file_io_set_game_root(game_dir);
#ifdef SMZUGGLER_NAME
    file_io_set_write_guard(true);
#endif

    int scripts_ok = 1;
    error_handler_log(LOG_INFO, "Loading game from: %s", game_dir);
    if (!script_loader_load_all(js, shim_dir, game_dir)) {
        /* A bad --shims/--game path must not produce a black window that
           exits 0. Fail loudly instead. */
        fatal_error("Failed to load game scripts from:\n%s\n\nShims: %s\n\n"
                    "Check the --game and --shims paths (see debug.log).",
                    game_dir, shim_dir);
        scripts_ok = 0;
    }

    /* Initialize display info in JS (screen size, devicePixelRatio). */
    js_engine_eval(js, "typeof __dom_initDisplayInfo === 'function' && __dom_initDisplayInfo();",
                   "<display-init>");

    /* Propagate actual window size to JS so letterbox scaling and mouse
       coordinate conversion are correct from the first frame. */
    {
        int init_w, init_h;
        platform_get_window_size(platform, &init_w, &init_h);
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "__dom_setWindowSize && __dom_setWindowSize(%d, %d);",
                 init_w, init_h);
        js_engine_eval(js, buf, "<window-size-init>");
    }

    /* A failed script load skips the loop and takes the same teardown. */
    GameLoop *loop = scripts_ok ? game_loop_create(platform, js, 60) : NULL;
    if (!loop) {
        if (scripts_ok) fatal_error("Failed to create game loop");
        video_player_shutdown();
        effekseer_shutdown();
        input_manager_shutdown();
        if (!no_audio) audio_engine_shutdown();
        font_manager_shutdown();
        canvas2d_shutdown();
        image_loader_shutdown();
        {
            extern Renderer *s_renderer;
            if (s_renderer) { renderer_destroy(s_renderer); s_renderer = NULL; }
        }
        bind_filters_shutdown();
        filters_shutdown();
        js_profiler_stop(profiler);
        js_engine_shutdown(js);
        platform_shutdown(platform);
        error_handler_shutdown();
        return 1;
    }

    if (perf_options.requested) {
        /* perf_suite_run opens one frame-stats file per scene and run. */
    } else if (frame_stats_path) {
        char scrubbed[1024];
        log_scrub(frame_stats_path, scrubbed, sizeof(scrubbed));
        if (game_loop_enable_frame_stats(loop, frame_stats_path, 60.0)) {
            error_handler_log(LOG_INFO, "Frame stats: writing to %s", scrubbed);
        } else {
            error_handler_log(LOG_ERROR, "Could not open frame stats file %s",
                              scrubbed);
        }
    }
    if (perf_every > 0) {
        game_loop_set_perf_report(loop, perf_every);
    }
    if (max_frames > 0) {
        extern Renderer *s_renderer;
        game_loop_set_frame_limit(loop, max_frames, screenshot_path, &s_renderer);
        if (probe_expr) game_loop_set_exit_probe(loop, probe_expr);
    }
    if (control_mode) {
        extern Renderer *s_renderer;
        char shim_path[1024];
        snprintf(shim_path, sizeof(shim_path), "%s/control_shim.js", shim_dir);
        if (!js_engine_eval_file(js, shim_path) ||
            !game_loop_enable_control(loop, &s_renderer)) {
            error_handler_log(LOG_ERROR, "Control mode could not be enabled");
        }
    }
    int run_status = 0;
    if (perf_options.requested) {
        if (!perf_suite_run(loop, js, &perf_options, perf_err,
                            sizeof perf_err)) {
            error_handler_log(LOG_ERROR, "%s", perf_err);
            game_loop_enable_frame_stats(loop, "", 60.0);
            run_status = 1;
        } else {
            char scrubbed[1024];
            log_scrub(perf_options.frame_stats_dir, scrubbed,
                      sizeof(scrubbed));
            error_handler_log(LOG_INFO,
                              "Perf suite finished; analyse %s with "
                              "tools/perf/analyse.py", scrubbed);
        }
    } else {
        game_loop_run(loop);
    }

    game_loop_destroy(loop);
    video_player_shutdown();
    effekseer_shutdown();
    input_manager_shutdown();
    audio_engine_shutdown();
    font_manager_shutdown();
    canvas2d_shutdown();
    image_loader_shutdown();
    /* Clean up renderer and filter GPU resources before destroying GL context. */
    {
        extern Renderer *s_renderer;
        if (s_renderer) {
            renderer_destroy(s_renderer);
            s_renderer = NULL;
        }
    }
    bind_filters_shutdown();
    filters_shutdown();
    int exit_status = run_status ? run_status : process_exit_status(js);
    js_profiler_stop(profiler);
    js_engine_shutdown(js);
    platform_shutdown(platform);
    error_handler_log(LOG_INFO, "Shutdown complete");
    error_handler_shutdown();
    return exit_status;
}

/** Run services on the process main thread with borrowed CLI arguments.
 * Returns the runtime exit status after JS and graphics teardown. */
int main(int argc, char *argv[])
{
    return runtime_main(argc, argv);
}

#endif /* RMMZ_TEST_DATA_DIR */
