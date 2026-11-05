/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: ported from QuickJS to the jsb seam (spec section 4.1).
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 *
 * __native_renderer: the high-level sprite-batch renderer API (frame
 * lifecycle, textures, render textures, quads, scissor and readPixels) that
 * pixi_shim.js and tilemap_shim.js drive (docs/js-binding.md, "bind_renderer").
 *
 * smzuggler: the port keeps Outsider's structure function by function so
 * upstream fixes re-apply by hand (docs/UPSTREAM.md). Every jsb_val here is
 * a call-scoped handle, never kept past the native call that made it, and
 * buffer bytes from jsb_arg_bytes are used only before the call returns
 * (spec section 15.8, docs/js-binding.md "Handle and lifetime rules").
 * Main thread only.
 */

#include "bindings/bind_renderer.h"
#include "rendering/renderer.h"
#include "rendering/sprite_batch.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef RMMZ_HAS_GL
#include "rendering/gl_loader.h"
#endif

/* Cap on pixel dimensions so w*h*4 cannot overflow size_t: 16384 (GL max) is
   safe on 64-bit; on 32-bit 16384^2*4 == 2^32 wraps, so cap at 8192 there. */
#if SIZE_MAX > 0xFFFFFFFFu
#define MAX_PIXEL_DIM 16384
#else
#define MAX_PIXEL_DIM 8192
#endif

/* Single global renderer instance; also accessed by bind_tilemap.c. */
Renderer *s_renderer = NULL;

/* Returns whether argument `index` is a plain ArrayBuffer, not a view.
 *
 * Why: upstream read pixels with its ArrayBuffer getter, which accepts only
 * an ArrayBuffer, while jsb_arg_bytes also takes TypedArrays and DataViews.
 * Among those three, only views have a `byteOffset` (an inherited getter).
 * Fails, with the exception pending, only if a getter threw. */
static bool arg_is_array_buffer(jsb_ctx *ctx, int index, bool *out)
{
    jsb_val *off = jsb_get_prop(ctx, jsb_arg_val(ctx, index), "byteOffset");
    if (!off) return false;
    *out = jsb_val_type(ctx, off) == JSB_TYPE_UNDEFINED;
    return true;
}

/* Reads argument `index`'s bytes into `*buf` / `*size` when it is an
 * ArrayBuffer.
 *
 * Why: keeps upstream's "ArrayBuffer only" rule for updateTexture (see
 * arg_is_array_buffer). Returns false, with the pending exception dropped,
 * for any other value, so the caller throws upstream's own TypeError. The
 * bytes are borrowed from JS for the rest of the call. */
static bool arg_array_buffer(jsb_ctx *ctx, int index,
                             uint8_t **buf, size_t *size)
{
    bool is_ab = false;
    if (!arg_is_array_buffer(ctx, index, &is_ab) || !is_ab ||
        !jsb_arg_bytes(ctx, index, buf, size)) {
        jsb_clear_exception(ctx);
        return false;
    }
    return true;
}

/* Lifecycle */

/* init(width, height) -> bool
 *
 * Why: creates the renderer on first use and resizes it afterwards, so the
 * shim can call it on every canvas resize. Dimensions are coerced with
 * ToInt32; fewer than two arguments is a TypeError. Returns whether a
 * renderer exists afterwards. */
static void js_renderer_init(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "init requires width and height");
        return;
    }

    int32_t w, h;
    if (!jsb_arg_to_i32(ctx, 0, &w)) return;
    if (!jsb_arg_to_i32(ctx, 1, &h)) return;

    if (s_renderer) {
        renderer_resize(s_renderer, w, h);
        jsb_ret_bool(ctx, true);
        return;
    }

    s_renderer = renderer_create(w, h);
    jsb_ret_bool(ctx, s_renderer != NULL);
}

/* shutdown()
 *
 * Why: releases the renderer and every texture it owns; the shim calls it
 * when the canvas goes away. Safe when no renderer exists. */
static void js_renderer_shutdown(jsb_ctx *ctx)
{
    if (s_renderer) {
        renderer_destroy(s_renderer);
        s_renderer = NULL;
    }
    jsb_ret_undefined(ctx);
}

/* resize(width, height)
 *
 * Why: changes the logical viewport without recreating the renderer.
 * Dimensions are coerced with ToInt32; fewer than two arguments is a
 * TypeError. A no-op when no renderer exists. */
static void js_renderer_resize(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "resize requires width and height");
        return;
    }
    if (!s_renderer) {
        jsb_ret_undefined(ctx);
        return;
    }

    int32_t w, h;
    if (!jsb_arg_to_i32(ctx, 0, &w)) return;
    if (!jsb_arg_to_i32(ctx, 1, &h)) return;
    renderer_resize(s_renderer, w, h);
    jsb_ret_undefined(ctx);
}

/* getSize() -> {width, height} | null
 *
 * Why: the shim reads the logical size back after a resize. Returns null
 * when no renderer exists. */
static void js_renderer_get_size(jsb_ctx *ctx)
{
    if (!s_renderer) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    int w, h;
    renderer_get_size(s_renderer, &w, &h);
    jsb_val *obj = jsb_new_object(ctx);
    jsb_set_prop(ctx, obj, "width", jsb_new_i32(ctx, w));
    jsb_set_prop(ctx, obj, "height", jsb_new_i32(ctx, h));
    jsb_ret_val(ctx, obj);
}

/* Frame management */

/* beginFrame()
 *
 * Why: starts a frame cleared to opaque black. A no-op without a renderer. */
static void js_renderer_begin_frame(jsb_ctx *ctx)
{
    if (s_renderer) renderer_begin_frame(s_renderer);
    jsb_ret_undefined(ctx);
}

/* beginFrameTransparent() - clear to transparent black (for FBO rendering)
 *
 * Why: render textures need a transparent clear so their alpha composites
 * correctly. A no-op without a renderer. */
static void js_renderer_begin_frame_transparent(jsb_ctx *ctx)
{
    if (s_renderer) renderer_begin_frame_transparent(s_renderer);
    jsb_ret_undefined(ctx);
}

/* endFrame()
 *
 * Why: flushes the sprite batch and finishes the frame. A no-op without a
 * renderer. */
static void js_renderer_end_frame(jsb_ctx *ctx)
{
    if (s_renderer) renderer_end_frame(s_renderer);
    jsb_ret_undefined(ctx);
}

/* Texture management */

/* createTexture(width, height) -> textureId
 *
 * Why: allocates a blank RGBA texture for the shim's BaseTexture. Dimensions
 * are coerced with ToInt32; fewer than two arguments is a TypeError and a
 * non-positive or oversized size is a RangeError. Returns 0 when the
 * backend cannot create one. */
static void js_renderer_create_texture(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "createTexture requires width and height");
        return;
    }

    int32_t w, h;
    if (!jsb_arg_to_i32(ctx, 0, &w)) return;
    if (!jsb_arg_to_i32(ctx, 1, &h)) return;

    if (w <= 0 || h <= 0 || w > MAX_PIXEL_DIM || h > MAX_PIXEL_DIM) {
        jsb_throw_range(ctx, "createTexture: dimensions out of range %dx%d",
                        w, h);
        return;
    }

    uint32_t tex = renderer_create_texture(w, h);
    jsb_ret_val(ctx, jsb_new_u32(ctx, tex));
}

/* updateTexture(textureId, width, height, pixelsArrayBuffer)
 *
 * Why: uploads RGBA pixels into an existing texture. The first three
 * arguments are coerced; the fourth must be an ArrayBuffer (not a view),
 * as upstream required. A buffer smaller than w*h*4 is a RangeError. */
static void js_renderer_update_texture(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) {
        jsb_throw_type(ctx, "updateTexture requires texId, w, h, pixels");
        return;
    }

    uint32_t tex;
    int32_t w, h;
    if (!jsb_arg_to_u32(ctx, 0, &tex)) return;
    if (!jsb_arg_to_i32(ctx, 1, &w)) return;
    if (!jsb_arg_to_i32(ctx, 2, &h)) return;

    uint8_t *pixels = NULL;
    size_t buf_size = 0;
    if (!arg_array_buffer(ctx, 3, &pixels, &buf_size)) {
        jsb_throw_type(ctx, "updateTexture: fourth arg must be ArrayBuffer");
        return;
    }

    if (w <= 0 || h <= 0 || w > MAX_PIXEL_DIM || h > MAX_PIXEL_DIM ||
        buf_size < (size_t)w * (size_t)h * 4) {
        jsb_throw_range(ctx, "updateTexture: buffer too small for %dx%d", w, h);
        return;
    }

    renderer_update_texture(tex, w, h, pixels);
    jsb_ret_undefined(ctx);
}

/* deleteTexture(textureId)
 *
 * Why: frees a texture created by createTexture. The id is coerced with
 * ToUint32; fewer than one argument is a TypeError. */
static void js_renderer_delete_texture(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "deleteTexture requires textureId");
        return;
    }

    uint32_t tex;
    if (!jsb_arg_to_u32(ctx, 0, &tex)) return;
    renderer_delete_texture(tex);
    jsb_ret_undefined(ctx);
}

/* Render textures (FBO) */

/* createRenderTexture(width, height) -> {fbo, texture}
 *
 * Why: allocates an offscreen target for PIXI RenderTexture. Dimensions are
 * coerced with ToInt32; fewer than two arguments is a TypeError and a
 * non-positive or oversized size is a RangeError. Both ids are 0 when the
 * backend cannot create one. */
static void js_renderer_create_rt(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "createRenderTexture requires width and height");
        return;
    }

    int32_t w, h;
    if (!jsb_arg_to_i32(ctx, 0, &w)) return;
    if (!jsb_arg_to_i32(ctx, 1, &h)) return;

    if (w <= 0 || h <= 0 || w > MAX_PIXEL_DIM || h > MAX_PIXEL_DIM) {
        jsb_throw_range(ctx,
                        "createRenderTexture: dimensions out of range %dx%d",
                        w, h);
        return;
    }

    uint32_t tex = 0;
    uint32_t fbo = renderer_create_fbo(w, h, &tex);

    jsb_val *obj = jsb_new_object(ctx);
    jsb_set_prop(ctx, obj, "fbo", jsb_new_u32(ctx, fbo));
    jsb_set_prop(ctx, obj, "texture", jsb_new_u32(ctx, tex));
    jsb_ret_val(ctx, obj);
}

/* deleteRenderTexture(fbo, texture)
 *
 * Why: frees both ids createRenderTexture returned. Both are coerced with
 * ToUint32; fewer than two arguments is a TypeError. */
static void js_renderer_delete_rt(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "deleteRenderTexture requires fbo and texture");
        return;
    }

    uint32_t fbo, tex;
    if (!jsb_arg_to_u32(ctx, 0, &fbo)) return;
    if (!jsb_arg_to_u32(ctx, 1, &tex)) return;
    renderer_delete_fbo(fbo, tex);
    jsb_ret_undefined(ctx);
}

/* bindRenderTexture(fbo, width, height)
 *
 * Why: redirects drawing into a render texture. Arguments are coerced;
 * fewer than three is a TypeError. A no-op without a renderer. */
static void js_renderer_bind_rt(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 3) {
        jsb_throw_type(ctx, "bindRenderTexture requires fbo, width, height");
        return;
    }
    if (!s_renderer) {
        jsb_ret_undefined(ctx);
        return;
    }

    uint32_t fbo;
    int32_t w, h;
    if (!jsb_arg_to_u32(ctx, 0, &fbo)) return;
    if (!jsb_arg_to_i32(ctx, 1, &w)) return;
    if (!jsb_arg_to_i32(ctx, 2, &h)) return;
    renderer_bind_fbo(s_renderer, fbo, w, h);
    jsb_ret_undefined(ctx);
}

/* unbindRenderTexture()
 *
 * Why: returns drawing to the default framebuffer. A no-op without a
 * renderer. */
static void js_renderer_unbind_rt(jsb_ctx *ctx)
{
    if (s_renderer) renderer_unbind_fbo(s_renderer);
    jsb_ret_undefined(ctx);
}

/* Sprite batch drawing */

/* drawQuad(texture, x, y, w, h, u0, v0, u1, v1, tint, alpha)
 *
 * Why: the shim's hot path for one axis-aligned sprite. All arguments are
 * coerced (texture and tint with ToUint32, the rest with ToFloat64); fewer
 * than eleven is a TypeError. A no-op without a renderer. */
static void js_renderer_draw_quad(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 11) {
        jsb_throw_type(ctx, "drawQuad requires 11 arguments");
        return;
    }
    if (!s_renderer) {
        jsb_ret_undefined(ctx);
        return;
    }

    uint32_t texture;
    double x, y, w, h, u0, v0, u1, v1, alpha;
    uint32_t tint;

    if (!jsb_arg_to_u32(ctx, 0, &texture)) return;
    if (!jsb_arg_to_f64(ctx, 1, &x)) return;
    if (!jsb_arg_to_f64(ctx, 2, &y)) return;
    if (!jsb_arg_to_f64(ctx, 3, &w)) return;
    if (!jsb_arg_to_f64(ctx, 4, &h)) return;
    if (!jsb_arg_to_f64(ctx, 5, &u0)) return;
    if (!jsb_arg_to_f64(ctx, 6, &v0)) return;
    if (!jsb_arg_to_f64(ctx, 7, &u1)) return;
    if (!jsb_arg_to_f64(ctx, 8, &v1)) return;
    if (!jsb_arg_to_u32(ctx, 9, &tint)) return;
    if (!jsb_arg_to_f64(ctx, 10, &alpha)) return;

    SpriteBatch *batch = renderer_get_batch(s_renderer);
    if (batch) {
        sprite_batch_draw(batch, texture,
                          (float)x, (float)y, (float)w, (float)h,
                          (float)u0, (float)v0, (float)u1, (float)v1,
                          tint, (float)alpha);
    }
    jsb_ret_undefined(ctx);
}

/* drawQuadVertices(texture, x0,y0, x1,y1, x2,y2, x3,y3, u0,v0,u1,v1, tint, alpha)
 *
 * Why: draws a rotated, skewed or mirrored sprite from four corners in TL,
 * TR, BR, BL order. Arguments are coerced; fewer than fifteen is a
 * TypeError. A no-op without a renderer. */
static void js_renderer_draw_quad_verts(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 15) {
        jsb_throw_type(ctx, "drawQuadVertices requires 15 arguments");
        return;
    }
    if (!s_renderer) {
        jsb_ret_undefined(ctx);
        return;
    }

    uint32_t texture, tint;
    double p[8]; /* x0,y0,x1,y1,x2,y2,x3,y3 */
    double u0, v0, u1, v1, alpha;

    if (!jsb_arg_to_u32(ctx, 0, &texture)) return;
    for (int i = 0; i < 8; i++) {
        if (!jsb_arg_to_f64(ctx, 1 + i, &p[i])) return;
    }
    if (!jsb_arg_to_f64(ctx, 9, &u0))  return;
    if (!jsb_arg_to_f64(ctx, 10, &v0)) return;
    if (!jsb_arg_to_f64(ctx, 11, &u1)) return;
    if (!jsb_arg_to_f64(ctx, 12, &v1)) return;
    if (!jsb_arg_to_u32(ctx, 13, &tint)) return;
    if (!jsb_arg_to_f64(ctx, 14, &alpha)) return;

    SpriteBatch *batch = renderer_get_batch(s_renderer);
    if (batch) {
        sprite_batch_draw_verts(batch, texture,
                                (float)p[0], (float)p[1], (float)p[2], (float)p[3],
                                (float)p[4], (float)p[5], (float)p[6], (float)p[7],
                                (float)u0, (float)v0, (float)u1, (float)v1,
                                tint, (float)alpha);
    }
    jsb_ret_undefined(ctx);
}

/* drawQuadVerticesUV(texture, x0,y0, x1,y1, x2,y2, x3,y3,
 *                    u0,v0, u1,v1, u2,v2, u3,v3, tint, alpha)
 *
 * Why: as drawQuadVertices but with a UV per corner, for atlas sub-rects
 * that are not axis-aligned. Arguments are coerced; fewer than nineteen is
 * a TypeError. A no-op without a renderer. */
static void js_renderer_draw_quad_verts_uv(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 19) {
        jsb_throw_type(ctx, "drawQuadVerticesUV requires 19 arguments");
        return;
    }
    if (!s_renderer) {
        jsb_ret_undefined(ctx);
        return;
    }

    uint32_t texture, tint;
    float xy[8], uv[8];
    double d, alpha;

    if (!jsb_arg_to_u32(ctx, 0, &texture)) return;
    for (int i = 0; i < 8; i++) {
        if (!jsb_arg_to_f64(ctx, 1 + i, &d)) return;
        xy[i] = (float)d;
    }
    for (int i = 0; i < 8; i++) {
        if (!jsb_arg_to_f64(ctx, 9 + i, &d)) return;
        uv[i] = (float)d;
    }
    if (!jsb_arg_to_u32(ctx, 17, &tint)) return;
    if (!jsb_arg_to_f64(ctx, 18, &alpha)) return;

    SpriteBatch *batch = renderer_get_batch(s_renderer);
    if (batch) {
        sprite_batch_draw_verts_uv(batch, texture, xy, uv, tint, (float)alpha);
    }
    jsb_ret_undefined(ctx);
}

/* setTextureFilter(texture, linear)
 *
 * Why: mirrors PIXI's BaseTexture.scaleMode. The texture id is coerced with
 * ToUint32 and `linear` with ToBoolean; fewer than two arguments is a
 * TypeError. A no-op without a renderer. */
static void js_renderer_set_texture_filter(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "setTextureFilter requires texture and linear");
        return;
    }
    uint32_t texture;
    if (!jsb_arg_to_u32(ctx, 0, &texture)) return;
    bool linear = jsb_arg_truthy(ctx, 1);
    if (s_renderer) renderer_set_texture_filter(texture, linear);
    jsb_ret_undefined(ctx);
}

/* flush()
 *
 * Why: submits the pending sprite batch, for example before a filter pass
 * changes GL state. A no-op without a renderer. */
static void js_renderer_flush(jsb_ctx *ctx)
{
    if (s_renderer) {
        SpriteBatch *batch = renderer_get_batch(s_renderer);
        if (batch) sprite_batch_flush(batch);
    }
    jsb_ret_undefined(ctx);
}

/* setBlendMode(mode)
 *
 * Why: selects the sprite batch blend mode. The mode is coerced with
 * ToInt32; fewer than one argument is a TypeError. A no-op without a
 * renderer. */
static void js_renderer_set_blend(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "setBlendMode requires mode");
        return;
    }
    if (!s_renderer) {
        jsb_ret_undefined(ctx);
        return;
    }

    int32_t mode;
    if (!jsb_arg_to_i32(ctx, 0, &mode)) return;

    SpriteBatch *batch = renderer_get_batch(s_renderer);
    if (batch) sprite_batch_set_blend_mode(batch, mode);
    jsb_ret_undefined(ctx);
}

/* getDrawCalls() -> number
 *
 * Why: the shim's frame statistics. Returns 0 without a renderer. */
static void js_renderer_draw_calls(jsb_ctx *ctx)
{
    if (!s_renderer) {
        jsb_ret_i32(ctx, 0);
        return;
    }
    SpriteBatch *batch = renderer_get_batch(s_renderer);
    jsb_ret_i32(ctx, sprite_batch_get_draw_calls(batch));
}

/* getQuadCount() -> number
 *
 * Why: the shim's frame statistics. Returns 0 without a renderer. */
static void js_renderer_quad_count(jsb_ctx *ctx)
{
    if (!s_renderer) {
        jsb_ret_i32(ctx, 0);
        return;
    }
    SpriteBatch *batch = renderer_get_batch(s_renderer);
    jsb_ret_i32(ctx, sprite_batch_get_quad_count(batch));
}

/* rebindBatch() - restore sprite batch GL state after filter rendering
 *
 * Why: a filter pass leaves its own program and buffers bound; the shim
 * calls this before drawing again. A no-op without a renderer. */
static void js_renderer_rebind_batch(jsb_ctx *ctx)
{
    if (s_renderer) renderer_rebind_batch(s_renderer);
    jsb_ret_undefined(ctx);
}

/* setScissor(x, y, w, h) - enable scissor test with the given rect (GL pixel coords)
 *
 * Why: clips drawing to a rectangle. Arguments are coerced with ToInt32;
 * fewer than four is a TypeError. Pending draws are flushed first so the
 * new state applies to the next batch. A no-op without a renderer. */
static void js_renderer_set_scissor(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) {
        jsb_throw_type(ctx, "setScissor requires x, y, w, h");
        return;
    }
    if (!s_renderer) {
        jsb_ret_undefined(ctx);
        return;
    }

    int32_t x, y, w, h;
    if (!jsb_arg_to_i32(ctx, 0, &x)) return;
    if (!jsb_arg_to_i32(ctx, 1, &y)) return;
    if (!jsb_arg_to_i32(ctx, 2, &w)) return;
    if (!jsb_arg_to_i32(ctx, 3, &h)) return;

    /* Flush pending draws before changing scissor state. */
    SpriteBatch *batch = renderer_get_batch(s_renderer);
    if (batch) sprite_batch_flush(batch);

    renderer_set_scissor(s_renderer, x, y, w, h);
    jsb_ret_undefined(ctx);
}

/* clearScissor() - disable scissor test
 *
 * Why: undoes setScissor. Pending draws are flushed first. A no-op without
 * a renderer. */
static void js_renderer_clear_scissor(jsb_ctx *ctx)
{
    if (!s_renderer) {
        jsb_ret_undefined(ctx);
        return;
    }

    SpriteBatch *batch = renderer_get_batch(s_renderer);
    if (batch) sprite_batch_flush(batch);

    renderer_clear_scissor(s_renderer);
    jsb_ret_undefined(ctx);
}

/* setScreenViewport(x, y, w, h) - letterbox viewport for the default framebuffer
 *
 * Why: maps the game's logical size into the window with letterboxing.
 * Arguments are coerced with ToInt32; fewer than four is a TypeError. A
 * no-op without a renderer. */
static void js_renderer_set_screen_viewport(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) {
        jsb_throw_type(ctx, "setScreenViewport requires x, y, w, h");
        return;
    }
    if (!s_renderer) {
        jsb_ret_undefined(ctx);
        return;
    }

    int32_t x, y, w, h;
    if (!jsb_arg_to_i32(ctx, 0, &x)) return;
    if (!jsb_arg_to_i32(ctx, 1, &y)) return;
    if (!jsb_arg_to_i32(ctx, 2, &w)) return;
    if (!jsb_arg_to_i32(ctx, 3, &h)) return;
    renderer_set_screen_viewport(s_renderer, x, y, w, h);
    jsb_ret_undefined(ctx);
}

/* readPixels(fbo, width, height) -> ArrayBuffer | null
 *
 * Why: screenshots and the visual-regression harness read a render target
 * back as RGBA. Arguments are coerced; fewer than three is a TypeError and
 * a non-positive or oversized size is a RangeError. Returns null without a
 * renderer or when the read fails. The returned buffer is an engine-owned
 * copy; the native pixels are freed here. */
static void js_renderer_read_pixels(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 3) {
        jsb_throw_type(ctx, "readPixels requires fbo, width, height");
        return;
    }
    if (!s_renderer) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    uint32_t fbo;
    int32_t w, h;
    if (!jsb_arg_to_u32(ctx, 0, &fbo)) return;
    if (!jsb_arg_to_i32(ctx, 1, &w)) return;
    if (!jsb_arg_to_i32(ctx, 2, &h)) return;

    if (w <= 0 || h <= 0 || w > MAX_PIXEL_DIM || h > MAX_PIXEL_DIM) {
        jsb_throw_range(ctx, "readPixels: dimensions too large %dx%d", w, h);
        return;
    }

    uint8_t *pixels = renderer_read_pixels(s_renderer, fbo, w, h);
    if (!pixels) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    size_t len = (size_t)w * (size_t)h * 4;
    jsb_val *ab = jsb_new_buffer_copy(ctx, pixels, len);
    free(pixels);
    jsb_ret_val(ctx, ab);
}

/* Registration */

/* Fills __native_renderer in upstream's function-list order, with its
 * arities as each function's `length`. Returns false with a JS exception
 * pending. */
static bool renderer_module_init(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn_n(ctx, module, "init",                 js_renderer_init, 2) &&
           jsb_set_fn_n(ctx, module, "shutdown",             js_renderer_shutdown, 0) &&
           jsb_set_fn_n(ctx, module, "resize",               js_renderer_resize, 2) &&
           jsb_set_fn_n(ctx, module, "getSize",              js_renderer_get_size, 0) &&
           jsb_set_fn_n(ctx, module, "beginFrame",           js_renderer_begin_frame, 0) &&
           jsb_set_fn_n(ctx, module, "beginFrameTransparent", js_renderer_begin_frame_transparent, 0) &&
           jsb_set_fn_n(ctx, module, "endFrame",             js_renderer_end_frame, 0) &&
           jsb_set_fn_n(ctx, module, "createTexture",        js_renderer_create_texture, 2) &&
           jsb_set_fn_n(ctx, module, "updateTexture",        js_renderer_update_texture, 4) &&
           jsb_set_fn_n(ctx, module, "deleteTexture",        js_renderer_delete_texture, 1) &&
           jsb_set_fn_n(ctx, module, "setTextureFilter",     js_renderer_set_texture_filter, 2) &&
           jsb_set_fn_n(ctx, module, "createRenderTexture",  js_renderer_create_rt, 2) &&
           jsb_set_fn_n(ctx, module, "deleteRenderTexture",  js_renderer_delete_rt, 2) &&
           jsb_set_fn_n(ctx, module, "bindRenderTexture",    js_renderer_bind_rt, 3) &&
           jsb_set_fn_n(ctx, module, "unbindRenderTexture",  js_renderer_unbind_rt, 0) &&
           jsb_set_fn_n(ctx, module, "drawQuad",             js_renderer_draw_quad, 11) &&
           jsb_set_fn_n(ctx, module, "drawQuadVertices",     js_renderer_draw_quad_verts, 15) &&
           jsb_set_fn_n(ctx, module, "drawQuadVerticesUV",   js_renderer_draw_quad_verts_uv, 19) &&
           jsb_set_fn_n(ctx, module, "flush",                js_renderer_flush, 0) &&
           jsb_set_fn_n(ctx, module, "setBlendMode",         js_renderer_set_blend, 1) &&
           jsb_set_fn_n(ctx, module, "getDrawCalls",         js_renderer_draw_calls, 0) &&
           jsb_set_fn_n(ctx, module, "getQuadCount",         js_renderer_quad_count, 0) &&
           jsb_set_fn_n(ctx, module, "readPixels",           js_renderer_read_pixels, 3) &&
           jsb_set_fn_n(ctx, module, "rebindBatch",          js_renderer_rebind_batch, 0) &&
           jsb_set_fn_n(ctx, module, "setScissor",           js_renderer_set_scissor, 4) &&
           jsb_set_fn_n(ctx, module, "clearScissor",         js_renderer_clear_scissor, 0) &&
           jsb_set_fn_n(ctx, module, "setScreenViewport",    js_renderer_set_screen_viewport, 4);
}

/* jsb_runtime_setup callback that defines the module; `user` is unused. */
static bool renderer_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_renderer", renderer_module_init);
}

/* smzuggler: upstream's function list defined the functions non-enumerable
 * (writable, configurable); jsb_set_fn_n assigns them, which makes them
 * enumerable. This restores the old attributes so the JS API stays
 * byte-for-byte. Run once by bind_renderer_define, after the module exists. */
static const char renderer_hide_fns_js[] =
    "(function (m) {\n"
    "  Object.getOwnPropertyNames(m).forEach(function (k) {\n"
    "    Object.defineProperty(m, k, { enumerable: false });\n"
    "  });\n"
    "})(__native_renderer);\n";

/* Define the shim bridge on the borrowed runtime, on the main thread.
 * No handles survive setup; errors return false and are printed to stderr. */
bool bind_renderer_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_renderer_define: no jsb runtime\n");
        return false;
    }
    if (!jsb_runtime_setup(rt, renderer_define_setup, NULL))
        return false;
    char err[256];
    if (!jsb_runtime_eval(rt, renderer_hide_fns_js, "bind_renderer.c", err,
                          sizeof err)) {
        fprintf(stderr, "bind_renderer_define: %s\n", err);
        return false;
    }
    return true;
}
