/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Exercises the WebGL2 shader path: GLSL ES 1.00 and 3.00 compile and link
 * through the driver, a syntax error reports its source line, and a draw
 * after useProgram leaves no GL error.
 *
 * Why: PIXI 5.3.12 hands the runtime WebGL1-style GLSL ES on a WebGL2 context,
 * while a desktop GL 3.3 core backend cannot compile that text unchanged.
 * This proves both spellings reach a linked program. It skips only when no GL
 * device exists; a GUI run must pass. Main thread only.
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
    char *got = js_engine_eval_string(engine, script, "webgl_shaders.js");
    if (!got || strcmp(got, want) != 0) {
        fprintf(stderr, "webgl_shaders: expected %s, got %s\n  script: %s\n",
                want, got ? got : "(exception)", script);
        exit(1);
    }
    js_engine_free_string(got);
}

/* Compiles and links a GLSL ES 3.00 program, then draws with it. */
static void check_es3_program(JSEngine *engine)
{
    expect(engine,
           "var v3=gl.createShader(gl.VERTEX_SHADER);"
           "gl.shaderSource(v3,'#version 300 es\\n"
           "in vec2 aPos;\\n"
           "void main(){gl_Position=vec4(aPos,0.0,1.0);}');"
           "gl.compileShader(v3);"
           "var vok=gl.getShaderParameter(v3,gl.COMPILE_STATUS);"
           "if(!vok)throw new Error('vertex: '+gl.getShaderInfoLog(v3));"
           "var f3=gl.createShader(gl.FRAGMENT_SHADER);"
           "gl.shaderSource(f3,'#version 300 es\\n"
           "precision highp float;\\n"
           "uniform float uAlpha;\\n"
           "out vec4 color;\\n"
           "void main(){color=vec4(0.0,1.0,0.0,uAlpha);}');"
           "gl.compileShader(f3);"
           "var fok=gl.getShaderParameter(f3,gl.COMPILE_STATUS);"
           "if(!fok)throw new Error('fragment: '+gl.getShaderInfoLog(f3));"
           "var p3=gl.createProgram();"
           "gl.attachShader(p3,v3);gl.attachShader(p3,f3);"
           "gl.bindAttribLocation(p3,0,'aPos');"
           "gl.linkProgram(p3);"
           "if(!gl.getProgramParameter(p3,gl.LINK_STATUS))"
           "throw new Error('link: '+gl.getProgramInfoLog(p3));"
           "var loc=gl.getUniformLocation(p3,'uAlpha');"
           "var au=gl.getActiveUniform(p3,0);"
           "var al=gl.getActiveAttrib(p3,0);"
           "var intro=(loc!==null && au!==null && au.name==='uAlpha' && "
           "al!==null && al.name==='aPos' && "
           "gl.getAttribLocation(p3,'aPos')>=0);"
           "var vb3=gl.createBuffer();"
           "gl.bindBuffer(gl.ARRAY_BUFFER,vb3);"
           "gl.bufferData(gl.ARRAY_BUFFER,new Float32Array("
           "[-1,-1,3,-1,-1,3]),gl.STATIC_DRAW);"
           "var vao3=gl.createVertexArray();gl.bindVertexArray(vao3);"
           "gl.enableVertexAttribArray(0);"
           "gl.vertexAttribPointer(0,2,gl.FLOAT,false,0,0);"
           "gl.viewport(0,0,4,4);"
           "gl.clearColor(0,0,0,1);gl.clear(gl.COLOR_BUFFER_BIT);"
           "gl.useProgram(p3);gl.uniform1f(loc,1.0);"
           "gl.drawArrays(gl.TRIANGLES,0,3);"
           "var px3=new Uint8Array(4);"
           "gl.readPixels(0,0,1,1,gl.RGBA,gl.UNSIGNED_BYTE,px3);"
           "Array.from(px3).join(',')+'|'+(gl.getError()===gl.NO_ERROR)"
           "+'|'+String(typeof gl.getShaderInfoLog(v3)==='string')"
           "+'|'+intro",
           "0,255,0,255|true|true|true");
    expect(engine,
           "gl.deleteProgram(p3);gl.deleteShader(v3);gl.deleteShader(f3);"
           "gl.deleteBuffer(vb3);gl.deleteVertexArray(vao3);"
           "gl.getError()===gl.NO_ERROR",
           "true");
}

/* Compiles GLSL ES 1.00 (attribute/varying/gl_FragColor) on WebGL2. */
static void check_es1_program(JSEngine *engine)
{
    expect(engine,
           "var v1=gl.createShader(gl.VERTEX_SHADER);"
           "gl.shaderSource(v1,'attribute vec2 aPos;\\n"
           "varying vec2 vUv;\\n"
           "void main(){vUv=aPos;gl_Position=vec4(aPos,0.0,1.0);}');"
           "gl.compileShader(v1);"
           "if(!gl.getShaderParameter(v1,gl.COMPILE_STATUS))"
           "throw new Error('vertex1: '+gl.getShaderInfoLog(v1));"
           "var f1=gl.createShader(gl.FRAGMENT_SHADER);"
           "gl.shaderSource(f1,'precision mediump float;\\n"
           "varying vec2 vUv;\\n"
           "void main(){gl_FragColor=vec4(0.0,0.0,1.0,1.0);}');"
           "gl.compileShader(f1);"
           "if(!gl.getShaderParameter(f1,gl.COMPILE_STATUS))"
           "throw new Error('fragment1: '+gl.getShaderInfoLog(f1));"
           "var p1=gl.createProgram();"
           "gl.attachShader(p1,v1);gl.attachShader(p1,f1);"
           "gl.bindAttribLocation(p1,0,'aPos');"
           "gl.linkProgram(p1);"
           "if(!gl.getProgramParameter(p1,gl.LINK_STATUS))"
           "throw new Error('link1: '+gl.getProgramInfoLog(p1));"
           "var vb1=gl.createBuffer();"
           "gl.bindBuffer(gl.ARRAY_BUFFER,vb1);"
           "gl.bufferData(gl.ARRAY_BUFFER,new Float32Array("
           "[-1,-1,3,-1,-1,3]),gl.STATIC_DRAW);"
           "var vao1=gl.createVertexArray();gl.bindVertexArray(vao1);"
           "gl.enableVertexAttribArray(0);"
           "gl.vertexAttribPointer(0,2,gl.FLOAT,false,0,0);"
           "gl.viewport(0,0,4,4);"
           "gl.clearColor(0,0,0,1);gl.clear(gl.COLOR_BUFFER_BIT);"
           "gl.useProgram(p1);gl.drawArrays(gl.TRIANGLES,0,3);"
           "var px1=new Uint8Array(4);"
           "gl.readPixels(0,0,1,1,gl.RGBA,gl.UNSIGNED_BYTE,px1);"
           "Array.from(px1).join(',')+'|'+(gl.getError()===gl.NO_ERROR)",
           "0,0,255,255|true");
    expect(engine,
           "gl.deleteProgram(p1);gl.deleteShader(v1);gl.deleteShader(f1);"
           "gl.deleteBuffer(vb1);gl.deleteVertexArray(vao1);"
           "gl.getError()===gl.NO_ERROR",
           "true");
}

/* Compiles a deliberate syntax error and checks the log names the source. */
static void check_compile_error(JSEngine *engine)
{
    expect(engine,
           "var bad=gl.createShader(gl.VERTEX_SHADER);"
           "gl.shaderSource(bad,'#version 300 es\\n"
           "in vec2 aPos;\\n"
           "void main(){gl_Position=vec4(aPos,0.0,1.0)}');"
           "gl.compileShader(bad);"
           "var ok=gl.getShaderParameter(bad,gl.COMPILE_STATUS);"
           "var log=gl.getShaderInfoLog(bad);"
           "var text=(typeof log==='string'&&log.length>0);"
           "(ok===false)+'|'+text+'|'+(log.indexOf('void main')>=0)+'|'+"
           "(log.indexOf('3: void main')>=0)",
           "true|true|true|true");
    expect(engine, "gl.deleteShader(bad);gl.getError()===gl.NO_ERROR", "true");
}

/* Loads the DOM shim and runs the device-gated checks. */
int main(void)
{
    JSEngine *engine = js_engine_init();
    if (!engine) {
        fprintf(stderr, "webgl_shaders: js_engine_init failed\n");
        return 1;
    }
    if (!webgl_define(js_engine_get_jsb(engine))) {
        fprintf(stderr, "webgl_shaders: webgl_define failed\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        return 1;
    }
    if (!js_engine_eval_file(engine, "src/shims/dom_shim.js")) {
        fprintf(stderr, "webgl_shaders: could not load dom_shim.js\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        return 1;
    }

    /* The card allows a skip only when no GL device exists. */
    char *probe = js_engine_eval_string(
        engine,
        "var __probe=document.createElement('canvas');"
        "__probe.width=4;__probe.height=4;"
        "var __gl=__probe.getContext('webgl2');"
        "(__gl!==null)?'ok':'missing'",
        "webgl_shaders.js");
    int has_device = probe && strcmp(probe, "ok") == 0;
    js_engine_free_string(probe);
    if (!has_device) {
        printf("webgl_shaders: SKIP (no GL device or required entry point)\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        return 0;
    }

    expect(engine,
           "var cnv=document.createElement('canvas');"
           "cnv.width=4;cnv.height=4;"
           "var gl=cnv.getContext('webgl2');typeof gl==='object'?'ok':'missing'",
           "ok");
    check_es3_program(engine);
    check_es1_program(engine);
    check_compile_error(engine);
    webgl_shutdown();
    js_engine_shutdown(engine);
    printf("webgl_shaders: all checks passed\n");
    return 0;
}
