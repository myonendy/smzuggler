/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Exercises WebGL2 framebuffer and depth-stencil renderbuffer objects.
 *
 * Why: PIXI's render targets and VisuMZ's stencil calls only work when an FBO
 * can carry a texture plus a depth24-stencil8 renderbuffer, and when a draw
 * that fails the stencil test leaves colour untouched. The test draws through
 * the real shader pipeline and reads each result back. It skips only when no
 * GL device exists; a GUI run must pass. Main thread only.
 */
#include "engine/js_engine.h"
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
    char *got = js_engine_eval_string(engine, script, "webgl_fbo.js");
    if (!got || strcmp(got, want) != 0) {
        fprintf(stderr, "webgl_fbo: expected %s, got %s\n  script: %s\n",
                want, got ? got : "(exception)", script);
        exit(1);
    }
    js_engine_free_string(got);
}

/* Creates the context, FBO and its colour/depth-stencil attachments. */
static void setup_fbo(JSEngine *engine)
{
    expect(engine,
           "var cnv=document.createElement('canvas');"
           "cnv.width=4;cnv.height=4;"
           "var gl=cnv.getContext('webgl2');"
           "typeof gl==='object'?'ok':'missing'",
           "ok");
    expect(engine,
           "var colorTex=gl.createTexture();"
           "gl.bindTexture(gl.TEXTURE_2D,colorTex);"
           "gl.texImage2D(gl.TEXTURE_2D,0,gl.RGBA8,4,4,0,gl.RGBA,"
           "gl.UNSIGNED_BYTE,null);"
           "gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MIN_FILTER,gl.NEAREST);"
           "gl.texParameteri(gl.TEXTURE_2D,gl.TEXTURE_MAG_FILTER,gl.NEAREST);"
           "var depthStencil=gl.createRenderbuffer();"
           "gl.bindRenderbuffer(gl.RENDERBUFFER,depthStencil);"
           "gl.renderbufferStorage(gl.RENDERBUFFER,gl.DEPTH24_STENCIL8,4,4);"
           "var fbo=gl.createFramebuffer();"
           "gl.bindFramebuffer(gl.FRAMEBUFFER,fbo);"
           "gl.framebufferTexture2D(gl.FRAMEBUFFER,gl.COLOR_ATTACHMENT0,"
           "gl.TEXTURE_2D,colorTex,0);"
           "gl.framebufferRenderbuffer(gl.FRAMEBUFFER,"
           "gl.DEPTH_STENCIL_ATTACHMENT,gl.RENDERBUFFER,depthStencil);"
           "gl.checkFramebufferStatus(gl.FRAMEBUFFER)===gl.FRAMEBUFFER_COMPLETE",
           "true");
}

/* Builds the two-shader program and the full-screen triangle. */
static void setup_pipeline(JSEngine *engine)
{
    expect(engine,
           "function makeShader(type,src){"
           "var s=gl.createShader(type);"
           "gl.shaderSource(s,src);"
           "gl.compileShader(s);"
           "if(!gl.getShaderParameter(s,gl.COMPILE_STATUS))"
           "return gl.getShaderInfoLog(s);"
           "return s;}"
           "var es=gl.getParameter(gl.VERSION).indexOf('OpenGL ES')>=0;"
           "var prefix=es?'#version 300 es\\n':'#version 330 core\\n';"
           "var vs=makeShader(gl.VERTEX_SHADER,prefix+"
           "'in vec2 aPos;void main(){gl_Position=vec4(aPos,0.0,1.0);}');"
           "var fs=makeShader(gl.FRAGMENT_SHADER,prefix+"
           "(es?'precision mediump float;':'')+"
           "'out vec4 color;void main(){color=vec4(1.0,0.0,0.0,1.0);}');"
           "if(typeof vs==='string'||typeof fs==='string')"
           "throw new Error('shader: '+vs+' / '+fs);"
           "var program=gl.createProgram();"
           "gl.attachShader(program,vs);"
           "gl.attachShader(program,fs);"
           "gl.bindAttribLocation(program,0,'aPos');"
           "gl.linkProgram(program);"
           "if(!gl.getProgramParameter(program,gl.LINK_STATUS))"
           "throw new Error('link: '+gl.getProgramInfoLog(program));"
           "gl.deleteShader(vs);gl.deleteShader(fs);"
           "var vb=gl.createBuffer();"
           "gl.bindBuffer(gl.ARRAY_BUFFER,vb);"
           "gl.bufferData(gl.ARRAY_BUFFER,new Float32Array("
           "[-1,-1,3,-1,-1,3]),gl.STATIC_DRAW);"
           "var vao=gl.createVertexArray();"
           "gl.bindVertexArray(vao);"
           "gl.enableVertexAttribArray(0);"
           "gl.vertexAttribPointer(0,2,gl.FLOAT,false,0,0);"
           "gl.viewport(0,0,4,4);"
           "'ok'",
           "ok");
}

/* Clear, fail the stencil test, then pass it and read both results. */
static void check_stencil(JSEngine *engine)
{
    expect(engine,
           "function readOne(){var px=new Uint8Array(4);"
           "gl.readPixels(0,0,1,1,gl.RGBA,gl.UNSIGNED_BYTE,px);"
           "return Array.from(px).join(',');}"
           "gl.clearColor(0,1,0,1);"
           "gl.clearStencil(0);"
           "gl.stencilMask(0xFF);"
           "gl.clear(gl.COLOR_BUFFER_BIT|gl.STENCIL_BUFFER_BIT);"
           "gl.enable(gl.STENCIL_TEST);"
           "gl.stencilFunc(gl.EQUAL,1,0xFF);"
           "gl.stencilOp(gl.KEEP,gl.KEEP,gl.KEEP);"
           "gl.useProgram(program);"
           "gl.drawArrays(gl.TRIANGLES,0,3);"
           "var failed=readOne();"
           "gl.stencilFunc(gl.EQUAL,0,0xFF);"
           "gl.drawArrays(gl.TRIANGLES,0,3);"
           "var passed=readOne();"
           "gl.disable(gl.STENCIL_TEST);"
           "failed+'|'+passed+'|'+(gl.getError()===gl.NO_ERROR)",
           "0,255,0,255|255,0,0,255|true");
}

/* Restores the default framebuffer and proves it is drawable again. */
static void check_restore_default(JSEngine *engine)
{
    expect(engine,
           "gl.bindFramebuffer(gl.FRAMEBUFFER,null);"
           "gl.viewport(0,0,4,4);"
           "gl.clearColor(0,0,1,1);"
           "gl.clear(gl.COLOR_BUFFER_BIT);"
           "var px=new Uint8Array(4);"
           "gl.readPixels(0,0,1,1,gl.RGBA,gl.UNSIGNED_BYTE,px);"
           "Array.from(px).join(',')+'|'+(gl.getError()===gl.NO_ERROR)",
           "0,0,255,255|true");
    expect(engine,
           "gl.bindFramebuffer(gl.FRAMEBUFFER,null);"
           "gl.deleteFramebuffer(fbo);"
           "gl.deleteRenderbuffer(depthStencil);"
           "gl.deleteTexture(colorTex);"
           "gl.deleteProgram(program);"
           "gl.deleteBuffer(vb);"
           "gl.deleteVertexArray(vao);"
           "gl.getError()===gl.NO_ERROR",
           "true");
}

/* Loads the DOM shim and runs the device-gated checks. */
int main(void)
{
    JSEngine *engine = js_engine_init();
    if (!engine) {
        fprintf(stderr, "webgl_fbo: js_engine_init failed\n");
        return 1;
    }
    bool ok = webgl_define(js_engine_get_jsb(engine)) &&
              js_engine_eval_file(engine, "src/shims/dom_shim.js");
    if (!ok) {
        fprintf(stderr, "webgl_fbo: shim setup failed\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        return 1;
    }

    /* The card allows a skip only when no GL device exists. */
    char *probe = js_engine_eval_string(
        engine,
        "var __probe=document.createElement('canvas');"
        "String(__probe.getContext('webgl2')!==null)",
        "webgl_fbo.js");
    int has_device = probe && strcmp(probe, "true") == 0;
    js_engine_free_string(probe);
    if (!has_device) {
        printf("webgl_fbo: SKIP (no GL device or required entry point)\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        return 0;
    }

    setup_fbo(engine);
    setup_pipeline(engine);
    check_stencil(engine);
    check_restore_default(engine);
    webgl_shutdown();
    js_engine_shutdown(engine);
    printf("webgl_fbo: all checks passed\n");
    return 0;
}
