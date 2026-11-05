/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 */

#include "bindings/bind_io.h"
#include "bindings/bind_platform.h"
#include "io/file_io.h"
#include "engine/js_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_paths.h"

/* Minimal test framework */

static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void name(void)
#define RUN(name) do { \
    tests_run++; \
    printf("  [%d] %s ... ", tests_run, #name); \
    name(); \
    if (tests_failed >= tests_run - tests_passed) { \
        /* test already marked as failed */ \
    } else { \
        printf("PASS\n"); \
        tests_passed++; \
    } \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    assertion failed: %s\n    at %s:%d\n", \
               #cond, __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

/* Test setup / teardown */

static const char *TEST_SAVE_DIR = "tests/fixtures/test_save";

static void setup_test_env(void)
{
    test_rm_rf(TEST_SAVE_DIR);
    test_mkdir_p(TEST_SAVE_DIR);
}

static JSEngine *s_js = NULL;

static void js_setup(void)
{
    setup_test_env();

    s_js = js_engine_init();
    bind_io_define(js_engine_get_jsb(s_js));
    bind_platform_define_process(js_engine_get_jsb(s_js));
    file_io_set_game_root(TEST_SAVE_DIR);

    /* DOM shim must precede the storage shim (provides window, document). */
    js_engine_eval_file(s_js, "src/shims/dom_shim.js");
    js_engine_execute_pending_jobs(s_js);

    js_engine_eval_file(s_js, "src/shims/storage_shim.js");
    js_engine_execute_pending_jobs(s_js);
}

static void js_teardown(void)
{
    js_engine_shutdown(s_js);
    s_js = NULL;
    test_rm_rf(TEST_SAVE_DIR);
}

static int js_check(const char *script)
{
    char *result = js_engine_eval_string(s_js, script, "<test>");
    if (!result) return 0;
    int ok = (strcmp(result, "true") == 0);
    if (!ok) {
        printf("(got: %s) ", result);
    }
    js_engine_free_string(result);
    return ok;
}

/* Tests: localStorage */

TEST(localstorage_exists)
{
    js_setup();
    ASSERT(js_check("typeof localStorage === 'object'"));
    ASSERT(js_check("typeof localStorage.getItem === 'function'"));
    ASSERT(js_check("typeof localStorage.setItem === 'function'"));
    ASSERT(js_check("typeof localStorage.removeItem === 'function'"));
    ASSERT(js_check("typeof localStorage.clear === 'function'"));
    ASSERT(js_check("typeof localStorage.key === 'function'"));
    js_teardown();
}

TEST(localstorage_set_get)
{
    js_setup();
    ASSERT(js_check(
        "localStorage.setItem('testKey', 'testValue');"
        "localStorage.getItem('testKey') === 'testValue'"
    ));
    js_teardown();
}

TEST(localstorage_get_nonexistent)
{
    js_setup();
    ASSERT(js_check("localStorage.getItem('nonexistent') === null"));
    js_teardown();
}

TEST(localstorage_remove)
{
    js_setup();
    ASSERT(js_check(
        "localStorage.setItem('removeMe', 'value');"
        "localStorage.removeItem('removeMe');"
        "localStorage.getItem('removeMe') === null"
    ));
    js_teardown();
}

TEST(localstorage_clear)
{
    js_setup();
    ASSERT(js_check(
        "localStorage.setItem('a', '1');"
        "localStorage.setItem('b', '2');"
        "localStorage.clear();"
        "localStorage.getItem('a') === null && localStorage.getItem('b') === null"
    ));
    js_teardown();
}

TEST(localstorage_length_and_key)
{
    js_setup();
    ASSERT(js_check(
        "localStorage.clear();"
        "localStorage.setItem('x', '10');"
        "localStorage.setItem('y', '20');"
        "localStorage.length === 2"
    ));
    ASSERT(js_check(
        "var k = localStorage.key(0);"
        "k === 'x' || k === 'y'"
    ));
    ASSERT(js_check("localStorage.key(99) === null"));
    js_teardown();
}

TEST(localstorage_persistence)
{
    js_setup();
    ASSERT(js_check(
        "localStorage.setItem('persist', 'hello');"
        "true"
    ));

    /* The value must be flushed to disk immediately. */
    char save_path[4096];
    snprintf(save_path, sizeof(save_path), "%s/save/localStorage.json", TEST_SAVE_DIR);
    ASSERT(file_io_exists(save_path));

    size_t size = 0;
    char *content = file_io_read_text(save_path, &size);
    ASSERT(content != NULL);
    ASSERT(strstr(content, "persist") != NULL);
    ASSERT(strstr(content, "hello") != NULL);
    free(content);

    js_teardown();
}

TEST(localstorage_coerces_to_string)
{
    js_setup();
    ASSERT(js_check(
        "localStorage.setItem('num', 42);"
        "localStorage.getItem('num') === '42'"
    ));
    js_teardown();
}

/* Tests: localforage */

TEST(localforage_exists)
{
    js_setup();
    ASSERT(js_check("typeof localforage === 'object'"));
    ASSERT(js_check("typeof localforage.setItem === 'function'"));
    ASSERT(js_check("typeof localforage.getItem === 'function'"));
    ASSERT(js_check("typeof localforage.removeItem === 'function'"));
    ASSERT(js_check("typeof localforage.keys === 'function'"));
    js_teardown();
}

TEST(localforage_set_get)
{
    js_setup();
    js_engine_eval(s_js,
        "var forageResult = null;"
        "localforage.setItem('key1', 'val1')"
        "    .then(function() { return localforage.getItem('key1'); })"
        "    .then(function(v) { forageResult = v; });",
        "<test>");
    js_engine_execute_pending_jobs(s_js);

    ASSERT(js_check("forageResult === 'val1'"));
    js_teardown();
}

TEST(localforage_get_nonexistent)
{
    js_setup();
    js_engine_eval(s_js,
        "var forageNull = 'notset';"
        "localforage.getItem('nope')"
        "    .then(function(v) { forageNull = v; });",
        "<test>");
    js_engine_execute_pending_jobs(s_js);

    ASSERT(js_check("forageNull === null"));
    js_teardown();
}

TEST(localforage_remove)
{
    js_setup();
    js_engine_eval(s_js,
        "var forageRemoved = 'not_done';"
        "localforage.setItem('rmkey', 'rmval')"
        "    .then(function() { return localforage.removeItem('rmkey'); })"
        "    .then(function() { return localforage.getItem('rmkey'); })"
        "    .then(function(v) { forageRemoved = v; });",
        "<test>");
    js_engine_execute_pending_jobs(s_js);

    ASSERT(js_check("forageRemoved === null"));
    js_teardown();
}

TEST(localforage_keys)
{
    js_setup();
    js_engine_eval(s_js,
        "var forageKeys = null;"
        "Promise.all(["
        "    localforage.setItem('k1', 'v1'),"
        "    localforage.setItem('k2', 'v2')"
        "]).then(function() { return localforage.keys(); })"
        "  .then(function(keys) { forageKeys = keys; });",
        "<test>");
    js_engine_execute_pending_jobs(s_js);

    ASSERT(js_check("Array.isArray(forageKeys)"));
    ASSERT(js_check("forageKeys.length === 2"));
    js_teardown();
}

TEST(localforage_keys_returns_original_keys)
{
    js_setup();
    /* keys() must return the original keys, not the sanitized filenames. */
    js_engine_eval(s_js,
        "var forageOrigKeys = null;"
        "localforage.setItem('rmmzsave.123456.file0', 'data1')"
        "    .then(function() { return localforage.setItem('key with spaces', 'data2'); })"
        "    .then(function() { return localforage.keys(); })"
        "    .then(function(keys) { forageOrigKeys = keys; });",
        "<test>");
    js_engine_execute_pending_jobs(s_js);

    ASSERT(js_check("Array.isArray(forageOrigKeys)"));
    ASSERT(js_check("forageOrigKeys.length === 2"));
    ASSERT(js_check("forageOrigKeys.indexOf('rmmzsave.123456.file0') >= 0"));
    ASSERT(js_check("forageOrigKeys.indexOf('key with spaces') >= 0"));
    js_teardown();
}

TEST(localforage_no_key_collision)
{
    js_setup();
    /* "a_b" and "a/b" must map to distinct filenames (injective key encoding). */
    js_engine_eval(s_js,
        "var collisionResult = {};"
        "localforage.setItem('a_b', 'underscore')"
        "    .then(function() { return localforage.setItem('a/b', 'slash'); })"
        "    .then(function() { return localforage.getItem('a_b'); })"
        "    .then(function(v) { collisionResult.u = v; })"
        "    .then(function() { return localforage.getItem('a/b'); })"
        "    .then(function(v) { collisionResult.s = v; })"
        "    .then(function() { return localforage.keys(); })"
        "    .then(function(k) { collisionResult.keys = k; });",
        "<test>");
    js_engine_execute_pending_jobs(s_js);

    ASSERT(js_check("collisionResult.u === 'underscore'"));
    ASSERT(js_check("collisionResult.s === 'slash'"));
    ASSERT(js_check("collisionResult.keys.length === 2"));
    ASSERT(js_check("collisionResult.keys.indexOf('a_b') >= 0"));
    ASSERT(js_check("collisionResult.keys.indexOf('a/b') >= 0"));
    js_teardown();
}

TEST(localforage_no_unicode_key_collision)
{
    js_setup();
    /* U+1004 and U+0100 followed by "4" must not collide; fixed-width hex
       encoding keeps them distinct ("_1004" vs "_01004"). */
    js_engine_eval(s_js,
        "var ucResult = {};"
        "localforage.setItem('\\u1004', 'single')"
        "    .then(function() { return localforage.setItem('\\u0100' + '4', 'multi'); })"
        "    .then(function() { return localforage.getItem('\\u1004'); })"
        "    .then(function(v) { ucResult.single = v; })"
        "    .then(function() { return localforage.getItem('\\u0100' + '4'); })"
        "    .then(function(v) { ucResult.multi = v; })"
        "    .then(function() { return localforage.keys(); })"
        "    .then(function(k) { ucResult.keys = k; });",
        "<test>");
    js_engine_execute_pending_jobs(s_js);

    ASSERT(js_check("ucResult.single === 'single'"));
    ASSERT(js_check("ucResult.multi === 'multi'"));
    ASSERT(js_check("ucResult.keys.length === 2"));
    js_teardown();
}

TEST(localforage_keys_after_remove)
{
    js_setup();
    /* removeItem must also drop the key from the key map. */
    js_engine_eval(s_js,
        "var forageKeysAfterRm = null;"
        "localforage.setItem('key.to.keep', 'v1')"
        "    .then(function() { return localforage.setItem('key.to.remove', 'v2'); })"
        "    .then(function() { return localforage.removeItem('key.to.remove'); })"
        "    .then(function() { return localforage.keys(); })"
        "    .then(function(keys) { forageKeysAfterRm = keys; });",
        "<test>");
    js_engine_execute_pending_jobs(s_js);

    ASSERT(js_check("Array.isArray(forageKeysAfterRm)"));
    ASSERT(js_check("forageKeysAfterRm.length === 1"));
    ASSERT(js_check("forageKeysAfterRm[0] === 'key.to.keep'"));
    js_teardown();
}

/* Tests: require("fs") */

TEST(require_fs_exists)
{
    js_setup();
    ASSERT(js_check("typeof require === 'function'"));
    ASSERT(js_check(
        "var fs = require('fs');"
        "typeof fs === 'object'"
    ));
    ASSERT(js_check("typeof fs.existsSync === 'function'"));
    ASSERT(js_check("typeof fs.mkdirSync === 'function'"));
    ASSERT(js_check("typeof fs.readFileSync === 'function'"));
    ASSERT(js_check("typeof fs.writeFileSync === 'function'"));
    ASSERT(js_check("typeof fs.unlinkSync === 'function'"));
    ASSERT(js_check("typeof fs.readdirSync === 'function'"));
    ASSERT(js_check("typeof fs.renameSync === 'function'"));
    js_teardown();
}

TEST(require_fs_write_read_roundtrip)
{
    js_setup();
    ASSERT(js_check(
        "var fs = require('fs');"
        "fs.mkdirSync('save');"
        "fs.writeFileSync('save/test.txt', 'hello world');"
        "fs.readFileSync('save/test.txt') === 'hello world'"
    ));
    js_teardown();
}

TEST(require_fs_write_read_with_encoding)
{
    js_setup();
    ASSERT(js_check(
        "var fs = require('fs');"
        "fs.mkdirSync('save');"
        "fs.writeFileSync('save/test_enc.txt', 'encoded data');"
        "fs.readFileSync('save/test_enc.txt', { encoding: 'utf8' }) === 'encoded data'"
    ));
    js_teardown();
}

TEST(require_fs_exists_sync)
{
    js_setup();
    ASSERT(js_check(
        "var fs = require('fs');"
        "fs.mkdirSync('save');"
        "fs.writeFileSync('save/exists_test.txt', 'data');"
        "fs.existsSync('save/exists_test.txt') === true"
    ));
    ASSERT(js_check("fs.existsSync('save/nonexistent.txt') === false"));
    js_teardown();
}

TEST(require_fs_unlink)
{
    js_setup();
    ASSERT(js_check(
        "var fs = require('fs');"
        "fs.mkdirSync('save');"
        "fs.writeFileSync('save/delete_me.txt', 'data');"
        "fs.unlinkSync('save/delete_me.txt');"
        "fs.existsSync('save/delete_me.txt') === false"
    ));
    js_teardown();
}

TEST(require_fs_readdir)
{
    js_setup();
    ASSERT(js_check(
        "var fs = require('fs');"
        "fs.mkdirSync('save');"
        "fs.writeFileSync('save/a.txt', 'a');"
        "fs.writeFileSync('save/b.txt', 'b');"
        "var files = fs.readdirSync('save');"
        "Array.isArray(files) && files.length >= 2"
    ));
    js_teardown();
}

TEST(require_fs_rename)
{
    js_setup();
    ASSERT(js_check(
        "var fs = require('fs');"
        "fs.mkdirSync('save');"
        "fs.writeFileSync('save/old_name.txt', 'content');"
        "fs.renameSync('save/old_name.txt', 'save/new_name.txt');"
        "fs.existsSync('save/old_name.txt') === false && "
        "fs.readFileSync('save/new_name.txt') === 'content'"
    ));
    js_teardown();
}

/* Tests: require("path") */

TEST(require_path_exists)
{
    js_setup();
    ASSERT(js_check(
        "var path = require('path');"
        "typeof path === 'object'"
    ));
    ASSERT(js_check("typeof path.join === 'function'"));
    ASSERT(js_check("typeof path.dirname === 'function'"));
    ASSERT(js_check("typeof path.basename === 'function'"));
    ASSERT(js_check("typeof path.resolve === 'function'"));
    js_teardown();
}

TEST(require_path_join)
{
    js_setup();
    ASSERT(js_check(
        "var path = require('path');"
        "path.join('a', 'b', 'c') === 'a/b/c'"
    ));
    ASSERT(js_check("path.join('a/', '/b') === 'a/b'"));
    /* Trailing separators survive, as in Node. StorageManager relies on it:
       path.join(base, "save/") + "config.rmmzsave" must land inside save/. */
    ASSERT(js_check("path.join('a', 'b/') === 'a/b/'"));
    ASSERT(js_check("path.join('/game', 'save/') + 'config.rmmzsave' === '/game/save/config.rmmzsave'"));
    js_teardown();
}

TEST(require_path_dirname)
{
    js_setup();
    ASSERT(js_check(
        "var path = require('path');"
        "path.dirname('/foo/bar/baz.js') === '/foo/bar'"
    ));
    ASSERT(js_check("path.dirname('foo.js') === '.'"));
    ASSERT(js_check("path.dirname('/foo') === '/'"));
    js_teardown();
}

TEST(require_path_basename)
{
    js_setup();
    ASSERT(js_check(
        "var path = require('path');"
        "path.basename('/foo/bar/baz.js') === 'baz.js'"
    ));
    ASSERT(js_check("path.basename('/foo/bar/baz.js', '.js') === 'baz'"));
    ASSERT(js_check("path.basename('file.txt') === 'file.txt'"));
    js_teardown();
}

TEST(require_path_resolve)
{
    js_setup();
    ASSERT(js_check(
        "var path = require('path');"
        "path.resolve('/foo', 'bar') === '/foo/bar'"
    ));
    ASSERT(js_check("path.resolve('/foo', '/bar') === '/bar'"));
    js_teardown();
}

/* Tests: require() for unknown modules */

TEST(require_unknown_module)
{
    js_setup();
    /* Unknown modules should return an empty object, not throw. */
    ASSERT(js_check(
        "var mod = require('nw.gui');"
        "typeof mod === 'object'"
    ));
    js_teardown();
}

/* Tests: process object */

TEST(process_exists)
{
    js_setup();
    ASSERT(js_check("typeof process === 'object'"));
    ASSERT(js_check("typeof process.mainModule === 'object'"));
    ASSERT(js_check("typeof process.mainModule.filename === 'string'"));
    js_teardown();
}

TEST(process_mainmodule_filename)
{
    js_setup();
    ASSERT(js_check("process.mainModule.filename.indexOf('index.html') >= 0"));
    js_teardown();
}

/* Tests: process host facts (env, platform, arch, versions, hrtime, nextTick) */

#if defined(__ANDROID__)
#define EXPECTED_PLATFORM "android"
#elif defined(_WIN32)
#define EXPECTED_PLATFORM "win32"
#elif defined(__APPLE__)
#define EXPECTED_PLATFORM "darwin"
#elif defined(__linux__)
#define EXPECTED_PLATFORM "linux"
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define EXPECTED_ARCH "x64"
#elif defined(__aarch64__) || defined(_M_ARM64)
#define EXPECTED_ARCH "arm64"
#endif

/* Set (value != NULL) or unset an environment variable. */
static void test_setenv(const char *name, const char *value)
{
#ifdef _WIN32
    _putenv_s(name, value ? value : "");
#else
    if (value) setenv(name, value, 1);
    else unsetenv(name);
#endif
}

/* Copy of an env var (or NULL) so a test can restore it afterwards. */
static char *test_getenv_copy(const char *name)
{
    const char *v = getenv(name);
    if (!v) return NULL;
    char *copy = malloc(strlen(v) + 1);
    if (copy) strcpy(copy, v);
    return copy;
}

static void test_restore_env(const char *name, char *saved)
{
    test_setenv(name, saved);
    free(saved);
}

/* Like ASSERT, then jump to a `done:` label so env restoration always runs. */
#define ASSERT_DONE(cond)                                            \
    do {                                                             \
        if (!(cond)) {                                               \
            printf("FAIL\n    assertion failed: %s\n    at %s:%d\n", \
                   #cond, __FILE__, __LINE__);                       \
            tests_failed++;                                          \
            goto done;                                               \
        }                                                            \
    } while (0)

TEST(process_env_is_snapshot_of_host)
{
    char *saved = test_getenv_copy("RMMZ_TEST_PROCESS_ENV");
    test_setenv("RMMZ_TEST_PROCESS_ENV", "a b=c");
    js_setup();
    test_setenv("RMMZ_TEST_PROCESS_ENV", "changed");
    ASSERT_DONE(js_check("process.env.RMMZ_TEST_PROCESS_ENV === 'a b=c'"));
    ASSERT_DONE(js_check("Object.getPrototypeOf(process.env) === Object.prototype"));
    ASSERT_DONE(js_check(
        "Object.keys(process.env).length > 0 &&"
        "Object.keys(process.env).every(function(k) {"
        "  return typeof process.env[k] === 'string'; })"
    ));
    ASSERT_DONE(js_check("process.env.RMMZ_TEST_PROCESS_ENV_MISSING === undefined"));
    js_teardown();
done:
    if (s_js) js_teardown();
    test_restore_env("RMMZ_TEST_PROCESS_ENV", saved);
}

TEST(process_platform_and_arch_match_target)
{
    js_setup();
#ifdef EXPECTED_PLATFORM
    ASSERT(js_check("process.platform === '" EXPECTED_PLATFORM "'"));
#endif
#ifdef EXPECTED_ARCH
    ASSERT(js_check("process.arch === '" EXPECTED_ARCH "'"));
#endif
    ASSERT(js_check("typeof process.platform === 'string' && process.platform.length > 0"));
    ASSERT(js_check("typeof process.arch === 'string' && process.arch.length > 0"));
    js_teardown();
}

TEST(process_versions_match_reference_nwjs)
{
    js_setup();
    ASSERT(js_check("process.versions.nw === '0.80.0'"));
    ASSERT(js_check("process.versions.chromium === '117.0.5938.63'"));
    ASSERT(js_check("process.versions.node === '20.7.0'"));
    ASSERT(js_check("typeof __native_process.engine === 'string' && "
                    "__native_process.engine.length > 0 && "
                    "Object.keys(process.versions).some(function (k) {"
                    "  return process.versions[k] === __native_process.engine; })"));
    js_teardown();
}

TEST(process_hrtime_shape_and_monotonic)
{
    js_setup();
    ASSERT(js_check(
        "var t = process.hrtime();"
        "t instanceof Array && t.length === 2 &&"
        "t[0] === Math.floor(t[0]) && t[0] >= 0 &&"
        "t[1] === Math.floor(t[1]) && t[1] >= 0 && t[1] < 1e9"
    ));
    ASSERT(js_check(
        "var prev = process.hrtime(), ok = true;"
        "for (var i = 0; i < 2000; i++) {"
        "  var cur = process.hrtime();"
        "  if (cur[0] < prev[0] || (cur[0] === prev[0] && cur[1] < prev[1])) ok = false;"
        "  prev = cur;"
        "}"
        "ok"
    ));
    ASSERT(js_check(
        "var start = process.hrtime(); var x = 0;"
        "for (var j = 0; j < 100000; j++) x += j;"
        "var d = process.hrtime(start);"
        "d.length === 2 && d[0] >= 0 && d[1] >= 0 && d[1] < 1e9 && (d[0] > 0 || d[1] > 0)"
    ));
    ASSERT(js_check(
        "var threw = false;"
        "try { process.hrtime(5); } catch (e) { threw = e instanceof TypeError; }"
        "threw"
    ));
    ASSERT(js_check(
        "typeof BigInt !== 'function' || (function() {"
        "  var a = process.hrtime.bigint(), b = process.hrtime.bigint();"
        "  return typeof a === 'bigint' && b >= a; })()"
    ));
    js_teardown();
}

TEST(process_next_tick_order_like_node)
{
    js_setup();
    /* Node runs this as N1,N2,P1,P2,T. */
    ASSERT(js_check(
        "var order = [];"
        "setTimeout(function() { order.push('T'); }, 0);"
        "process.nextTick(function(a, b) {"
        "  order.push('N1' + a + b);"
        "  Promise.resolve().then(function() { order.push('P2'); });"
        "  process.nextTick(function() { order.push('N2'); });"
        "}, ':', 'x');"
        "Promise.resolve().then(function() { order.push('P1'); });"
        "order.length === 0"
    ));
    js_engine_execute_pending_jobs(s_js);
    ASSERT(js_check("order.join(',') === 'N1:x,N2,P1,P2'"));
    ASSERT(js_check("__dom_flushTimers(); order.join(',') === 'N1:x,N2,P1,P2,T'"));
    ASSERT(js_check(
        "var threw = false;"
        "try { process.nextTick(5); } catch (e) { threw = e instanceof TypeError; }"
        "threw"
    ));
    js_teardown();
}

TEST(process_next_tick_continues_after_throw)
{
    js_setup();
    ASSERT(js_check(
        "var ran = false;"
        "process.nextTick(function() { throw new Error('tick'); });"
        "process.nextTick(function() { ran = true; });"
        "ran === false"
    ));
    js_engine_execute_pending_jobs(s_js);
    ASSERT(js_check("ran === true"));
    js_teardown();
}

TEST(process_next_tick_after_already_queued_promise)
{
    js_setup();
    /* A Promise job queued before the first nextTick of a turn still runs
       first: ticks are drained by Promise.resolve().then, so this is P,N.
       Node would run N,P. */
    ASSERT(js_check(
        "var order = [];"
        "Promise.resolve().then(function() { order.push('P'); });"
        "process.nextTick(function() { order.push('N'); });"
        "order.length === 0"));
    js_engine_execute_pending_jobs(s_js);
    ASSERT(js_check("order.join(',') === 'P,N'"));
    js_teardown();
}

/* Stand-in for how save-location plugins pick the user data directory from
   process.platform and process.env (not any game's plugin). */
static const char *SAVE_DIR_SNIPPET =
    "function saveDir() {"
    "  var env = process.env, base;"
    "  if (process.platform === 'win32') base = env.APPDATA;"
    "  else if (process.platform === 'darwin') base = env.HOME + '/Library/Application Support';"
    "  else base = env.XDG_DATA_HOME || (env.HOME + '/.local/share');"
    "  return base + '/StandInSaves/';"
    "}"
    "true";

TEST(process_env_gives_real_save_dir)
{
    char *home = test_getenv_copy("HOME");
    char *xdg = test_getenv_copy("XDG_DATA_HOME");

    test_setenv("HOME", "/tmp/rmmz-home");
    test_setenv("XDG_DATA_HOME", NULL);
    js_setup();
    ASSERT_DONE(js_check(SAVE_DIR_SNIPPET));
    ASSERT_DONE(js_check("saveDir().indexOf('undefined') === -1"));
#if defined(__linux__) && !defined(__ANDROID__)
    ASSERT_DONE(js_check("saveDir() === '/tmp/rmmz-home/.local/share/StandInSaves/'"));
#endif
    js_teardown();

    test_setenv("XDG_DATA_HOME", "/tmp/rmmz-xdg-data");
    js_setup();
    ASSERT_DONE(js_check(SAVE_DIR_SNIPPET));
    ASSERT_DONE(js_check("saveDir().indexOf('undefined') === -1"));
#if defined(__linux__) && !defined(__ANDROID__)
    ASSERT_DONE(js_check("saveDir() === '/tmp/rmmz-xdg-data/StandInSaves/'"));
#endif
    js_teardown();
done:
    if (s_js) js_teardown();
    test_restore_env("HOME", home);
    test_restore_env("XDG_DATA_HOME", xdg);
}

/* With the save-path guard on, the save sequence of a save-location plugin
   (exists check on the folder, mkdir, write, exists check on the file, read)
   must land under $XDG_DATA_HOME/<name>/ and read back, with nothing written
   in the game root. */
TEST(guarded_save_lands_in_xdg_data_home)
{
    char *home = test_getenv_copy("HOME");
    char *xdg = test_getenv_copy("XDG_DATA_HOME");
    char scratch[TEST_PATH_MAX], scratch_home[TEST_PATH_MAX];
    char scratch_xdg[TEST_PATH_MAX], save_file[TEST_PATH_MAX * 2];
    snprintf(scratch, sizeof(scratch), "%s/rmmz_test_storage_guarded_save", test_tmp_root());
    snprintf(scratch_home, sizeof(scratch_home), "%s/home", scratch);
    snprintf(scratch_xdg, sizeof(scratch_xdg), "%s/xdg-data", scratch);
    snprintf(save_file, sizeof(save_file), "%s/StandInSaves/file1.rmmzsave", scratch_xdg);
    test_rm_rf(scratch);
    test_mkdir_p(scratch_home);
    test_mkdir_p(scratch_xdg);

    test_setenv("HOME", scratch_home);
    test_setenv("XDG_DATA_HOME", scratch_xdg);
    js_setup();
    file_io_set_write_guard(true);
    ASSERT_DONE(js_check(SAVE_DIR_SNIPPET));
#if defined(__linux__) && !defined(__ANDROID__)
    ASSERT_DONE(js_check(
        "var fs = require('fs');"
        "var dirPath = saveDir();"
        "var filePath = dirPath + 'file1.rmmzsave';"
        "var saveData = 'compressed-save-data-here';"
        "var dirBefore = fs.existsSync(dirPath);"
        "if (!dirBefore) fs.mkdirSync(dirPath, { recursive: true });"
        "fs.writeFileSync(filePath, saveData);"
        "!dirBefore && fs.existsSync(filePath) &&"
        "  fs.readFileSync(filePath, { encoding: 'utf8' }) === saveData"));
    char *on_disk = file_io_read_text(save_file, NULL);
    int saved = on_disk && strcmp(on_disk, "compressed-save-data-here") == 0;
    free(on_disk);
    ASSERT_DONE(saved);
    size_t root_entries = 0;
    char **list = file_io_readdir(TEST_SAVE_DIR, &root_entries);
    int root_listed = list != NULL;
    file_io_free_dirlist(list);
    ASSERT_DONE(root_listed && root_entries == 0);
#endif
done:
    file_io_set_write_guard(false);
    if (s_js) js_teardown();
    test_restore_env("HOME", home);
    test_restore_env("XDG_DATA_HOME", xdg);
    test_rm_rf(scratch);
}

#undef ASSERT_DONE

/* Tests: Utils.isNwjs() pattern */

TEST(utils_is_nwjs_pattern)
{
    js_setup();
    /* The exact check RPG Maker MZ's Utils.isNwjs() performs. */
    ASSERT(js_check(
        "typeof require === 'function' && typeof process === 'object'"
    ));
    js_teardown();
}

/* Tests: StorageManager simulation (end-to-end) */

TEST(storage_manager_file_directory_path)
{
    js_setup();
    /* Mirrors StorageManager.fileDirectoryPath(). */
    ASSERT(js_check(
        "var path = require('path');"
        "var base = path.dirname(process.mainModule.filename);"
        "var dirPath = path.join(base, 'save/');"
        "typeof dirPath === 'string' && dirPath.indexOf('save') >= 0"
    ));
    js_teardown();
}

TEST(storage_manager_save_load_roundtrip)
{
    js_setup();
    /* Mirrors the StorageManager save/load flow: mkdir, write, read back. */
    ASSERT(js_check(
        "var fs = require('fs');"
        "var path = require('path');"
        "var base = path.dirname(process.mainModule.filename);"
        "var dirPath = path.join(base, 'save/');"
        "if (!fs.existsSync(dirPath)) fs.mkdirSync(dirPath);"
        "var filePath = dirPath + 'file0.rmmzsave';"
        "var saveData = 'compressed-save-data-here';"
        "fs.writeFileSync(filePath, saveData);"
        "var loaded = fs.readFileSync(filePath, { encoding: 'utf8' });"
        "loaded === saveData"
    ));
    js_teardown();
}

TEST(storage_manager_backup_rename_pattern)
{
    js_setup();
    /* Mirrors the backup/rename pattern in StorageManager.saveToLocalFile. */
    ASSERT(js_check(
        "var fs = require('fs');"
        "fs.mkdirSync('save');"
        "var filePath = 'save/test.rmmzsave';"
        "var backupPath = filePath + '_';"
        ""
        "/* Create initial save */"
        "fs.writeFileSync(filePath, 'save-v1');"
        ""
        "/* Simulate backup-rename-write pattern */"
        "if (fs.existsSync(backupPath)) fs.unlinkSync(backupPath);"
        "if (fs.existsSync(filePath)) fs.renameSync(filePath, backupPath);"
        "fs.writeFileSync(filePath, 'save-v2');"
        "if (fs.existsSync(backupPath)) fs.unlinkSync(backupPath);"
        ""
        "fs.readFileSync(filePath) === 'save-v2' && !fs.existsSync(backupPath)"
    ));
    js_teardown();
}

/* Tests: pako verification */

TEST(pako_works_in_js_engine)
{
    js_setup();

    /* pako.min.js is taken from the test game (it ships with every RMMZ game). */
    if (!test_game_dir()) {
        printf("SKIP (RMMZ_TEST_GAME_DIR not set) ");
        js_teardown();
        return;
    }
    char pako_path[TEST_PATH_MAX];
    snprintf(pako_path, sizeof(pako_path), "%s/js/libs/pako.min.js", test_game_dir());
    file_io_set_game_root(test_game_dir());
    bool loaded = js_engine_eval_file(s_js, pako_path);
    js_engine_execute_pending_jobs(s_js);

    if (!loaded) {
        printf("SKIP (pako.min.js not found) ");
        js_teardown();
        return;
    }

    ASSERT(js_check(
        "typeof pako === 'object' && typeof pako.deflate === 'function'"
    ));

    ASSERT(js_check(
        "var input = 'Hello, RPG Maker MZ save system!';"
        "var compressed = pako.deflate(input, { to: 'string', level: 1 });"
        "compressed.length > 0"
    ));

    ASSERT(js_check(
        "var decompressed = pako.inflate(compressed, { to: 'string' });"
        "decompressed === input"
    ));

    js_teardown();
}

/* Tests: nw shim */

TEST(nw_shim_exists)
{
    js_setup();
    ASSERT(js_check("typeof nw === 'object'"));
    ASSERT(js_check("typeof nw.Window === 'object'"));
    ASSERT(js_check("typeof nw.Window.get === 'function'"));
    ASSERT(js_check(
        "var win = nw.Window.get();"
        "typeof win === 'object' && typeof win.on === 'function'"
    ));
    js_teardown();
}

TEST(nw_window_get_is_singleton)
{
    js_setup();
    ASSERT(js_check("nw.Window.get() === nw.Window.get()"));
    ASSERT(js_check(
        "nw.Window.get().on('x', function() {});"
        "nw.Window.get().listeners('x').length === 1"
    ));
    js_teardown();
}

TEST(nw_window_listener_order)
{
    js_setup();
    ASSERT(js_check(
        "var w = nw.Window.get(), log = [];"
        "w.on('ev', function(a) { log.push('a' + a); });"
        "w.addListener('ev', function(a) { log.push('b' + a); });"
        "w.once('ev', function(a) { log.push('c' + a); });"
        "w.emit('ev', 1);"
        "log.join(',') === 'a1,b1,c1'"
    ));
    js_teardown();
}

TEST(nw_window_once_fires_once)
{
    js_setup();
    ASSERT(js_check(
        "var w = nw.Window.get(), n = 0;"
        "w.once('ev', function() { n++; });"
        "w.emit('ev'); w.emit('ev');"
        "n === 1 && w.listeners('ev').length === 0"
    ));
    ASSERT(js_check(
        "var w = nw.Window.get(), n = 0, f = function() { n++; };"
        "w.on('dup', f); w.once('dup', f);"
        "w.emit('dup'); w.emit('dup');"
        "n === 3 && w.listeners('dup').length === 1"
    ));
    js_teardown();
}

TEST(nw_window_off_removes_one_registration)
{
    js_setup();
    ASSERT(js_check(
        "var w = nw.Window.get(), n = 0, f = function() { n++; };"
        "w.on('ev', f); w.on('ev', f);"
        "w.off('ev', f); w.emit('ev');"
        "n === 1 && w.listeners('ev').length === 1"
    ));
    ASSERT(js_check(
        "n = 0; w.removeListener('ev', f); w.emit('ev');"
        "n === 0 && w.listeners('ev').length === 0"
    ));
    ASSERT(js_check(
        "var w = nw.Window.get(), g = function() {};"
        "w.off('missing', g) === w && w.removeListener('missing', g) === w"
    ));
    js_teardown();
}

TEST(nw_window_remove_all_listeners)
{
    js_setup();
    ASSERT(js_check(
        "var w = nw.Window.get(), f = function() {};"
        "w.on('close', f); w.on('close', f); w.on('blur', f);"
        "w.removeAllListeners('close') === w &&"
        "w.listeners('close').length === 0 && w.listeners('blur').length === 1"
    ));
    ASSERT(js_check(
        "var w = nw.Window.get(), f = function() {};"
        "w.on('close', f);"
        "w.removeAllListeners() === w &&"
        "w.listeners('close').length === 0 && w.listeners('blur').length === 0"
    ));
    js_teardown();
}

TEST(nw_window_emit_reports_listeners)
{
    js_setup();
    ASSERT(js_check("nw.Window.get().emit('nobody') === false"));
    ASSERT(js_check(
        "var w = nw.Window.get(), got = null;"
        "w.on('ev', function(a, b) { got = [this === w, a, b]; });"
        "w.emit('ev', 2, 3) === true && got[0] && got[1] === 2 && got[2] === 3"
    ));
    js_teardown();
}

TEST(nw_window_listeners_returns_copy)
{
    js_setup();
    ASSERT(js_check(
        "var w = nw.Window.get(), f = function() {};"
        "w.on('ev', f);"
        "var l = w.listeners('ev'); l.push(f); l.length = 0;"
        "w.listeners('ev').length === 1 && w.listeners('ev')[0] === f &&"
        "w.listeners('none').length === 0"
    ));
    js_teardown();
}

TEST(nw_window_methods_return_this)
{
    js_setup();
    ASSERT(js_check(
        "var w = nw.Window.get(), f = function() {};"
        "w.on('a', f) === w && w.addListener('a', f) === w && w.once('a', f) === w"
    ));
    ASSERT(js_check(
        "var threw = false;"
        "try { nw.Window.get().on('a', 5); } catch (e) { threw = e instanceof TypeError; }"
        "threw"
    ));
    js_teardown();
}

TEST(nw_window_clean_exit_pattern)
{
    js_setup();
    /* removeAllListeners('close') then on('close', ...); close() emits instead of quitting. */
    ASSERT(js_check(
        "var w = nw.Window.get(), hit = 0;"
        "w.on('close', function() { hit += 10; });"
        "w.removeAllListeners('close');"
        "w.on('close', function() { hit++; });"
        "w.close();"
        "hit === 1"
    ));
    js_teardown();
}

TEST(nw_window_methods_exist)
{
    js_setup();
    ASSERT(js_check(
        "var w = nw.Window.get(), names = ['closeDevTools', 'close', 'focus', 'show',"
        " 'hide', 'maximize', 'restore', 'enterFullscreen', 'leaveFullscreen',"
        " 'toggleFullscreen'], ok = true;"
        "for (var i = 0; i < names.length; i++) ok = ok && typeof w[names[i]] === 'function';"
        "ok && typeof w.width === 'number' && typeof w.height === 'number' &&"
        "typeof w.x === 'number' && typeof w.y === 'number' &&"
        "typeof w.title === 'string' && w.zoomLevel === 0"
    ));
    ASSERT(js_check(
        "var w = nw.Window.get();"
        "w.enterFullscreen(); var a = w.isFullscreen;"
        "w.toggleFullscreen(); var b = w.isFullscreen;"
        "w.toggleFullscreen(); w.leaveFullscreen();"
        "a === true && b === false && w.isFullscreen === false"
    ));
    js_teardown();
}

/* Main */

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    printf("=== Storage Shim Tests ===\n");

    printf("\n--- localStorage tests ---\n");
    RUN(localstorage_exists);
    RUN(localstorage_set_get);
    RUN(localstorage_get_nonexistent);
    RUN(localstorage_remove);
    RUN(localstorage_clear);
    RUN(localstorage_length_and_key);
    RUN(localstorage_persistence);
    RUN(localstorage_coerces_to_string);

    printf("\n--- localforage tests ---\n");
    RUN(localforage_exists);
    RUN(localforage_set_get);
    RUN(localforage_get_nonexistent);
    RUN(localforage_remove);
    RUN(localforage_keys);
    RUN(localforage_keys_returns_original_keys);
    RUN(localforage_no_key_collision);
    RUN(localforage_no_unicode_key_collision);
    RUN(localforage_keys_after_remove);

    printf("\n--- require('fs') tests ---\n");
    RUN(require_fs_exists);
    RUN(require_fs_write_read_roundtrip);
    RUN(require_fs_write_read_with_encoding);
    RUN(require_fs_exists_sync);
    RUN(require_fs_unlink);
    RUN(require_fs_readdir);
    RUN(require_fs_rename);

    printf("\n--- require('path') tests ---\n");
    RUN(require_path_exists);
    RUN(require_path_join);
    RUN(require_path_dirname);
    RUN(require_path_basename);
    RUN(require_path_resolve);

    printf("\n--- require() unknown modules ---\n");
    RUN(require_unknown_module);

    printf("\n--- process object tests ---\n");
    RUN(process_exists);
    RUN(process_mainmodule_filename);
    RUN(process_env_is_snapshot_of_host);
    RUN(process_platform_and_arch_match_target);
    RUN(process_versions_match_reference_nwjs);
    RUN(process_hrtime_shape_and_monotonic);
    RUN(process_next_tick_order_like_node);
    RUN(process_next_tick_continues_after_throw);
    RUN(process_next_tick_after_already_queued_promise);
    RUN(process_env_gives_real_save_dir);
    RUN(guarded_save_lands_in_xdg_data_home);

    printf("\n--- RPG Maker MZ compatibility tests ---\n");
    RUN(utils_is_nwjs_pattern);
    RUN(storage_manager_file_directory_path);
    RUN(storage_manager_save_load_roundtrip);
    RUN(storage_manager_backup_rename_pattern);

    printf("\n--- pako integration tests ---\n");
    RUN(pako_works_in_js_engine);

    printf("\n--- nw shim tests ---\n");
    RUN(nw_shim_exists);
    RUN(nw_window_get_is_singleton);
    RUN(nw_window_listener_order);
    RUN(nw_window_once_fires_once);
    RUN(nw_window_off_removes_one_registration);
    RUN(nw_window_remove_all_listeners);
    RUN(nw_window_emit_reports_listeners);
    RUN(nw_window_listeners_returns_copy);
    RUN(nw_window_methods_return_this);
    RUN(nw_window_clean_exit_pattern);
    RUN(nw_window_methods_exist);

    printf("\n=== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(", %d FAILED", tests_failed);
    }
    printf(" ===\n");

    return tests_failed > 0 ? 1 : 0;
}
