/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: port the __native_filters binding to the jsb seam.
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 *
 * __native_filters: the native bridge under pixi_shim.js, which compiles and
 * draws the PIXI filter shaders on Outsider's renderer (docs/js-binding.md,
 * "bind_filters"). It is kept only through M1 as the rendering baseline
 * (spec section 4.2). The port keeps Outsider's function order, arities,
 * coercion order and messages so upstream fixes re-apply by hand.
 *
 * Every jsb_val here is call-scoped and never kept; shader IDs stay in plain
 * C statics like upstream's. The only bytes borrowed from JS are the mat4
 * argument, and only until the call returns (spec section 15.8). Main thread
 * only.
 */

#include "bindings/bind_filters.h"
#include "rendering/filters.h"
#include "engine/error_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Built-in shader IDs, compiled during init(). */

static uint32_t s_color_matrix_shader = 0;
static uint32_t s_blur_shader = 0;
static uint32_t s_alpha_shader = 0;
static uint32_t s_color_filter_shader = 0;
static uint32_t s_mask_shader = 0;

/* Borrowed runtime of the module last defined; init() needs it to publish the
 * shader IDs, because jsb has no way to reach the global object from a native
 * call. Set by bind_filters_define and cleared by bind_filters_shutdown; the
 * host owns it. */
static jsb_runtime *s_runtime = NULL;

/* Writes the five shader IDs onto the already-defined module.
 *
 * Why: upstream exposed compiled IDs as properties of __native_filters from
 * inside init(). The IDs are unsigned, so the generated assignment script
 * needs no escaping; a failure is reported but does not fail init(), as
 * upstream ignored the property writes. Main thread only. */
static void publish_shader_ids(void)
{
    if (!s_runtime) return;
    char script[256];
    int n = snprintf(script, sizeof script,
                     "__native_filters.colorMatrixShader=%u;"
                     "__native_filters.maskShader=%u;"
                     "__native_filters.blurShader=%u;"
                     "__native_filters.alphaShader=%u;"
                     "__native_filters.colorFilterShader=%u;",
                     s_color_matrix_shader, s_mask_shader, s_blur_shader,
                     s_alpha_shader, s_color_filter_shader);
    if (n <= 0 || (size_t)n >= sizeof script) return;
    char error[256];
    if (!jsb_runtime_eval(s_runtime, script, "bind_filters.c", error,
                          sizeof error))
        fprintf(stderr, "bind_filters: %s\n", error);
}

/* init() -> bool
 *
 * Why: creates the shared filter quad and compiles the five built-in shaders
 * (compat: PIXI.Filter lifecycle). Without a graphics context every compile
 * returns 0, so it logs and returns false and leaves the IDs at 0, as
 * upstream did. Main thread only; no handles escape. */
static void js_filters_init(jsb_ctx *ctx)
{
    filters_init();

    /* Compile built-in filter shaders. */
    if (!s_color_matrix_shader)
        s_color_matrix_shader = filter_compile_shader(NULL, filter_color_matrix_frag_src());
    if (!s_blur_shader)
        s_blur_shader = filter_compile_shader(NULL, filter_blur_frag_src());
    if (!s_alpha_shader)
        s_alpha_shader = filter_compile_shader(NULL, filter_alpha_frag_src());
    if (!s_color_filter_shader)
        s_color_filter_shader = filter_compile_shader(NULL, filter_color_filter_frag_src());
    if (!s_mask_shader)
        s_mask_shader = filter_compile_shader(NULL, filter_mask_frag_src());

    if (!s_color_matrix_shader || !s_blur_shader ||
        !s_alpha_shader || !s_color_filter_shader) {
        error_handler_log(LOG_ERROR, "Built-in shader compilation failed:"
                          " colorMatrix=%u blur=%u alpha=%u colorFilter=%u",
                          s_color_matrix_shader, s_blur_shader,
                          s_alpha_shader, s_color_filter_shader);
        jsb_ret_bool(ctx, false);
        return;
    }

    publish_shader_ids();
    jsb_ret_bool(ctx, true);
}

/* shutdown()
 *
 * Why: releases the built-in programs and the filter quad at scene or
 * process teardown, then returns undefined (compat: PIXI.Filter lifecycle).
 * Repeated calls are safe because every ID is zeroed. Main thread only. */
static void js_filters_shutdown(jsb_ctx *ctx)
{
    if (s_color_matrix_shader) { filter_delete_shader(s_color_matrix_shader); s_color_matrix_shader = 0; }
    if (s_blur_shader) { filter_delete_shader(s_blur_shader); s_blur_shader = 0; }
    if (s_alpha_shader) { filter_delete_shader(s_alpha_shader); s_alpha_shader = 0; }
    if (s_color_filter_shader) { filter_delete_shader(s_color_filter_shader); s_color_filter_shader = 0; }
    if (s_mask_shader) { filter_delete_shader(s_mask_shader); s_mask_shader = 0; }
    filters_shutdown();
    jsb_ret_undefined(ctx);
}

/* compileShader(vertSrc_or_null, fragSrc) -> shaderId
 *
 * Why: PIXI.Filter creates its GL program here (compat: PIXI.Filter). Fewer
 * than two arguments is upstream's TypeError. A vertex source that does not
 * convert is ignored (the default vertex shader is used) while a fragment
 * source that does not convert gets upstream's fragSrc TypeError. Returns
 * the unsigned program ID, 0 when GL is unavailable. Main thread only;
 * converted strings are freed before return. */
static void js_filters_compile(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "compileShader requires vertSrc and fragSrc");
        return;
    }

    char *vert_src = NULL;
    if (jsb_val_type(ctx, jsb_arg_val(ctx, 0)) != JSB_TYPE_NULL &&
        jsb_val_type(ctx, jsb_arg_val(ctx, 0)) != JSB_TYPE_UNDEFINED) {
        vert_src = jsb_arg_to_str(ctx, 0, NULL);
        /* Upstream cleared the conversion error and used the default shader. */
        if (!vert_src) jsb_clear_exception(ctx);
    }
    char *frag_src = jsb_arg_to_str(ctx, 1, NULL);
    if (!frag_src) {
        jsb_free_str(vert_src);
        jsb_clear_exception(ctx);
        jsb_throw_type(ctx, "compileShader: fragSrc required");
        return;
    }

    uint32_t id = filter_compile_shader(vert_src, frag_src);
    jsb_free_str(vert_src);
    jsb_free_str(frag_src);
    jsb_ret_val(ctx, jsb_new_u32(ctx, id));
}

/* deleteShader(shaderId)
 *
 * Why: PIXI.Filter destroys its program through this (compat: PIXI.Filter).
 * A missing ID is ignored; the ID uses ToUint32 coercion. Main thread only. */
static void js_filters_delete(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    uint32_t id;
    if (!jsb_arg_to_u32(ctx, 0, &id)) return;
    filter_delete_shader(id);
    jsb_ret_undefined(ctx);
}

/* Filter rendering */

/* beginFilter(shaderId, inputTexture, width, height)
 *
 * Why: binds a filter's program, input texture and target size before
 * drawQuad (compat: PIXI.Filter.apply). Fewer than four arguments is
 * upstream's TypeError; values use ToUint32/ToInt32 coercion. Main thread. */
static void js_filters_begin(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) {
        jsb_throw_type(ctx, "beginFilter requires shaderId, inputTexture, w, h");
        return;
    }

    uint32_t shader, texture;
    int32_t w, h;
    if (!jsb_arg_to_u32(ctx, 0, &shader)) return;
    if (!jsb_arg_to_u32(ctx, 1, &texture)) return;
    if (!jsb_arg_to_i32(ctx, 2, &w)) return;
    if (!jsb_arg_to_i32(ctx, 3, &h)) return;

    filter_begin(shader, texture, (int)w, (int)h);
    jsb_ret_undefined(ctx);
}

/* setUniform1f(shader, name, value)
 *
 * Why: uploads a float uniform for PIXI.Filter.uniforms (compat:
 * PIXI.Filter). Fewer than three arguments is a no-op; the name is converted
 * with ToString and freed before return. Main thread only. */
static void js_filters_uniform_1f(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 3) return;

    uint32_t shader;
    if (!jsb_arg_to_u32(ctx, 0, &shader)) return;
    char *name = jsb_arg_to_str(ctx, 1, NULL);
    if (!name) return;
    double val;
    if (!jsb_arg_to_f64(ctx, 2, &val)) { jsb_free_str(name); return; }

    filter_set_uniform_1f(shader, name, (float)val);
    jsb_free_str(name);
    jsb_ret_undefined(ctx);
}

/* setUniformTexture(shader, name, texture, unit)
 *
 * Why: binds an extra sampler for a multi-input PIXI.Filter (compat:
 * PIXI.Filter). Fewer than four arguments is a no-op; coercion order and the
 * name's lifetime are upstream's. Main thread only. */
static void js_filters_uniform_texture(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) return;
    uint32_t shader, texture;
    int32_t unit;
    if (!jsb_arg_to_u32(ctx, 0, &shader)) return;
    char *name = jsb_arg_to_str(ctx, 1, NULL);
    if (!name) return;
    if (!jsb_arg_to_u32(ctx, 2, &texture) || !jsb_arg_to_i32(ctx, 3, &unit)) {
        jsb_free_str(name);
        return;
    }
    filter_set_uniform_texture(shader, name, texture, (int)unit);
    jsb_free_str(name);
    jsb_ret_undefined(ctx);
}

/* setUniform2f(shader, name, x, y)
 *
 * Why: uploads a vec2 uniform (compat: PIXI.Filter). Fewer than four
 * arguments is a no-op; the name is freed before return. Main thread only. */
static void js_filters_uniform_2f(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) return;

    uint32_t shader;
    if (!jsb_arg_to_u32(ctx, 0, &shader)) return;
    char *name = jsb_arg_to_str(ctx, 1, NULL);
    if (!name) return;
    double x, y;
    if (!jsb_arg_to_f64(ctx, 2, &x)) { jsb_free_str(name); return; }
    if (!jsb_arg_to_f64(ctx, 3, &y)) { jsb_free_str(name); return; }

    filter_set_uniform_2f(shader, name, (float)x, (float)y);
    jsb_free_str(name);
    jsb_ret_undefined(ctx);
}

/* setUniform4f(shader, name, x, y, z, w)
 *
 * Why: uploads a vec4 uniform (compat: PIXI.Filter). Fewer than six
 * arguments is a no-op; the name is freed before return. Main thread only. */
static void js_filters_uniform_4f(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 6) return;

    uint32_t shader;
    if (!jsb_arg_to_u32(ctx, 0, &shader)) return;
    char *name = jsb_arg_to_str(ctx, 1, NULL);
    if (!name) return;
    double x, y, z, w;
    if (!jsb_arg_to_f64(ctx, 2, &x)) { jsb_free_str(name); return; }
    if (!jsb_arg_to_f64(ctx, 3, &y)) { jsb_free_str(name); return; }
    if (!jsb_arg_to_f64(ctx, 4, &z)) { jsb_free_str(name); return; }
    if (!jsb_arg_to_f64(ctx, 5, &w)) { jsb_free_str(name); return; }

    filter_set_uniform_4f(shader, name, (float)x, (float)y, (float)z, (float)w);
    jsb_free_str(name);
    jsb_ret_undefined(ctx);
}

/* Reads argument `index` as the mat4 forms upstream accepted: an
 * ArrayBuffer, a TypedArray view, or a plain Array, in that order.
 *
 * Why: pixi_shim's generic Filter path passes a plain 16-element Array while
 * the built-in filters pass Float32Array/ArrayBuffer, so all three must keep
 * working. jsb_arg_bytes also takes a DataView, which upstream's
 * ArrayBuffer/TypedArray probes both refused, so a DataView is excluded
 * here (it has byteOffset but no BYTES_PER_ELEMENT). On a plain Array it
 * reads 16 elements with Number coercion into `arr`, matching upstream.
 * Returns true with `*buf`/`*size` set, or false with no exception pending
 * for anything else. Bytes are borrowed until the call returns. */
static bool mat4_arg_bytes(jsb_ctx *ctx, int index, uint8_t **buf,
                           size_t *size, float arr[16])
{
    jsb_val *val = jsb_arg_val(ctx, index);
    bool can_be_buffer = true;
    jsb_val *offset = jsb_get_prop(ctx, val, "byteOffset");
    if (!offset) {
        jsb_clear_exception(ctx);
        can_be_buffer = false;
    } else if (jsb_val_type(ctx, offset) != JSB_TYPE_UNDEFINED) {
        jsb_val *elem = jsb_get_prop(ctx, val, "BYTES_PER_ELEMENT");
        if (!elem) {
            jsb_clear_exception(ctx);
            can_be_buffer = false;
        } else if (jsb_val_type(ctx, elem) == JSB_TYPE_UNDEFINED) {
            can_be_buffer = false; /* a DataView, which upstream refused */
        }
    }
    if (can_be_buffer && jsb_arg_bytes(ctx, index, buf, size)) return true;
    if (can_be_buffer) jsb_clear_exception(ctx);

    if (!jsb_is_array(ctx, val)) return false;
    for (int i = 0; i < 16; i++) {
        jsb_val *elem = jsb_get_index(ctx, val, (uint32_t)i);
        double d = 0;
        if (!elem || !jsb_val_to_f64(ctx, elem, &d)) return false;
        arr[i] = (float)d;
    }
    *buf = (uint8_t *)arr;
    *size = sizeof(float) * 16;
    return true;
}

/* setUniformMat4(shader, name, Float32Array_16 | ArrayBuffer | Array_16)
 *
 * Why: uploads a column-major matrix uniform (compat: PIXI.Filter). Fewer
 * than three arguments is a no-op; a value of another shape is silently
 * ignored, as upstream did; shorter buffers are ignored when under 16
 * floats. The name is freed before return. Main thread only. */
static void js_filters_uniform_mat4(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 3) return;

    uint32_t shader;
    if (!jsb_arg_to_u32(ctx, 0, &shader)) return;
    char *name = jsb_arg_to_str(ctx, 1, NULL);
    if (!name) return;

    uint8_t *buf = NULL;
    size_t buf_size = 0;
    float arr[16];
    if (!mat4_arg_bytes(ctx, 2, &buf, &buf_size, arr)) {
        jsb_free_str(name);
        return;
    }

    if (buf && buf_size >= 16 * sizeof(float))
        filter_set_uniform_mat4(shader, name, (const float *)buf);

    jsb_free_str(name);
    jsb_ret_undefined(ctx);
}

/* drawQuad()
 *
 * Why: draws the fullscreen filter quad with the program and uniforms set by
 * beginFilter (compat: PIXI.Filter.apply). Arguments are ignored. */
static void js_filters_draw_quad(jsb_ctx *ctx)
{
    filter_draw_quad();
    jsb_ret_undefined(ctx);
}

/* endFilter()
 *
 * Why: unbinds the program and VAO after drawQuad (compat:
 * PIXI.Filter.apply). Arguments are ignored. */
static void js_filters_end(jsb_ctx *ctx)
{
    filter_end();
    jsb_ret_undefined(ctx);
}

/* Registration */

/* Fill __native_filters with upstream's function order and arities, then its
 * five shader-ID properties. The runtime owns every function; the IDs start
 * at 0 and init() overwrites them. Returns false if a property could not be
 * set. */
static bool filters_module_init(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn_n(ctx, module, "init", js_filters_init, 0) &&
           jsb_set_fn_n(ctx, module, "shutdown", js_filters_shutdown, 0) &&
           jsb_set_fn_n(ctx, module, "compileShader", js_filters_compile, 2) &&
           jsb_set_fn_n(ctx, module, "deleteShader", js_filters_delete, 1) &&
           jsb_set_fn_n(ctx, module, "beginFilter", js_filters_begin, 4) &&
           jsb_set_fn_n(ctx, module, "setUniform1f", js_filters_uniform_1f, 3) &&
           jsb_set_fn_n(ctx, module, "setUniform2f", js_filters_uniform_2f, 4) &&
           jsb_set_fn_n(ctx, module, "setUniformTexture",
                        js_filters_uniform_texture, 4) &&
           jsb_set_fn_n(ctx, module, "setUniform4f", js_filters_uniform_4f, 6) &&
           jsb_set_fn_n(ctx, module, "setUniformMat4",
                        js_filters_uniform_mat4, 3) &&
           jsb_set_fn_n(ctx, module, "drawQuad", js_filters_draw_quad, 0) &&
           jsb_set_fn_n(ctx, module, "endFilter", js_filters_end, 0) &&
           jsb_set_prop(ctx, module, "colorMatrixShader",
                        jsb_new_u32(ctx, 0)) &&
           jsb_set_prop(ctx, module, "maskShader", jsb_new_u32(ctx, 0)) &&
           jsb_set_prop(ctx, module, "blurShader", jsb_new_u32(ctx, 0)) &&
           jsb_set_prop(ctx, module, "alphaShader", jsb_new_u32(ctx, 0)) &&
           jsb_set_prop(ctx, module, "colorFilterShader",
                        jsb_new_u32(ctx, 0));
}

/* Define the module inside setup's handle scope; user is unused. */
static bool filters_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_filters", filters_module_init);
}

/* smzuggler: restore upstream's non-enumerable function properties.
 * jsb_set_fn_n assigns, so only the functions (not the shader IDs, which
 * upstream exposed with its ordinary property setter) are made
 * non-enumerable. */
static const char filters_hide_fns_js[] =
    "(function(m){Object.getOwnPropertyNames(m).forEach(function(k){"
    "if(typeof m[k]==='function')"
    "Object.defineProperty(m,k,{enumerable:false})})})(__native_filters);";

/* See bind_filters.h: borrow rt and report definition errors on the main
 * thread. */
bool bind_filters_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_filters_define: no jsb runtime\n");
        return false;
    }
    if (!jsb_runtime_setup(rt, filters_define_setup, NULL)) return false;
    char error[256];
    if (!jsb_runtime_eval(rt, filters_hide_fns_js, "bind_filters.c", error,
                          sizeof error)) {
        fprintf(stderr, "bind_filters_define: %s\n", error);
        return false;
    }
    s_runtime = rt;
    return true;
}

void bind_filters_shutdown(void)
{
    if (s_color_matrix_shader) { filter_delete_shader(s_color_matrix_shader); s_color_matrix_shader = 0; }
    if (s_blur_shader) { filter_delete_shader(s_blur_shader); s_blur_shader = 0; }
    if (s_alpha_shader) { filter_delete_shader(s_alpha_shader); s_alpha_shader = 0; }
    if (s_color_filter_shader) { filter_delete_shader(s_color_filter_shader); s_color_filter_shader = 0; }
    if (s_mask_shader) { filter_delete_shader(s_mask_shader); s_mask_shader = 0; }
    s_runtime = NULL;
}
