/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Checks the jsb port of bind_renderer.c (card M1-T023) on the engine's own
 * jsb runtime: QuickJS in quickjs mode, smz_js_engine_v8 in v8 mode.
 *
 * Why: --dump-surface lists no native modules and no function `length`
 * (DEC-M0-T009b), so this test is what shows __native_renderer kept the
 * pre-port JS API: property names and order, typeof and length, the result
 * shape of every function that runs without a device, and wrong-argument
 * errors. The expected values are those of the pre-port QuickJS binding.
 * No graphics device is created, so the renderer's GL calls stay out of the
 * test; the device path only exercises the CPU-side bookkeeping. Main
 * thread only.
 */

#include "bindings/bind_renderer.h"
#include "engine/js_engine.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static JSEngine *s_js;

/* Evaluates `src` and asserts that String(result) equals `want`. */
static void expect(const char *src, const char *want)
{
    char *got = js_engine_eval_string(s_js, src, "bind_renderer_jsb.js");
    if (!got || strcmp(got, want) != 0) {
        fprintf(stderr, "FAIL: %s\n  want: %s\n  got:  %s\n", src, want,
                got ? got : "(exception)");
    }
    assert(got && strcmp(got, want) == 0);
    js_engine_free_string(got);
}

/* Evaluates `call` inside try/catch and asserts the thrown error's
 * `name: message` (an empty message checks only the name). */
static void expect_throw(const char *call, const char *name, const char *message)
{
    char src[512], want[256];
    snprintf(src, sizeof(src),
             "(function(){try{%s;return 'no throw';}catch(e){"
             "return e.name + (%d ? ': ' + e.message : '');}})()",
             call, message[0] != '\0');
    snprintf(want, sizeof(want), message[0] ? "%s: %s" : "%s%s", name, message);
    expect(src, want);
}

/* Names, order, typeof, length and the old non-enumerable descriptors. */
static void check_shape(void)
{
    expect("typeof __native_renderer", "object");
    expect("Object.getOwnPropertyNames(__native_renderer).join()",
           "init,shutdown,resize,getSize,beginFrame,beginFrameTransparent,"
           "endFrame,createTexture,updateTexture,deleteTexture,"
           "setTextureFilter,createRenderTexture,deleteRenderTexture,"
           "bindRenderTexture,unbindRenderTexture,drawQuad,drawQuadVertices,"
           "drawQuadVerticesUV,flush,setBlendMode,getDrawCalls,getQuadCount,"
           "readPixels,rebindBatch,setScissor,clearScissor,setScreenViewport");
    expect("Object.keys(__native_renderer).length", "0");
    expect("Object.getOwnPropertyNames(__native_renderer).map(function(n){"
           "var f=__native_renderer[n],d=Object.getOwnPropertyDescriptor("
           "__native_renderer,n);"
           "return [typeof f,f.length,f.name,d.writable,d.enumerable,"
           "d.configurable].join(':')}).join()",
           "function:2:init:true:false:true,"
           "function:0:shutdown:true:false:true,"
           "function:2:resize:true:false:true,"
           "function:0:getSize:true:false:true,"
           "function:0:beginFrame:true:false:true,"
           "function:0:beginFrameTransparent:true:false:true,"
           "function:0:endFrame:true:false:true,"
           "function:2:createTexture:true:false:true,"
           "function:4:updateTexture:true:false:true,"
           "function:1:deleteTexture:true:false:true,"
           "function:2:setTextureFilter:true:false:true,"
           "function:2:createRenderTexture:true:false:true,"
           "function:2:deleteRenderTexture:true:false:true,"
           "function:3:bindRenderTexture:true:false:true,"
           "function:0:unbindRenderTexture:true:false:true,"
           "function:11:drawQuad:true:false:true,"
           "function:15:drawQuadVertices:true:false:true,"
           "function:19:drawQuadVerticesUV:true:false:true,"
           "function:0:flush:true:false:true,"
           "function:1:setBlendMode:true:false:true,"
           "function:0:getDrawCalls:true:false:true,"
           "function:0:getQuadCount:true:false:true,"
           "function:3:readPixels:true:false:true,"
           "function:0:rebindBatch:true:false:true,"
           "function:4:setScissor:true:false:true,"
           "function:0:clearScissor:true:false:true,"
           "function:4:setScreenViewport:true:false:true");
}

/* Missing arguments and bad buffer/range arguments, as the old binding
 * reported them. These run before init(), so no renderer exists. */
static void check_errors(void)
{
    expect_throw("__native_renderer.init()", "TypeError",
                 "init requires width and height");
    expect_throw("__native_renderer.resize()", "TypeError",
                 "resize requires width and height");
    expect_throw("__native_renderer.createTexture()", "TypeError",
                 "createTexture requires width and height");
    expect_throw("__native_renderer.updateTexture()", "TypeError",
                 "updateTexture requires texId, w, h, pixels");
    expect_throw("__native_renderer.deleteTexture()", "TypeError",
                 "deleteTexture requires textureId");
    expect_throw("__native_renderer.setTextureFilter()", "TypeError",
                 "setTextureFilter requires texture and linear");
    expect_throw("__native_renderer.createRenderTexture()", "TypeError",
                 "createRenderTexture requires width and height");
    expect_throw("__native_renderer.deleteRenderTexture()", "TypeError",
                 "deleteRenderTexture requires fbo and texture");
    expect_throw("__native_renderer.bindRenderTexture()", "TypeError",
                 "bindRenderTexture requires fbo, width, height");
    expect_throw("__native_renderer.drawQuad()", "TypeError",
                 "drawQuad requires 11 arguments");
    expect_throw("__native_renderer.drawQuadVertices()", "TypeError",
                 "drawQuadVertices requires 15 arguments");
    expect_throw("__native_renderer.drawQuadVerticesUV()", "TypeError",
                 "drawQuadVerticesUV requires 19 arguments");
    expect_throw("__native_renderer.setBlendMode()", "TypeError",
                 "setBlendMode requires mode");
    expect_throw("__native_renderer.readPixels()", "TypeError",
                 "readPixels requires fbo, width, height");
    expect_throw("__native_renderer.setScissor()", "TypeError",
                 "setScissor requires x, y, w, h");
    expect_throw("__native_renderer.setScreenViewport()", "TypeError",
                 "setScreenViewport requires x, y, w, h");

    expect("[undefined,null,42,'x',{},new Uint8Array(16),"
           "new DataView(new ArrayBuffer(16))].every(function(v){"
           "try{__native_renderer.updateTexture(0,2,2,v);return false}"
           "catch(e){return e.name==='TypeError'&&e.message==="
           "'updateTexture: fourth arg must be ArrayBuffer'}})", "true");

    expect_throw("__native_renderer.createTexture(0,2)", "RangeError",
                 "createTexture: dimensions out of range 0x2");
    expect_throw("__native_renderer.createTexture(-1,2)", "RangeError",
                 "createTexture: dimensions out of range -1x2");
    expect_throw("__native_renderer.createRenderTexture(0,2)", "RangeError",
                 "createRenderTexture: dimensions out of range 0x2");
    expect_throw("__native_renderer.updateTexture(0,2,2,new ArrayBuffer(4))",
                 "RangeError", "updateTexture: buffer too small for 2x2");
    expect_throw("__native_renderer.updateTexture(0,0,2,new ArrayBuffer(16))",
                 "RangeError", "updateTexture: buffer too small for 0x2");

    expect_throw("__native_renderer.init({valueOf:function(){"
                 "throw new RangeError('w')}},2)", "RangeError", "w");
    expect_throw("__native_renderer.createTexture({valueOf:function(){"
                 "throw new RangeError('w')}},2)", "RangeError", "w");
    expect_throw("__native_renderer.updateTexture({valueOf:function(){"
                 "throw new RangeError('t')}},2,2,new ArrayBuffer(16))",
                 "RangeError", "t");
    expect_throw("__native_renderer.setTextureFilter({valueOf:function(){"
                 "throw new RangeError('t')}},true)", "RangeError", "t");
}

/* Every function's result with no renderer: the pre-port defaults. */
static void check_no_device(void)
{
    expect("String(__native_renderer.shutdown())", "undefined");
    expect("String(__native_renderer.resize(2,2))", "undefined");
    expect("String(__native_renderer.getSize())", "null");
    expect("String(__native_renderer.beginFrame())", "undefined");
    expect("String(__native_renderer.beginFrameTransparent())", "undefined");
    expect("String(__native_renderer.endFrame())", "undefined");
    expect("__native_renderer.createTexture(2,2)", "0");
    expect("String(__native_renderer.updateTexture(0,2,2,"
           "new ArrayBuffer(16)))", "undefined");
    expect("String(__native_renderer.deleteTexture(0))", "undefined");
    expect("String(__native_renderer.setTextureFilter(0,true))", "undefined");
    expect("JSON.stringify(__native_renderer.createRenderTexture(2,2))",
           "{\"fbo\":0,\"texture\":0}");
    expect("String(__native_renderer.deleteRenderTexture(0,0))", "undefined");
    expect("String(__native_renderer.bindRenderTexture(0,2,2))", "undefined");
    expect("String(__native_renderer.unbindRenderTexture())", "undefined");
    expect("String(__native_renderer.drawQuad(0,0,0,1,1,0,0,1,1,0,1))",
           "undefined");
    expect("String(__native_renderer.drawQuadVertices("
           "0,0,0,1,0,1,1,0,1,0,0,1,1,0,1))", "undefined");
    expect("String(__native_renderer.drawQuadVerticesUV("
           "0,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,1,0,1))", "undefined");
    expect("String(__native_renderer.flush())", "undefined");
    expect("String(__native_renderer.setBlendMode(0))", "undefined");
    expect("__native_renderer.getDrawCalls()", "0");
    expect("__native_renderer.getQuadCount()", "0");
    expect("String(__native_renderer.readPixels(0,2,2))", "null");
    expect("String(__native_renderer.rebindBatch())", "undefined");
    expect("String(__native_renderer.setScissor(0,0,2,2))", "undefined");
    expect("String(__native_renderer.clearScissor())", "undefined");
    expect("String(__native_renderer.setScreenViewport(0,0,2,2))",
           "undefined");
}

/* With a renderer: coercion, size round trips, readPixels and its range
 * check. No GL is present, so texture ids stay 0 and readPixels returns a
 * zero-filled CPU buffer. */
static void check_with_device(void)
{
    expect("__native_renderer.init('3','4')", "true");
    expect("JSON.stringify(__native_renderer.getSize())",
           "{\"width\":3,\"height\":4}");
    expect("String(__native_renderer.resize('5','6'))", "undefined");
    expect("JSON.stringify(__native_renderer.getSize())",
           "{\"width\":5,\"height\":6}");
    expect("__native_renderer.createTexture(2,2)", "0");
    expect("JSON.stringify(__native_renderer.createRenderTexture(2,2))",
           "{\"fbo\":0,\"texture\":0}");
    expect("var p=__native_renderer.readPixels(0,2,2);"
           "[p instanceof ArrayBuffer,p.byteLength].join()", "true,16");
    expect_throw("__native_renderer.readPixels(0,0,2)", "RangeError",
                 "readPixels: dimensions too large 0x2");
    expect_throw("__native_renderer.readPixels(0,2,0)", "RangeError",
                 "readPixels: dimensions too large 2x0");
    expect_throw("__native_renderer.readPixels({valueOf:function(){"
                 "throw new RangeError('f')}},2,2)", "RangeError", "f");
    expect_throw("__native_renderer.drawQuad({valueOf:function(){"
                 "throw new RangeError('t')}},0,0,1,1,0,0,1,1,0,1)",
                 "RangeError", "t");
    expect_throw("__native_renderer.setBlendMode({valueOf:function(){"
                 "throw new RangeError('m')}})", "RangeError", "m");
    expect("String(__native_renderer.shutdown())", "undefined");
    expect("String(__native_renderer.getSize())", "null");
}

int main(void)
{
    s_js = js_engine_init();
    assert(s_js);
    jsb_runtime *rt = js_engine_get_jsb(s_js);
    assert(rt);

    assert(bind_renderer_define(rt));
    check_shape();
    check_errors();
    check_no_device();
    check_with_device();

    js_engine_shutdown(s_js);
    printf("bind_renderer_jsb: all checks passed\n");
    return 0;
}
