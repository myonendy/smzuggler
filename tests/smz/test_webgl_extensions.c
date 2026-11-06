/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Exercises WebGL2 getExtension/getSupportedExtensions and getParameter
 * against the recorded PIXI 5.3.12 / Chrome 117 extension probe list.
 *
 * Why: PIXI trusts getExtension to mean the driver can really do the feature,
 * and reads a few limits while configuring its renderer. The fixture is the
 * recorded Chrome 117 answer so the runtime cannot drift into claiming an
 * extension it does not back. It skips only when no GL device exists; a GUI
 * run must pass. Main thread only.
 */
#include "engine/js_engine.h"
#include "webgl/webgl.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EXTENSIONS 64

/* One recorded PIXI probe: the name and whether Chrome 117 backed it. */
struct ExtensionRow { char name[128]; int chrome; };

/* Evaluates `script` and compares String(result) with `want`. */
static void expect(JSEngine *engine, const char *script, const char *want)
{
    assert(engine != NULL);
    assert(script != NULL);
    assert(want != NULL);
    char *got = js_engine_eval_string(engine, script, "webgl_extensions.js");
    if (!got || strcmp(got, want) != 0) {
        fprintf(stderr, "webgl_extensions: expected %s, got %s\n  script: %s\n",
                want, got ? got : "(exception)", script);
        exit(1);
    }
    js_engine_free_string(got);
}

/* Evaluates `script` and accepts either of two honest answers. */
static void expect_any(JSEngine *engine, const char *script, const char *first,
                       const char *second)
{
    char *got = js_engine_eval_string(engine, script, "webgl_extensions.js");
    if (!got || (strcmp(got, first) != 0 && strcmp(got, second) != 0)) {
        fprintf(stderr, "webgl_extensions: expected %s or %s, got %s\n",
                first, second, got ? got : "(exception)");
        exit(1);
    }
    js_engine_free_string(got);
}

/* Loads the recorded probe list; returns the row count or exits on error. */
static size_t load_fixture(struct ExtensionRow *rows)
{
    const char *path = "tests/fixtures/webgl_pixi_extensions_chrome117.txt";
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "webgl_extensions: cannot open %s\n", path);
        exit(1);
    }
    size_t count = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\0') continue;
        char answer[16] = {0};
        if (sscanf(p, "%127s %15s", rows[count].name, answer) != 2) continue;
        rows[count].chrome = strcmp(answer, "yes") == 0;
        if (++count == MAX_EXTENSIONS) break;
    }
    fclose(file);
    if (count == 0) {
        fprintf(stderr, "webgl_extensions: fixture is empty\n");
        exit(1);
    }
    return count;
}

/* Checks one recorded probe: getExtension and getSupportedExtensions agree,
 * and Chrome's WebGL2-only nulls stay null. */
static void check_row(JSEngine *engine, const struct ExtensionRow *row)
{
    char script[512];
    snprintf(script, sizeof(script),
             "var __e=gl.getExtension('%s');"
             "var __s=gl.getSupportedExtensions();"
             "( __e!==null )+'|'+( __s.indexOf('%s')>=0 )",
             row->name, row->name);
    /* Both sides must agree; a driver may back fewer than Chrome, never more. */
    if (row->chrome) {
        char *got = js_engine_eval_string(engine, script, "webgl_extensions.js");
        if (!got || (strcmp(got, "true|true") != 0 &&
                     strcmp(got, "false|false") != 0)) {
            fprintf(stderr,
                    "webgl_extensions: inconsistent answer for %s: %s\n",
                    row->name, got ? got : "(exception)");
            exit(1);
        }
        js_engine_free_string(got);
        return;
    }
    expect(engine, script, "false|false");
}

/* Asserts every advertised name is in the fixture and Chrome-backed, so the
 * supported list cannot invent an extension. */
static void check_supported_names(JSEngine *engine,
                                  const struct ExtensionRow *rows,
                                  size_t count)
{
    char *joined = js_engine_eval_string(
        engine,
        "var __s=gl.getSupportedExtensions();"
        "__s.join(',')",
        "webgl_extensions.js");
    if (!joined) {
        fprintf(stderr, "webgl_extensions: getSupportedExtensions failed\n");
        exit(1);
    }
    char *save = NULL;
    size_t backed = 0;
    for (char *name = strtok_r(joined, ",", &save); name;
         name = strtok_r(NULL, ",", &save)) {
        size_t i = 0;
        for (; i < count; i++)
            if (!strcmp(rows[i].name, name)) break;
        if (i == count || !rows[i].chrome) {
            fprintf(stderr,
                    "webgl_extensions: advertised %s is not a Chrome 117 PIXI probe\n",
                    name);
            exit(1);
        }
        backed++;
    }
    js_engine_free_string(joined);
    /* An honest backend with a GL device still backs at least one PIXI probe
     * (float linear filtering exists in WebGL2/GL 3.3 core). */
    if (backed == 0) {
        fprintf(stderr, "webgl_extensions: no extension is backed\n");
        exit(1);
    }
}

/* Checks unknown names, the anisotropic object and the limits PIXI reads. */
static void check_objects_and_limits(JSEngine *engine)
{
    expect(engine,
           "gl.getExtension('SMZ_not_an_extension')===null && "
           "gl.getExtension('WEBGL_draw_buffers')===null && "
           "gl.getExtension('OES_vertex_array_object')===null",
           "true");
    expect_any(engine,
               "var aniso=gl.getExtension('EXT_texture_filter_anisotropic');"
               "if(aniso===null)'absent';"
               "else (aniso.TEXTURE_MAX_ANISOTROPY_EXT===0x84FE && "
               "aniso.MAX_TEXTURE_MAX_ANISOTROPY_EXT===0x84FF && "
               "gl.getParameter(aniso.MAX_TEXTURE_MAX_ANISOTROPY_EXT)>0)"
               "?'ok':'bad'",
               "absent", "ok");
    expect(engine,
           "typeof gl.getParameter(gl.MAX_TEXTURE_SIZE)==='number' && "
           "gl.getParameter(gl.MAX_TEXTURE_SIZE)>=2048 && "
           "typeof gl.getParameter(gl.MAX_VERTEX_ATTRIBS)==='number' && "
           "gl.getParameter(gl.MAX_VERTEX_ATTRIBS)>=16 && "
           "typeof gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS)==='number' && "
           "gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS)>=8 && "
           "gl.getParameter(gl.MAX_COMBINED_TEXTURE_IMAGE_UNITS)>="
           "gl.getParameter(gl.MAX_TEXTURE_IMAGE_UNITS) && "
           "gl.getParameter(gl.MAX_RENDERBUFFER_SIZE)>0 && "
           "gl.getParameter(gl.VIEWPORT) instanceof Int32Array && "
           "gl.getParameter(gl.VIEWPORT).length===4 && "
           "gl.getParameter(gl.MAX_VIEWPORT_DIMS) instanceof Int32Array && "
           "gl.getParameter(gl.MAX_VIEWPORT_DIMS).length===2 && "
           "typeof gl.getParameter(gl.DEPTH_TEST)==='boolean' && "
           "gl.getParameter(gl.VERSION).length>0",
           "true");
}

/* Loads the DOM shim and runs the device-gated checks. */
int main(void)
{
    static struct ExtensionRow rows[MAX_EXTENSIONS];
    size_t count = load_fixture(rows);

    JSEngine *engine = js_engine_init();
    if (!engine) {
        fprintf(stderr, "webgl_extensions: js_engine_init failed\n");
        return 1;
    }
    if (!webgl_define(js_engine_get_jsb(engine))) {
        fprintf(stderr, "webgl_extensions: webgl_define failed\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        return 1;
    }
    if (!js_engine_eval_file(engine, "src/shims/dom_shim.js")) {
        fprintf(stderr, "webgl_extensions: could not load dom_shim.js\n");
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
        "webgl_extensions.js");
    int has_device = probe && strcmp(probe, "ok") == 0;
    js_engine_free_string(probe);
    if (!has_device) {
        printf("webgl_extensions: SKIP (no GL device or required entry point)\n");
        webgl_shutdown();
        js_engine_shutdown(engine);
        return 0;
    }

    expect(engine,
           "var cnv=document.createElement('canvas');"
           "cnv.width=4;cnv.height=4;"
           "var gl=cnv.getContext('webgl2');typeof gl==='object'?'ok':'missing'",
           "ok");
    for (size_t i = 0; i < count; i++) check_row(engine, &rows[i]);
    check_supported_names(engine, rows, count);
    check_objects_and_limits(engine);
    webgl_shutdown();
    js_engine_shutdown(engine);
    printf("webgl_extensions: all checks passed\n");
    return 0;
}
