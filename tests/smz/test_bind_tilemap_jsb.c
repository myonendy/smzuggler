/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Checks that __native_tilemap, ported to jsb by card M1-T025, keeps its JS
 * API byte-for-byte: property names, the function's typeof and length, its
 * result with and without a renderer, and argument errors.
 *
 * Why: --dump-surface lists no native modules and no `length`
 * (DEC-M0-T009b), so this test is what shows the port changed nothing
 * (DEC-M1-T014-JSB-ADOPT). Every expected value below was printed by the
 * pre-port QuickJS binary (bind_tilemap_register on js_engine.c). It calls
 * bind_tilemap_define on js_engine_get_jsb(): the QuickJS engine in quickjs
 * mode, smz_js_engine_v8 in v8 mode, so the port really runs on V8 there.
 * The renderer is created headless, so no graphics device is needed. Main
 * thread only; see docs/js-binding.md for the handle rules.
 */
#include "bindings/bind_tilemap.h"
#include "bindings/bind_renderer.h"
#include "engine/js_engine.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static JSEngine *s_js;

/* Evaluates `src` and asserts that String(result) equals `want`.
 *
 * Why: one helper keeps each check a single line and prints both sides on a
 * mismatch, so a failing check names what changed. Aborts on failure. */
static void expect(const char *src, const char *want)
{
    char *got = js_engine_eval_string(s_js, src, "test_bind_tilemap_jsb");
    if (!got || strcmp(got, want) != 0) {
        fprintf(stderr, "FAIL: %s\n  want: %s\n  got:  %s\n", src, want,
                got ? got : "(exception)");
    }
    assert(got && strcmp(got, want) == 0);
    js_engine_free_string(got);
}

/* Evaluates `call` inside try/catch and asserts the thrown error's
 * `name: message`. */
static void expect_throw(const char *call, const char *name,
                         const char *message)
{
    char src[1024], want[512];
    snprintf(src, sizeof(src),
             "(function(){try{%s;return 'no throw';}catch(e){"
             "return e.name + ': ' + e.message;}})()", call);
    snprintf(want, sizeof(want), "%s: %s", name, message);
    expect(src, want);
}

/* Names, order, typeof, length and the old non-enumerable descriptors. */
static void check_shape(void)
{
    expect("typeof __native_tilemap", "object");
    expect("Object.getOwnPropertyNames(__native_tilemap).join()", "drawTiles");
    expect("Object.keys(__native_tilemap).length", "0");
    expect("var f=__native_tilemap.drawTiles;"
           "var d=Object.getOwnPropertyDescriptor(__native_tilemap,"
           "'drawTiles');"
           "[typeof f,f.length,f.name,d.writable,d.enumerable,"
           "d.configurable].join(':')",
           "function:8:drawTiles:true:false:true");
}

/* With no renderer, drawTiles is a no-op that returns undefined. Its
 * argument-count check still runs first. */
static void check_no_device(void)
{
    expect("String(__native_tilemap.drawTiles(new ArrayBuffer(56), 1, [0],"
           "[256], [256], 1, 0, 0))", "undefined");
    expect_throw("__native_tilemap.drawTiles()", "TypeError",
                 "drawTiles requires 8 arguments");
    expect_throw("__native_tilemap.drawTiles(1,2,3,4,5,6,7)", "TypeError",
                 "drawTiles requires 8 arguments");
}

/* Argument errors, the pre-port TypeErrors and RangeError. The renderer is
 * created first: without one drawTiles returns before reading any argument
 * beyond the count. */
static void check_errors(void)
{
    expect("__native_renderer.init(816, 624)", "true");

    expect_throw("__native_tilemap.drawTiles()", "TypeError",
                 "drawTiles requires 8 arguments");
    expect_throw("__native_tilemap.drawTiles(1,2,3,4,5,6,7)", "TypeError",
                 "drawTiles requires 8 arguments");

    /* Upstream's buffer read accepted only an ArrayBuffer, not a view. */
    expect("[undefined,null,42,'x',{},new Uint8Array(56),"
           "new DataView(new ArrayBuffer(56))].every(function(v){"
           "try{__native_tilemap.drawTiles(v,1,[0],[256],[256],1,0,0);"
           "return false}"
           "catch(e){return e.name==='TypeError'&&e.message==="
           "'drawTiles: arg 0 must be an ArrayBuffer'}})", "true");
    expect_throw("__native_tilemap.drawTiles(new ArrayBuffer(28), 2, [0],"
                 "[256], [256], 1, 0, 0)", "RangeError",
                 "drawTiles: buffer too small for 2 elements");

    /* Conversions run before the element buffer is read, in upstream's
     * order, and their exceptions propagate unchanged. */
    expect_throw("__native_tilemap.drawTiles(new ArrayBuffer(56),"
                 "{valueOf:function(){throw new RangeError('count')}},"
                 "[0],[256],[256],1,0,0)", "RangeError", "count");
    expect_throw("__native_tilemap.drawTiles(new ArrayBuffer(56),1,"
                 "[{valueOf:function(){throw new RangeError('tid')}}],"
                 "[256],[256],1,0,0)", "RangeError", "tid");
    expect_throw("__native_tilemap.drawTiles(new ArrayBuffer(56),1,[0],"
                 "[{valueOf:function(){throw new RangeError('tw')}}],"
                 "[256],1,0,0)", "RangeError", "tw");
    expect_throw("__native_tilemap.drawTiles(new ArrayBuffer(56),1,[0],"
                 "[256],[{valueOf:function(){throw new RangeError('th')}}],"
                 "1,0,0)", "RangeError", "th");
    expect_throw("__native_tilemap.drawTiles(new ArrayBuffer(56),1,[0],"
                 "[256],[256],1,0,"
                 "{valueOf:function(){throw new RangeError('offsetY')}})",
                 "RangeError", "offsetY");
}

/* Result shapes with a renderer: no tiles, no tileset and a valid layer all
 * return undefined; arguments are coerced as upstream coerced them. */
static void check_results(void)
{
    expect("String(__native_tilemap.drawTiles(42, 0, 0, 0, 0, 1, 0, 0))",
           "undefined");
    expect("String(__native_tilemap.drawTiles(42, 1, 0, 0, 0, 0, 0, 0))",
           "undefined");
    expect("var b=new Float32Array([-1,0,0,10,20,24,24]);"
           "String(__native_tilemap.drawTiles(b.buffer, 1, [], [], [], 0, 0,"
           "0))", "undefined");
    expect("var b=new Float32Array([0,0,0,10,20,48,48]);"
           "String(__native_tilemap.drawTiles(b.buffer, '1', ['0'], ['256'],"
           "['256'], '1', '-100.5', '-50.5'))", "undefined");
    expect("var b=new Float32Array(14);"
           "String(__native_tilemap.drawTiles(b.buffer, 2, [0], [256], [256],"
           "1, 0, 0))", "undefined");
}

int main(void)
{
    s_js = js_engine_init();
    assert(s_js);
    assert(bind_tilemap_define(js_engine_get_jsb(s_js)));
    assert(bind_renderer_define(js_engine_get_jsb(s_js)));

    check_shape();
    check_no_device();
    check_errors();
    check_results();

    js_engine_eval(s_js, "__native_renderer.shutdown();", "<teardown>");
    js_engine_shutdown(s_js);
    printf("bind_tilemap_jsb: all checks passed\n");
    return 0;
}
