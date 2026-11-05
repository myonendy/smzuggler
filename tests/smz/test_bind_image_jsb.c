/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Headless contract test for the HTMLImageElement/Bitmap native image bridge.
 * Why: dump-surface omits native modules and function arities. Expected
 * values are checked against the pre-port binding on the QuickJS engine;
 * the port runs on the selected engine, including actual V8 in v8 mode.
 * Main thread only; borrowed handles never escape calls (docs/js-binding.md).
 */
#include "bindings/bind_image.h"
#include "engine/js_engine.h"
#include "io/file_io.h"
#include "rendering/image_loader.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compare observable JS results, printing context and aborting on failure.
 * The engine is borrowed; its returned string is freed after comparison. */
static void expect(JSEngine *engine, const char *script, const char *expected)
{
    char *got = js_engine_eval_string(engine, script, "test_bind_image_jsb");
    if (!got || strcmp(got, expected)) {
        fprintf(stderr, "FAIL: %s\nexpected: %s\ngot: %s\n", script,
                expected, got ? got : "(exception)");
        abort();
    }
    js_engine_free_string(got);
}

/* Verify every own property and the old non-enumerable function descriptors. */
static void check_shape(JSEngine *engine)
{
    expect(engine, "typeof __native_image", "object");
    expect(engine, "Object.getOwnPropertyNames(__native_image).join()",
           "decodeApng,loadImage,loadImageFromMemory,getImageInfo,getImagePixels,"
           "freeImage,cacheCount,clearCache");
    expect(engine, "Object.keys(__native_image).length", "0");
    expect(engine,
           "Object.getOwnPropertyNames(__native_image).map(function(n){"
           "var f=__native_image[n],d=Object.getOwnPropertyDescriptor(__native_image,n);"
           "return [typeof f,f.length,f.name,d.writable,d.enumerable,d.configurable].join(':')"
           "}).join()",
           "function:1:decodeApng:true:false:true,function:1:loadImage:true:false:true,"
           "function:2:loadImageFromMemory:true:false:true,"
           "function:1:getImageInfo:true:false:true,function:1:getImagePixels:true:false:true,"
           "function:1:freeImage:true:false:true,function:0:cacheCount:true:false:true,"
           "function:0:clearCache:true:false:true");
}

/* Preserve the binding's explicit errors, independent of engine diagnostics. */
static void check_errors(JSEngine *engine)
{
    expect(engine,
           "var m=__native_image;function error(f){try{f();return 'no error'}"
           "catch(e){return e.name+': '+e.message}};"
           "['decodeApng','loadImage','loadImageFromMemory','getImageInfo',"
           "'getImagePixels','freeImage'].map(function(n){return error(function(){m[n]()})}).join('|')",
           "TypeError: decodeApng requires an ArrayBuffer|"
           "TypeError: loadImage requires a path argument|"
           "TypeError: loadImageFromMemory requires key and ArrayBuffer|"
           "TypeError: getImageInfo requires a handle argument|"
           "TypeError: getImagePixels requires a handle argument|"
           "TypeError: freeImage requires a handle argument");
    expect(engine,
           "[undefined,null,42,'x',{},new Uint8Array(4),new DataView(new ArrayBuffer(4))]"
           ".every(function(v){return error(function(){m.loadImageFromMemory('x',v)})==="
           "'TypeError: second argument must be an ArrayBuffer'})", "true");
    expect(engine,
           "[undefined,null,42,'x',{},new DataView(new ArrayBuffer(4))]"
           ".every(function(v){return error(function(){m.decodeApng(v)})==="
           "'TypeError: decodeApng requires an ArrayBuffer'})", "true");
    expect(engine,
           "['getImageInfo','getImagePixels','freeImage'].map(function(n){return error(function(){"
           "m[n]({valueOf:function(){throw new RangeError('handle conversion')}})})}).join('|')",
           "RangeError: handle conversion|RangeError: handle conversion|RangeError: handle conversion");
    expect(engine,
           "error(function(){m.loadImageFromMemory({toString:function(){"
           "throw new RangeError('key conversion')}},new ArrayBuffer(4))})",
           "RangeError: key conversion");
    expect(engine, "String(m.loadImage({toString:function(){throw new Error('path conversion')}}))",
           "null");
    expect(engine, "String(m.loadImage('../outside.png'))", "null");
    expect(engine, "m.cacheCount()", "0");
}

/* Transfer the repository's synthetic PNG into JS; no game material is read.
 * Both C allocations are freed here; JS owns its new ArrayBuffer. */
static void install_png(JSEngine *engine)
{
    size_t size;
    uint8_t *bytes = file_io_read_binary("tests/fixtures/test_4x4.png", &size);
    assert(bytes);
    char *script = malloc(size * 4 + 100);
    assert(script);
    size_t used = (size_t)sprintf(script, "var png=new Uint8Array([");
    for (size_t i = 0; i < size; ++i)
        used += (size_t)sprintf(script + used, "%s%u", i ? "," : "", bytes[i]);
    strcpy(script + used, "]).buffer; 'ready'");
    expect(engine, script, "ready");
    free(script);
    free(bytes);
}

/* Exercise all image functions without GL, including copied pixel ownership. */
static void check_images(JSEngine *engine)
{
    expect(engine, "String(m.loadImage('tests/fixtures/missing.png'))", "null");
    expect(engine, "String(m.loadImageFromMemory('invalid',new ArrayBuffer(4)))", "null");
    expect(engine, "[m.getImageInfo(0),m.getImagePixels('0'),m.freeImage(0)].map(String).join()",
           "null,null,undefined");
    expect(engine, "var a=m.loadImage('tests/fixtures/test_4x4.png');Object.keys(a).join()",
           "width,height,handle,glTexture");
    expect(engine, "JSON.stringify([a.width,a.height,typeof a.handle,a.handle>0,a.glTexture])",
           "[4,4,\"number\",true,0]");
    expect(engine, "JSON.stringify(m.getImageInfo(String(a.handle)))",
           "{\"width\":4,\"height\":4,\"glTexture\":0}");
    expect(engine, "var pixels=m.getImagePixels(a.handle);"
           "[pixels instanceof ArrayBuffer,pixels.byteLength].join()", "true,64");
    expect(engine, "var copy=new Uint8Array(pixels),first=copy[0];copy[0]^=255;"
           "new Uint8Array(m.getImagePixels(a.handle))[0]===first", "true");
    expect(engine, "var b=m.loadImageFromMemory({toString:function(){return 'blob:test'}},png);"
           "Object.keys(b).join()", "width,height,handle,glTexture");
    expect(engine, "JSON.stringify([b.width,b.height,b.glTexture,m.cacheCount('ignored')])",
           "[4,4,0,2]");
    expect(engine, "String(m.freeImage(String(a.handle)))", "undefined");
    expect(engine, "[m.getImageInfo(a.handle),m.getImagePixels(a.handle),m.cacheCount()].join()",
           ",,1");
    expect(engine, "String(m.clearCache('ignored'))", "undefined");
    expect(engine, "[m.cacheCount(),m.getImageInfo(b.handle)].join()", "0,");
    expect(engine, "String(m.loadImageFromMemory('empty',new ArrayBuffer(0)))", "null");
}

/* Wrap the synthetic PNG in one APNG frame (CRC ignored by the decoder).
 * Validate decoded object/array shapes and view offsets without a GL device. */
static void check_apng(JSEngine *engine)
{
    expect(engine,
           "function be(n){return [n>>>24,(n>>>16)&255,(n>>>8)&255,n&255]}"
           "function chunk(t,d){return be(d.length).concat(t.split('').map(function(c){"
           "return c.charCodeAt(0)}),d,[0,0,0,0])}"
           "var p=Array.from(new Uint8Array(png));"
           "var anim=new Uint8Array(p.slice(0,33).concat("
           "chunk('acTL',be(1).concat(be(3))),"
           "chunk('fcTL',be(0).concat(be(4),be(4),be(0),be(0),[0,7,0,100,0,0])),"
           "p.slice(33)));"
           "var apng=m.decodeApng(anim.buffer);Object.keys(apng).join()",
           "width,height,numPlays,frames");
    expect(engine, "JSON.stringify(apng)",
           "{\"width\":4,\"height\":4,\"numPlays\":3,\"frames\":[{\"delay\":70,\"glTexture\":0}]}");
    expect(engine, "Array.isArray(apng.frames)", "true");
    expect(engine, "var padded=new Uint8Array(anim.length+8);padded.set(anim,4);"
           "JSON.stringify(m.decodeApng(padded.subarray(4,4+anim.length)))===JSON.stringify(apng)",
           "true");
    expect(engine, "String(m.decodeApng(png))", "null");
    expect(engine, "String(m.decodeApng(new Uint8Array(4)))", "null");
}

/* Run on the engine-owned jsb runtime; image resources remain C-owned. */
int main(void)
{
    file_io_set_game_root(".");
    image_loader_init(false);
    JSEngine *engine = js_engine_init();
    assert(engine);
    assert(bind_image_define(js_engine_get_jsb(engine)));
    check_shape(engine);
    check_errors(engine);
    install_png(engine);
    check_images(engine);
    check_apng(engine);
    js_engine_shutdown(engine);
    image_loader_shutdown();
    file_io_set_game_root(NULL);
    puts("bind_image_jsb: all checks passed");
    return 0;
}
