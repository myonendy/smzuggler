/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Tests the jsb zero-copy buffer calls (src/jsb/jsb.h): jsb_arg_bytes,
 * jsb_new_external_buffer and jsb_adjust_external_memory.
 *
 * Why: ported bindings pass pixel, audio and file data through these
 * without copying, so view offsets, detached buffers, shared writes and the
 * exactly-once free callback must hold in both engine modes (docs/
 * js-binding.md, "Buffers"). gc() comes from the "--expose-gc" engine flag.
 * Handle rules: no jsb_val is kept past the native call that made it
 * (spec section 15.8, docs/js-binding.md). Main thread only, except the
 * free callback, which V8 may run on a GC helper thread (hence the atomic).
 */
#include "jsb.h"

#include <assert.h>

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXT_LEN 16

/* The native memory currently exposed by fn_ext (NULL when none). */
static uint8_t *g_ext;

/* How many times ext_free has run. */
static atomic_int g_frees;

/* Runs `src` and asserts that it completes with String(result) == `expect`;
 * prints the mismatch first so a failing assert says what went wrong. */
static void expect_js(jsb_runtime *rt, const char *src, const char *expect) {
    char out[512];
    bool ok = jsb_runtime_eval(rt, src, "test_jsb_buffers.js", out, sizeof out);
    if (!ok || strcmp(out, expect) != 0)
        fprintf(stderr, "FAIL: %s\n  expected %s, got %s%s\n", src, expect,
                ok ? "" : "exception ", out);
    assert(ok);
    assert(strcmp(out, expect) == 0);
}

/* Returns the byte length jsb_arg_bytes reports for arg0. */
static void fn_size(jsb_ctx *ctx) {
    uint8_t *data;
    size_t len;
    if (!jsb_arg_bytes(ctx, 0, &data, &len)) return;
    jsb_ret_f64(ctx, (double)len);
}

/* Returns byte arg1 of arg0's bytes as C sees them, or throws a RangeError
 * past the end. */
static void fn_peek(jsb_ctx *ctx) {
    uint8_t *data;
    size_t len;
    int32_t i;
    if (!jsb_arg_bytes(ctx, 0, &data, &len) || !jsb_arg_i32(ctx, 1, &i))
        return;
    if (i < 0 || (size_t)i >= len) {
        jsb_throw_range(ctx, "peek: index %d outside %zu bytes", i, len);
        return;
    }
    jsb_ret_i32(ctx, data[i]);
}

/* Writes byte value arg1 over every byte of arg0 from C. */
static void fn_fill(jsb_ctx *ctx) {
    uint8_t *data;
    size_t len;
    int32_t v;
    if (!jsb_arg_bytes(ctx, 0, &data, &len) || !jsb_arg_i32(ctx, 1, &v))
        return;
    if (len) memset(data, v & 0xff, len);
}

/* Frees fn_ext's memory and counts the call; `user` must be &g_frees. */
static void ext_free(void *data, size_t len, void *user) {
    assert(len == EXT_LEN);
    assert(user == (void *)&g_frees);
    free(data);
    atomic_fetch_add((atomic_int *)user, 1);
}

/* Returns an external ArrayBuffer over 16 malloc'd bytes 0..15. */
static void fn_ext(jsb_ctx *ctx) {
    uint8_t *mem = malloc(EXT_LEN);
    assert(mem);
    for (int i = 0; i < EXT_LEN; i++) mem[i] = (uint8_t)i;
    jsb_val *buf = jsb_new_external_buffer(ctx, mem, EXT_LEN, ext_free,
                                           &g_frees);
    if (!buf) {
        free(mem);
        return;
    }
    g_ext = mem;
    jsb_ret_val(ctx, buf);
}

/* Returns whether arg0's bytes are fn_ext's memory itself (no copy). */
static void fn_same(jsb_ctx *ctx) {
    uint8_t *data;
    size_t len;
    if (!jsb_arg_bytes(ctx, 0, &data, &len)) return;
    jsb_ret_bool(ctx, data == g_ext && len == EXT_LEN);
}

/* Writes arg1 into byte arg0 of fn_ext's memory directly from C. */
static void fn_poke(jsb_ctx *ctx) {
    int32_t i, v;
    if (!jsb_arg_i32(ctx, 0, &i) || !jsb_arg_i32(ctx, 1, &v)) return;
    assert(g_ext && i >= 0 && i < EXT_LEN);
    g_ext[i] = (uint8_t)v;
}

/* Reads byte arg0 of fn_ext's memory directly from C. */
static void fn_ext_peek(jsb_ctx *ctx) {
    int32_t i;
    if (!jsb_arg_i32(ctx, 0, &i)) return;
    assert(g_ext && i >= 0 && i < EXT_LEN);
    jsb_ret_i32(ctx, g_ext[i]);
}

/* Calls jsb_new_external_buffer with NULL data or (arg0 true) a NULL free
 * function; both must throw a TypeError and return NULL. */
static void fn_bad_ext(jsb_ctx *ctx) {
    static uint8_t mem[4];
    bool null_free;
    if (!jsb_arg_bool(ctx, 0, &null_free)) return;
    jsb_val *buf = null_free
        ? jsb_new_external_buffer(ctx, mem, sizeof mem, NULL, NULL)
        : jsb_new_external_buffer(ctx, NULL, 4, ext_free, &g_frees);
    assert(buf == NULL);
}

/* Returns jsb_adjust_external_memory(arg0). */
static void fn_adjust(jsb_ctx *ctx) {
    double change;
    if (!jsb_arg_f64(ctx, 0, &change)) return;
    jsb_ret_f64(ctx, (double)jsb_adjust_external_memory(ctx, (int64_t)change));
}

/* Fills the __native_buf test module. */
static bool init_module(jsb_ctx *ctx, jsb_val *m) {
    return jsb_set_fn(ctx, m, "size", fn_size) &&
           jsb_set_fn(ctx, m, "peek", fn_peek) &&
           jsb_set_fn(ctx, m, "fill", fn_fill) &&
           jsb_set_fn(ctx, m, "ext", fn_ext) &&
           jsb_set_fn(ctx, m, "same", fn_same) &&
           jsb_set_fn(ctx, m, "poke", fn_poke) &&
           jsb_set_fn(ctx, m, "extPeek", fn_ext_peek) &&
           jsb_set_fn(ctx, m, "badExt", fn_bad_ext) &&
           jsb_set_fn(ctx, m, "adjust", fn_adjust);
}

/* Setup callback: defines __native_buf. */
static bool setup(jsb_ctx *ctx, void *user) {
    (void)user;
    return jsb_define_module(ctx, "__native_buf", init_module);
}

/* Views with offsets: C sees the view's own bytes, not the whole buffer,
 * and C writes land in the right place of the shared ArrayBuffer. */
static void test_views(jsb_runtime *rt) {
    expect_js(rt, "var B = __native_buf; var ab = new ArrayBuffer(16);"
                  "var u = new Uint8Array(ab);"
                  "for (var i = 0; i < 16; i++) u[i] = i; B.size(ab)", "16");
    expect_js(rt, "B.size(new Uint8Array(ab, 4, 8))", "8");
    expect_js(rt, "B.peek(new Uint8Array(ab, 4, 8), 0)", "4");
    expect_js(rt, "B.size(new Uint16Array(ab, 2, 3))", "6");
    expect_js(rt, "B.peek(new Uint16Array(ab, 2, 3), 5)", "7");
    expect_js(rt, "B.size(new DataView(ab, 6, 2))", "2");
    expect_js(rt, "B.peek(new DataView(ab, 6, 2), 1)", "7");
    expect_js(rt, "B.size(new Float32Array(0))", "0");
    expect_js(rt, "B.fill(new Uint8Array(ab, 10, 2), 255);"
                  "[u[9], u[10], u[11], u[12]].join()", "9,255,255,12");
    expect_js(rt, "B.fill(new DataView(ab, 1, 1), 77); [u[0], u[1], u[2]]"
                  ".join()", "0,77,2");
    expect_js(rt, "B.fill(ab, 3); u[0] + u[15]", "6");
    expect_js(rt, "B.peek(ab, 15)", "3");
}

/* Non-buffers and detached buffers (and views on them) are TypeErrors. */
static void test_errors(jsb_runtime *rt) {
    expect_js(rt, "function te(f) { try { f(); return 'none'; }"
                  " catch (e) { return e instanceof TypeError; } }"
                  "[te(function () { B.size(); }),"
                  " te(function () { B.size(16); }),"
                  " te(function () { B.size([1, 2]); }),"
                  " te(function () { B.size({}); })].join()",
              "true,true,true,true");
    expect_js(rt, "var d = new ArrayBuffer(8); var dv = new Uint8Array(d, 2);"
                  "var dd = new DataView(d); var moved = d.transfer();"
                  "[d.detached, B.size(moved), te(function () { B.size(d); }),"
                  " te(function () { B.size(dv); }),"
                  " te(function () { B.fill(dd, 1); })].join()",
              "true,8,true,true,true");
    expect_js(rt, "[te(function () { B.badExt(false); }),"
                  " te(function () { B.badExt(true); })].join()", "true,true");
}

/* Calls gc() until the free count reaches `want`, at most 8 times: V8 may
 * finish freeing backing stores on a helper thread, and the next full GC
 * waits for that work. Returns the final count. */
static int collect_until(jsb_runtime *rt, int want) {
    for (int i = 0; i < 8 && atomic_load(&g_frees) < want; i++)
        expect_js(rt, "gc(); 0", "0");
    return atomic_load(&g_frees);
}

/* External buffers: same memory both ways, freed once after GC. */
static void test_external(jsb_runtime *rt) {
    expect_js(rt, "var e = B.ext(); var eu = new Uint8Array(e);"
                  "[e.byteLength, eu[3], B.same(e)].join()", "16,3,true");
    expect_js(rt, "B.poke(5, 200); eu[5]", "200");
    expect_js(rt, "eu[7] = 99; B.extPeek(7)", "99");
    expect_js(rt, "B.same(new Uint8Array(e, 0, 16))", "true");
    assert(collect_until(rt, 1) == 0); /* still referenced: not freed */
    expect_js(rt, "e = null; eu = null; 0", "0");
    assert(collect_until(rt, 1) == 1);
    assert(collect_until(rt, 2) == 1); /* never twice */
    /* A second buffer still alive at shutdown is freed by destroy. */
    expect_js(rt, "var keep = B.ext(); keep.byteLength", "16");
}

/* External memory accounting adds up and refuses to go negative. */
static void test_adjust(jsb_runtime *rt) {
    expect_js(rt, "[B.adjust(1000), B.adjust(-400), B.adjust(-1000),"
                  " B.adjust(-600)].join()", "1000,600,-1,0");
}

int main(int argc, char **argv) {
    (void)argc;
    assert(jsb_set_flags("--expose-gc"));
    jsb_runtime *rt = jsb_runtime_create(argv[0], SMZ_V8_ICU_DATA_PATH);
    assert(rt);
    assert(!jsb_set_flags("--expose-gc")); /* too late once started */
    assert(jsb_runtime_setup(rt, setup, NULL));
    test_views(rt);
    test_errors(rt);
    test_external(rt);
    test_adjust(rt);
    jsb_runtime_destroy(rt);
    assert(atomic_load(&g_frees) == 2);
    puts("jsb_buffers: ok");
    return 0;
}
