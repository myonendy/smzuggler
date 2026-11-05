/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * `--cpu-prof` writes a Chrome DevTools `.cpuprofile` (card M1-T028, spec
 * section 9 milestone M1).
 *
 * Why: a profile is only useful if the file parses, carries the fields the
 * DevTools Performance panel reads (`nodes`, `samples`, `timeDeltas`), names
 * the functions that actually ran, and states script URLs the way NW.js
 * shows them in DevTools, relative to the game root. This test starts the
 * profiler the way main.c does, runs a small hot script through the real V8
 * engine, stops the profiler and checks the written file. It also checks the
 * default profile path helper. Checks use assert() and the fragment compiles
 * with NDEBUG undefined, as the other tests/smz tests do.
 */

#include "engine/js_engine.h"
#include "engine/js_profiler.h"
#include "jsb.h"
#include "test_paths.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <process.h>
#define test_getpid _getpid
#else
#include <unistd.h>
#define test_getpid getpid
#endif

/* Prints what the next assert checks, so a failing run names it even when
   the abort message is cut off. */
static void note(const char *what)
{
    printf("  check: %s\n", what);
    fflush(stdout);
}

/* Skips JSON whitespace; returns the first significant byte. */
static const char *json_space(const char *p)
{
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
}

/* Validates a JSON string at `p` (which must be the opening quote) and
   returns the byte after its closing quote, or NULL when malformed. */
static const char *json_string(const char *p)
{
    if (*p != '"') return NULL;
    p++;
    while (*p && *p != '"') {
        if ((unsigned char)*p < 0x20) return NULL;
        if (*p == '\\') {
            p++;
            if (*p == 'u') {
                int i;
                for (i = 1; i <= 4; i++)
                    if (!isxdigit((unsigned char)p[i])) return NULL;
                p += 4;
            } else if (!*p || !strchr("\"\\/bfnrt", *p)) {
                return NULL;
            }
        }
        p++;
    }
    return *p == '"' ? p + 1 : NULL;
}

/* Validates a JSON number at `p` and returns the byte after it, or NULL. */
static const char *json_number(const char *p)
{
    if (*p == '-') p++;
    if (*p == '0') {
        p++;
    } else if (*p >= '1' && *p <= '9') {
        while (isdigit((unsigned char)*p)) p++;
    } else {
        return NULL;
    }
    if (*p == '.') {
        p++;
        if (!isdigit((unsigned char)*p)) return NULL;
        while (isdigit((unsigned char)*p)) p++;
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!isdigit((unsigned char)*p)) return NULL;
        while (isdigit((unsigned char)*p)) p++;
    }
    return p;
}

/* Validates a JSON value at `p` (any type) and returns the byte after it. */
static const char *json_value(const char *p);

/* Validates a JSON array at `p` (the opening bracket) and returns the byte
   after the closing bracket, or NULL when malformed. */
static const char *json_array(const char *p)
{
    p = json_space(p + 1);
    if (*p == ']') return p + 1;
    for (;;) {
        p = json_value(p);
        if (!p) return NULL;
        p = json_space(p);
        if (*p == ',') {
            p = json_space(p + 1);
            continue;
        }
        return *p == ']' ? p + 1 : NULL;
    }
}

/* Validates a JSON object at `p` (the opening brace); keys must be strings
   followed by a colon and a value. Returns the byte after the closing brace. */
static const char *json_object(const char *p)
{
    p = json_space(p + 1);
    if (*p == '}') return p + 1;
    for (;;) {
        p = json_string(p);
        if (!p) return NULL;
        p = json_space(p);
        if (*p != ':') return NULL;
        p = json_value(json_space(p + 1));
        if (!p) return NULL;
        p = json_space(p);
        if (*p == ',') {
            p = json_space(p + 1);
            continue;
        }
        return *p == '}' ? p + 1 : NULL;
    }
}

/* Dispatches on the first byte of a JSON value; see the helpers above. */
static const char *json_value(const char *p)
{
    p = json_space(p);
    switch (*p) {
    case '{': return json_object(p);
    case '[': return json_array(p);
    case '"': return json_string(p);
    case 't': return strncmp(p, "true", 4) == 0 ? p + 4 : NULL;
    case 'f': return strncmp(p, "false", 5) == 0 ? p + 5 : NULL;
    case 'n': return strncmp(p, "null", 4) == 0 ? p + 4 : NULL;
    default: return json_number(p);
    }
}

/* Returns 1 when `text` is exactly one well-formed JSON value. */
static int json_valid(const char *text)
{
    const char *end = json_value(text);
    return end && *json_space(end) == '\0';
}

/* Reads the whole file at `path` into malloc'd memory (free with free);
   returns NULL when it cannot be opened or read. */
static char *read_text(const char *path)
{
    FILE *f = fopen(path, "rb");
    long size;
    char *buffer;
    size_t got;
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    got = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    if (got != (size_t)size) {
        free(buffer);
        return NULL;
    }
    buffer[got] = '\0';
    return buffer;
}

/* Writes `text` to `path`; returns 1 on success. */
static int write_text(const char *path, const char *text)
{
    FILE *f = fopen(path, "wb");
    int ok;
    if (!f) return 0;
    ok = fputs(text, f) >= 0;
    return fclose(f) == 0 && ok;
}

/* Runs one hot script with a recording, then checks the written profile. */
static void test_profile(void)
{
    static const char script[] =
        "function cpu_prof_marker(n) {\n"
        "    var total = 0;\n"
        "    for (var i = 0; i < n; i++) total = (total + i * 3) % 1000003;\n"
        "    return total;\n"
        "}\n"
        "var keep = 0;\n"
        "for (var j = 0; j < 600; j++) keep += cpu_prof_marker(20000);\n";
    char root[TEST_PATH_MAX];
    char js_dir[TEST_PATH_MAX];
    char script_path[TEST_PATH_MAX];
    char profile_path[TEST_PATH_MAX];
    char *json;
    JSEngine *engine;
    js_profiler *profiler;

    snprintf(root, sizeof root, "%s/smz-cpuprof-%ld", test_tmp_root(),
             (long)test_getpid());
    snprintf(js_dir, sizeof js_dir, "%s/js", root);
    snprintf(script_path, sizeof script_path, "%s/main.js", js_dir);
    snprintf(profile_path, sizeof profile_path, "%s/profiles/run.cpuprofile", root);
    test_rm_rf(root);
    note("scratch game directory exists");
    assert(test_mkdir_p(js_dir));
    assert(write_text(script_path, script));

    engine = js_engine_init();
    note("V8 engine starts");
    assert(engine != NULL);
    note("profiler starts on the engine's isolate");
    profiler = js_profiler_start(js_engine_get_jsb(engine), profile_path, root);
    assert(profiler != NULL);
    note("hot script evaluates");
    assert(js_engine_eval_file(engine, script_path));
    note("profiler stops and writes the file");
    assert(js_profiler_stop(profiler));

    json = read_text(profile_path);
    note("profile file exists and is readable");
    assert(json != NULL);
    note("profile is valid JSON");
    assert(json_valid(json));
    note("profile has nodes, samples and timeDeltas");
    assert(strstr(json, "\"nodes\":") != NULL);
    assert(strstr(json, "\"samples\":") != NULL);
    assert(strstr(json, "\"timeDeltas\":") != NULL);
    note("profile has a node for the test function");
    assert(strstr(json, "\"functionName\":\"cpu_prof_marker\"") != NULL);
    note("script URL is game-relative");
    assert(strstr(json, "\"url\":\"js/main.js\"") != NULL);
    note("no absolute game path survives in the profile");
    assert(strstr(json, root) == NULL);
    assert(strstr(json, script_path) == NULL);
    free(json);

    js_engine_shutdown(engine);
    test_rm_rf(root);
}

/* Checks the shape of the default path under a state directory. */
static void test_default_path(void)
{
    char path[TEST_PATH_MAX];
    size_t len;
    note("default path sits under <state>/profiles and ends .cpuprofile");
    assert(js_profiler_default_path(test_tmp_path("smz-state"), path, sizeof path));
    assert(strncmp(path, test_tmp_path("smz-state"), strlen(test_tmp_path("smz-state"))) == 0);
    assert(strstr(path, "/profiles/cpu-") != NULL);
    len = strlen(path);
    assert(len > 11 && strcmp(path + len - 11, ".cpuprofile") == 0);
    note("default path rejects missing arguments");
    assert(!js_profiler_default_path(NULL, path, sizeof path));
    assert(!js_profiler_default_path("", path, sizeof path));
    assert(!js_profiler_default_path("state", path, 4));
}

/* Runs both checks; the process exit status is 0 when every assert held. */
int main(void)
{
    test_default_path();
    test_profile();
    printf("cpu profiler checks passed\n");
    return 0;
}
