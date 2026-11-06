/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Exercises a native WebGL2 context over GLES 3.0 through dom_shim.js.
 *
 * Why: the hollow stub made `getContext()` look successful while doing
 * nothing. This proves the native context issues real GL calls without an
 * error. It skips only when no GL device exists; a GUI run must pass.
 * Main thread only.
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
    char *got = js_engine_eval_string(engine, script, "webgl_context.js");
    if (!got || strcmp(got, want) != 0) {
        fprintf(stderr, "webgl_context: expected %s, got %s\n  script: %s\n",
                want, got ? got : "(exception)", script);
        exit(1);
    }
    js_engine_free_string(got);
}

/* Runs the GL-backed part of the test; the device is known to exist. */
static void run_gl_checks(JSEngine *engine)
{
    expect(engine,
           "var c=document.createElement('canvas');"
           "c.width=4;c.height=4;"
           "var gl=c.getContext('webgl2');"
           "(gl!==null)+'|'+(gl.canvas===c)+'|'+(gl.drawingBufferWidth===4)",
           "true|true|true");
    expect(engine, "gl.getParameter(gl.VERSION).length>0", "true");
    /* The canvas resize keeps the drawing-buffer size truthful. */
    expect(engine,
           "c.width=8;c.height=6;"
           "(gl.drawingBufferWidth===8)+'|'+(gl.drawingBufferHeight===6)",
           "true|true");
    /* The legacy probe names must reach the same native context. */
    expect(engine,
           "c.getContext('webgl')===gl && "
           "c.getContext('experimental-webgl')===gl",
           "true");
    /* State setters and the matching getters. */
    expect(engine,
           "gl.viewport(0,0,4,4);"
           "gl.clearColor(0.25,0.5,0.75,1);"
           "gl.enable(gl.DEPTH_TEST);"
           "var on=gl.isEnabled(gl.DEPTH_TEST);"
           "gl.disable(gl.DEPTH_TEST);"
           "var off=gl.isEnabled(gl.DEPTH_TEST);"
           "on+'|'+off",
           "true|false");
    /* Buffers, uploads and the VAO binding; no GL error means they worked. */
    expect(engine,
           "var vb=gl.createBuffer();"
           "gl.bindBuffer(gl.ARRAY_BUFFER,vb);"
           "gl.bufferData(gl.ARRAY_BUFFER,new Float32Array([0,0,1,0,0,1]),"
           "gl.STATIC_DRAW);"
           "gl.bufferSubData(gl.ARRAY_BUFFER,0,new Float32Array([0,1,1,1,1,0]));"
           "gl.getError()===gl.NO_ERROR",
           "true");
    expect(engine,
           "var ib=gl.createBuffer();"
           "gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER,ib);"
           "gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,new Uint16Array([0,1,2]),"
           "gl.STATIC_DRAW);"
           "gl.getError()===gl.NO_ERROR",
           "true");
    expect(engine,
           "var vao=gl.createVertexArray();"
           "gl.bindVertexArray(vao);"
           "gl.enableVertexAttribArray(0);"
           "gl.vertexAttribPointer(0,2,gl.FLOAT,false,0,0);"
           "gl.getError()===gl.NO_ERROR",
           "true");
    /* The clear is a real draw into the default framebuffer. */
    expect(engine,
           "gl.clear(gl.COLOR_BUFFER_BIT);"
           "gl.getError()===gl.NO_ERROR",
           "true");

    /* Explicit deletes release names; later errors must not appear. */
    expect(engine,
           "gl.deleteBuffer(vb);"
           "gl.deleteBuffer(ib);"
           "gl.deleteVertexArray(vao);"
           "gl.getError()===gl.NO_ERROR",
           "true");
    /* Using a deleted wrapper reports INVALID_OPERATION like Chrome, and is
     * a context error that the next getError() consumes. */
    expect(engine,
           "gl.bindBuffer(gl.ARRAY_BUFFER,vb);"
           "gl.getError()===gl.INVALID_OPERATION",
           "true");
    expect(engine, "gl.getError()===gl.NO_ERROR", "true");
    /* The same call site, hot enough for TurboFan's fast path, must report a
     * deleted buffer as INVALID_OPERATION rather than silently binding 0. */
    expect(engine,
           "function bindOnce(b){gl.bindBuffer(gl.ARRAY_BUFFER,b);"
           "return gl.getError();}"
           "var raw=gl.createBuffer();"
           "for(var i=0;i<200000;i++){bindOnce(raw);}"
           "var live=bindOnce(raw);"
           "gl.deleteBuffer(raw);"
           "var dead=bindOnce(raw);"
           "live+'|'+(dead===gl.INVALID_OPERATION)",
           "0|true");
}

/* Loads the DOM shim and runs the device-gated checks. */
int main(void)
{
    JSEngine *engine = js_engine_init();
    if (!engine) {
        fprintf(stderr, "webgl_context: js_engine_init failed\n");
        return 1;
    }
    if (!webgl_define(js_engine_get_jsb(engine))) {
        fprintf(stderr, "webgl_context: webgl_define failed\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        return 1;
    }
    if (!js_engine_eval_file(engine, "src/shims/dom_shim.js")) {
        fprintf(stderr, "webgl_context: could not load dom_shim.js\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        return 1;
    }
    expect(engine, "typeof __native_webgl", "object");

    /* The card allows a skip only when no GL device exists; creating a real
     * context is the honest check (hardware, software Mesa or none). */
    char *probe = js_engine_eval_string(
        engine,
        "var __probe=document.createElement('canvas');"
        "String(__probe.getContext('webgl2')!==null)",
        "webgl_context.js");
    int has_device = probe && strcmp(probe, "true") == 0;
    js_engine_free_string(probe);
    if (!has_device) {
        printf("webgl_context: SKIP (no GL device or required entry point)\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        return 0;
    }

    run_gl_checks(engine);
    webgl_shutdown();
    js_engine_shutdown(engine);
    printf("webgl_context: all checks passed\n");
    return 0;
}
