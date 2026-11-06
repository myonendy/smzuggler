/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Exercises WebGL2 texture uploads from typed arrays and Canvas2D.
 *
 * Why: PIXI's Bitmap path uploads CPU-canvas pixels after every draw and
 * probes UNPACK_FLIP_Y_WEBGL/PREMULTIPLY_ALPHA_WEBGL; a stub would silently
 * show the wrong art. Each upload is read back from an FBO attachment so the
 * test checks pixels, not just that a call returned. It skips only when no GL
 * device exists; a GUI run must pass. Main thread only.
 */
#include "bindings/bind_canvas2d.h"
#include "engine/js_engine.h"
#include "rendering/canvas2d.h"
#include "webgl/webgl.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Evaluates `script` and compares String(result) with `want`. */
static void expect(JSEngine *engine, const char *script, const char *want)
{
    assert(engine != NULL);
    assert(script != NULL);
    assert(want != NULL);
    char *got = js_engine_eval_string(engine, script, "webgl_textures.js");
    if (!got || strcmp(got, want) != 0) {
        fprintf(stderr, "webgl_textures: expected %s, got %s\n  script: %s\n",
                want, got ? got : "(exception)", script);
        exit(1);
    }
    js_engine_free_string(got);
}

/* Builds the canvas/context and the readback helper shared by every check. */
static void setup_context(JSEngine *engine)
{
    expect(engine,
           "var cnv=document.createElement('canvas');"
           "cnv.width=8;cnv.height=8;"
           "var gl=cnv.getContext('webgl2');"
           "typeof gl==='object'?'ok':'missing'",
           "ok");
    expect(engine,
           "function uploadRead(tex,upload){"
           "gl.bindTexture(gl.TEXTURE_2D,tex);"
           "upload();"
           "var fbo=gl.createFramebuffer();"
           "gl.bindFramebuffer(gl.FRAMEBUFFER,fbo);"
           "gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,"
           "gl.TEXTURE_2D,tex,0);"
           "var status=gl.checkFramebufferStatus(gl.FRAMEBUFFER);"
           "var px=new Uint8Array(16);"
           "gl.readPixels(0,0,2,2,gl.RGBA,gl.UNSIGNED_BYTE,px);"
           "gl.bindFramebuffer(gl.FRAMEBUFFER,null);"
           "gl.deleteFramebuffer(fbo);"
           "return (status===gl.FRAMEBUFFER_COMPLETE)+'|'+"
           "Array.from(px).join(',');}"
           "'ok'",
           "ok");
}

/* Typed-array upload, sub-image update and the readback round trip. */
static void check_typed_array(JSEngine *engine)
{
    expect(engine,
           "var taTex=gl.createTexture();"
           "var taData=new Uint8Array([255,0,0,255,0,255,0,255,"
           "0,0,255,255,255,255,0,255]);"
           "uploadRead(taTex,function(){gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,"
           "2,2,0,gl.RGBA,gl.UNSIGNED_BYTE,taData);})",
           "true|255,0,0,255,0,255,0,255,0,0,255,255,255,255,0,255");
    expect(engine,
           "gl.bindTexture(gl.TEXTURE_2D,taTex);"
           "gl.texSubImage2D(gl.TEXTURE_2D,0,0,0,1,1,gl.RGBA,gl.UNSIGNED_BYTE,"
           "new Uint8Array([1,2,3,4]));"
           "var subFbo=gl.createFramebuffer();"
           "gl.bindFramebuffer(gl.FRAMEBUFFER,subFbo);"
           "gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,"
           "gl.TEXTURE_2D,taTex,0);"
           "var subPx=new Uint8Array(4);"
           "gl.readPixels(0,0,1,1,gl.RGBA,gl.UNSIGNED_BYTE,subPx);"
           "gl.bindFramebuffer(gl.FRAMEBUFFER,null);"
           "gl.deleteFramebuffer(subFbo);"
           "gl.deleteTexture(taTex);"
           "Array.from(subPx).join(',')",
           "1,2,3,4");
}

/* Canvas2D upload: top row red, bottom row blue in the canvas. */
static void check_canvas_source(JSEngine *engine)
{
    expect(engine,
           "var cv=document.createElement('canvas');cv.width=2;cv.height=2;"
           "var c2=cv.getContext('2d');"
           "c2.fillStyle='red';c2.fillRect(0,0,2,1);"
           "c2.fillStyle='blue';c2.fillRect(0,1,2,1);"
           "var cvTex=gl.createTexture();"
           "uploadRead(cvTex,function(){gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA,"
           "gl.RGBA,gl.UNSIGNED_BYTE,cv);})",
           "true|255,0,0,255,255,0,0,255,0,0,255,255,0,0,255,255");
    /* FLIP_Y reverses the rows of a DOM source (typed arrays ignore it). */
    expect(engine,
           "gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL,true);"
           "var flipTex=gl.createTexture();"
           "var flipped=uploadRead(flipTex,function(){gl.texImage2D("
           "gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,cv);});"
           "gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL,false);"
           "gl.deleteTexture(flipTex);"
           "flipped",
           "true|0,0,255,255,0,0,255,255,255,0,0,255,255,0,0,255");
    expect(engine,
           "var ignoreTex=gl.createTexture();"
           "var ignoreData=new Uint8Array([1,2,3,4,5,6,7,8,9,10,11,12,"
           "13,14,15,16]);"
           "gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL,true);"
           "var ignored=uploadRead(ignoreTex,function(){gl.texImage2D("
           "gl.TEXTURE_2D,0,gl.RGBA,2,2,0,gl.RGBA,gl.UNSIGNED_BYTE,"
           "ignoreData);});"
           "gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL,false);"
           "gl.deleteTexture(ignoreTex);"
           "ignored",
           "true|1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16");
}

/* Premultiplied uploading of a half-transparent canvas pixel. */
static void check_premultiply(JSEngine *engine)
{
    expect(engine,
           "var pv=document.createElement('canvas');pv.width=1;pv.height=1;"
           "var p2=pv.getContext('2d');"
           "p2.fillStyle='rgba(255,0,0,0.5)';p2.fillRect(0,0,1,1);"
           "gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL,true);"
           "var premTex=gl.createTexture();"
           "var prem=uploadRead(premTex,function(){gl.texImage2D("
           "gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,pv);});"
           "gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL,false);"
           "gl.deleteTexture(premTex);"
           "var parts=prem.split('|')[1].split(',');"
           "parts[3]+'|'+(parts[0]>=127&&parts[0]<=128)",
           "128|true");
    /* The same flag must not touch an ArrayBufferView upload. */
    expect(engine,
           "var plainTex=gl.createTexture();"
           "gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL,true);"
           "var plain=uploadRead(plainTex,function(){gl.texImage2D("
           "gl.TEXTURE_2D,0,gl.RGBA,2,2,0,gl.RGBA,gl.UNSIGNED_BYTE,"
           "new Uint8Array([255,0,0,128,0,0,0,0,0,0,0,0,0,0,0,0]));});"
           "gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL,false);"
           "gl.deleteTexture(plainTex);"
           "plain.split('|')[1].split(',')[0]",
           "255");
}

/* ImageData-like objects and the deleted-wrapper error path. */
static void check_image_data_and_lifetime(JSEngine *engine)
{
    expect(engine,
           "var idTex=gl.createTexture();"
           "var imageData={width:1,height:1,"
           "data:new Uint8Array([10,20,30,40])};"
           "var idResult=uploadRead(idTex,function(){gl.texImage2D("
           "gl.TEXTURE_2D,0,gl.RGBA,gl.RGBA,gl.UNSIGNED_BYTE,imageData);});"
           "gl.deleteTexture(idTex);"
           "idResult.split('|')[1].split(',').slice(0,4).join(',')",
           "10,20,30,40");
    expect(engine,
           "while(gl.getError()!==gl.NO_ERROR){}"
           "var dead=gl.createTexture();"
           "gl.deleteTexture(dead);"
           "gl.bindTexture(gl.TEXTURE_2D,dead);"
           "gl.getError()===gl.INVALID_OPERATION",
           "true");
    expect(engine, "gl.getError()===gl.NO_ERROR", "true");
}

/* Loads the DOM and Canvas2D shims and runs the device-gated checks. */
int main(void)
{
    canvas2d_init();
    JSEngine *engine = js_engine_init();
    if (!engine) {
        fprintf(stderr, "webgl_textures: js_engine_init failed\n");
        canvas2d_shutdown();
        return 1;
    }
    bool ok = webgl_define(js_engine_get_jsb(engine)) &&
              bind_canvas2d_define(js_engine_get_jsb(engine)) &&
              js_engine_eval_file(engine, "src/shims/dom_shim.js") &&
              js_engine_eval_file(engine, "src/shims/canvas2d_shim.js");
    if (!ok) {
        fprintf(stderr, "webgl_textures: shim setup failed\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        canvas2d_shutdown();
        return 1;
    }

    /* The card allows a skip only when no GL device exists. */
    char *probe = js_engine_eval_string(
        engine,
        "var __probe=document.createElement('canvas');"
        "String(__probe.getContext('webgl2')!==null)",
        "webgl_textures.js");
    int has_device = probe && strcmp(probe, "true") == 0;
    js_engine_free_string(probe);
    if (!has_device) {
        printf("webgl_textures: SKIP (no GL device or required entry point)\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        canvas2d_shutdown();
        return 0;
    }

    setup_context(engine);
    check_typed_array(engine);
    check_canvas_source(engine);
    check_premultiply(engine);
    check_image_data_and_lifetime(engine);
    webgl_shutdown();
    js_engine_shutdown(engine);
    canvas2d_shutdown();
    printf("webgl_textures: all checks passed\n");
    return 0;
}
