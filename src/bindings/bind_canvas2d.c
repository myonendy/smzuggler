/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: ported from QuickJS to the jsb seam (spec section 4.1).
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 * Modified 2026-11-05: use jsb_arg_is_array_buffer and rely on non-enumerable jsb functions (card M1-T026b).
 *
 * __native_canvas2d: the CPU Canvas2D rasteriser behind the
 * HTMLCanvasElement / CanvasRenderingContext2D shim (docs/js-binding.md,
 * "bind_canvas2d"). Canvases are numeric handles owned by rendering/canvas2d.
 *
 * smzuggler: the port keeps Outsider's structure function by function so
 * upstream fixes re-apply by hand (docs/UPSTREAM.md). Every jsb_val here is
 * a call-scoped handle, never kept past the native call that made it, and
 * buffer bytes from jsb_arg_bytes are borrowed only after every argument
 * conversion (which may run JS) and only until the call returns (spec
 * section 15.8, docs/js-binding.md "Handle and lifetime rules").
 * Main thread only.
 */

#include "bindings/bind_canvas2d.h"
#include "rendering/canvas2d.h"
#include "rendering/image_loader.h"
#include "io/file_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_BUF_SIZE 4096

/* Cap on pixel dimensions so w*h*4 cannot overflow size_t: 16384 (GL max) is
   safe on 64-bit; on 32-bit 16384^2*4 == 2^32 wraps, so cap at 8192 there. */
#include <stdint.h>
#if SIZE_MAX > 0xFFFFFFFFu
#define MAX_PIXEL_DIM 16384
#else
#define MAX_PIXEL_DIM 8192
#endif

/* Borrows argument `index`'s bytes into `*buf` / `*size` when it is an
 * ArrayBuffer, as upstream's buffer read did.
 *
 * Why: keeps upstream's "ArrayBuffer only" rule; plain objects, including
 * ones spoofing ArrayBuffer.prototype, fail the shared brand check, and
 * views fail in jsb_arg_bytes. Returns false, with any pending exception
 * dropped, so the caller throws upstream's own TypeError. The bytes stay
 * owned by JS, valid until the call returns. */
static bool arg_array_buffer(jsb_ctx *ctx, int index,
                             uint8_t **buf, size_t *size)
{
    if (!jsb_arg_is_array_buffer(ctx, index) ||
        !jsb_arg_bytes(ctx, index, buf, size)) {
        jsb_clear_exception(ctx);
        return false;
    }
    return true;
}

/* Lifecycle */

/* create(width, height) -> handle | null
 *
 * Why: backing store of a new HTMLCanvasElement (compat: HTMLCanvasElement).
 * Sizes are ToInt32 in 1..MAX_PIXEL_DIM (else RangeError); fewer than two
 * arguments is a TypeError; null when canvas2d cannot allocate. The caller
 * frees the handle with destroy. */
static void js_c2d_create(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "create requires width and height");
        return;
    }

    int32_t w, h;
    if (!jsb_arg_to_i32(ctx, 0, &w) || !jsb_arg_to_i32(ctx, 1, &h))
        return;

    if (w <= 0 || h <= 0 || w > MAX_PIXEL_DIM || h > MAX_PIXEL_DIM) {
        jsb_throw_range(ctx, "create: dimensions out of range %dx%d", w, h);
        return;
    }

    Canvas2DHandle handle = canvas2d_create(w, h);
    if (handle == CANVAS2D_HANDLE_INVALID) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    jsb_ret_val(ctx, jsb_new_u32(ctx, handle));
}

/* destroy(handle)
 *
 * Why: frees a canvas the shim drops. `handle` is ToUint32; unknown handles
 * are ignored by canvas2d. No argument is a TypeError. */
static void js_c2d_destroy(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "destroy requires a handle");
        return;
    }
    uint32_t h;
    if (!jsb_arg_to_u32(ctx, 0, &h)) return;
    canvas2d_destroy(h);
    jsb_ret_undefined(ctx);
}

/* resize(handle, width, height)
 *
 * Why: backs setting canvas.width / canvas.height (compat:
 * HTMLCanvasElement). Sizes and errors as for create. */
static void js_c2d_resize(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 3) {
        jsb_throw_type(ctx, "resize requires handle, width, height");
        return;
    }
    uint32_t handle;
    int32_t w, h;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_i32(ctx, 1, &w)) return;
    if (!jsb_arg_to_i32(ctx, 2, &h)) return;

    if (w <= 0 || h <= 0 || w > MAX_PIXEL_DIM || h > MAX_PIXEL_DIM) {
        jsb_throw_range(ctx, "resize: dimensions out of range %dx%d", w, h);
        return;
    }

    canvas2d_resize(handle, w, h);
    jsb_ret_undefined(ctx);
}

/* getSize(handle) -> {width, height} | null
 *
 * Why: lets the shim read the real canvas size back. null for an unknown
 * handle; no argument is a TypeError. */
static void js_c2d_get_size(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "getSize requires a handle");
        return;
    }
    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    int w, h;
    if (!canvas2d_get_size(handle, &w, &h)) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }
    jsb_val *obj = jsb_new_object(ctx);
    jsb_set_prop(ctx, obj, "width", jsb_new_i32(ctx, w));
    jsb_set_prop(ctx, obj, "height", jsb_new_i32(ctx, h));
    jsb_ret_val(ctx, obj);
}

/* State setters */

/* setFillColor(handle, r, g, b, a)
 *
 * Why: CanvasRenderingContext2D.fillStyle after the shim parsed the CSS
 * colour. Channels are ToInt32 truncated to 8 bits, as upstream did. */
static void js_c2d_set_fill_color(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 5) {
        jsb_throw_type(ctx, "setFillColor requires handle,r,g,b,a");
        return;
    }
    uint32_t handle; int32_t r, g, b, a;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_i32(ctx, 1, &r)) return;
    if (!jsb_arg_to_i32(ctx, 2, &g)) return;
    if (!jsb_arg_to_i32(ctx, 3, &b)) return;
    if (!jsb_arg_to_i32(ctx, 4, &a)) return;
    canvas2d_set_fill_color(handle, (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
    jsb_ret_undefined(ctx);
}

/* setStrokeColor(handle, r, g, b, a)
 *
 * Why: CanvasRenderingContext2D.strokeStyle; as setFillColor. */
static void js_c2d_set_stroke_color(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 5) {
        jsb_throw_type(ctx, "setStrokeColor requires handle,r,g,b,a");
        return;
    }
    uint32_t handle; int32_t r, g, b, a;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_i32(ctx, 1, &r)) return;
    if (!jsb_arg_to_i32(ctx, 2, &g)) return;
    if (!jsb_arg_to_i32(ctx, 3, &b)) return;
    if (!jsb_arg_to_i32(ctx, 4, &a)) return;
    canvas2d_set_stroke_color(handle, (uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
    jsb_ret_undefined(ctx);
}

/* setGlobalAlpha(handle, alpha)
 *
 * Why: CanvasRenderingContext2D.globalAlpha; `alpha` is ToNumber and
 * clamped by canvas2d. */
static void js_c2d_set_global_alpha(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "setGlobalAlpha requires handle, alpha");
        return;
    }
    uint32_t handle; double alpha;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_f64(ctx, 1, &alpha)) return;
    canvas2d_set_global_alpha(handle, (float)alpha);
    jsb_ret_undefined(ctx);
}

/* setCompositeOp(handle, op)
 *
 * Why: CanvasRenderingContext2D.globalCompositeOperation, as the canvas2d
 * enum value the shim mapped it to (ToInt32). */
static void js_c2d_set_composite_op(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "setCompositeOp requires handle, op");
        return;
    }
    uint32_t handle; int32_t op;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_i32(ctx, 1, &op)) return;
    canvas2d_set_composite_op(handle, op);
    jsb_ret_undefined(ctx);
}

/* setLineWidth(handle, width)
 *
 * Why: CanvasRenderingContext2D.lineWidth, in pixels (ToNumber). */
static void js_c2d_set_line_width(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "setLineWidth requires handle, width");
        return;
    }
    uint32_t handle; double w;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_f64(ctx, 1, &w)) return;
    canvas2d_set_line_width(handle, (float)w);
    jsb_ret_undefined(ctx);
}

/* setTextAlign(handle, align)
 *
 * Why: CanvasRenderingContext2D.textAlign, as a canvas2d enum (ToInt32). */
static void js_c2d_set_text_align(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "setTextAlign requires handle, align");
        return;
    }
    uint32_t handle; int32_t align;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_i32(ctx, 1, &align)) return;
    canvas2d_set_text_align(handle, align);
    jsb_ret_undefined(ctx);
}

/* setTextBaseline(handle, baseline)
 *
 * Why: CanvasRenderingContext2D.textBaseline, as a canvas2d enum (ToInt32). */
static void js_c2d_set_text_baseline(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "setTextBaseline requires handle, baseline");
        return;
    }
    uint32_t handle; int32_t baseline;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_i32(ctx, 1, &baseline)) return;
    canvas2d_set_text_baseline(handle, baseline);
    jsb_ret_undefined(ctx);
}

/* State stack */

/* save(handle)
 *
 * Why: CanvasRenderingContext2D.save(). No argument is a TypeError. */
static void js_c2d_save(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "save requires a handle");
        return;
    }
    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    canvas2d_save(handle);
    jsb_ret_undefined(ctx);
}

/* restore(handle)
 *
 * Why: CanvasRenderingContext2D.restore(). No argument is a TypeError. */
static void js_c2d_restore(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "restore requires a handle");
        return;
    }
    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    canvas2d_restore(handle);
    jsb_ret_undefined(ctx);
}

/* Drawing operations */

/* fillRect(handle, x, y, w, h)
 *
 * Why: CanvasRenderingContext2D.fillRect in integer pixels (ToInt32). */
static void js_c2d_fill_rect(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 5) {
        jsb_throw_type(ctx, "fillRect requires handle,x,y,w,h");
        return;
    }
    uint32_t handle; int32_t x, y, w, h;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_i32(ctx, 1, &x)) return;
    if (!jsb_arg_to_i32(ctx, 2, &y)) return;
    if (!jsb_arg_to_i32(ctx, 3, &w)) return;
    if (!jsb_arg_to_i32(ctx, 4, &h)) return;
    canvas2d_fill_rect(handle, x, y, w, h);
    jsb_ret_undefined(ctx);
}

/* clearRect(handle, x, y, w, h)
 *
 * Why: CanvasRenderingContext2D.clearRect in integer pixels (ToInt32). */
static void js_c2d_clear_rect(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 5) {
        jsb_throw_type(ctx, "clearRect requires handle,x,y,w,h");
        return;
    }
    uint32_t handle; int32_t x, y, w, h;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_i32(ctx, 1, &x)) return;
    if (!jsb_arg_to_i32(ctx, 2, &y)) return;
    if (!jsb_arg_to_i32(ctx, 3, &w)) return;
    if (!jsb_arg_to_i32(ctx, 4, &h)) return;
    canvas2d_clear_rect(handle, x, y, w, h);
    jsb_ret_undefined(ctx);
}

/* strokeRect(handle, x, y, w, h)
 *
 * Why: CanvasRenderingContext2D.strokeRect in integer pixels (ToInt32). */
static void js_c2d_stroke_rect(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 5) {
        jsb_throw_type(ctx, "strokeRect requires handle,x,y,w,h");
        return;
    }
    uint32_t handle; int32_t x, y, w, h;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_i32(ctx, 1, &x)) return;
    if (!jsb_arg_to_i32(ctx, 2, &y)) return;
    if (!jsb_arg_to_i32(ctx, 3, &w)) return;
    if (!jsb_arg_to_i32(ctx, 4, &h)) return;
    canvas2d_stroke_rect(handle, x, y, w, h);
    jsb_ret_undefined(ctx);
}

/* drawImage(handle, srcPixels, srcW, srcH, sx, sy, sw, sh, dx, dy, dw, dh)
 *
 * Why: CanvasRenderingContext2D.drawImage with RGBA pixels the shim holds.
 * Numbers are ToInt32. A non-ArrayBuffer source is a TypeError; a source
 * size outside 1..MAX_PIXEL_DIM or under srcW*srcH*4 bytes is a RangeError.
 * The pixels are borrowed for the call only. */
static void js_c2d_draw_image(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 12) {
        jsb_throw_type(ctx, "drawImage requires 12 arguments");
        return;
    }

    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;

    int32_t src_w, src_h, sx, sy, sw, sh, dx, dy, dw, dh;
    if (!jsb_arg_to_i32(ctx, 2, &src_w)) return;
    if (!jsb_arg_to_i32(ctx, 3, &src_h)) return;
    if (!jsb_arg_to_i32(ctx, 4, &sx)) return;
    if (!jsb_arg_to_i32(ctx, 5, &sy)) return;
    if (!jsb_arg_to_i32(ctx, 6, &sw)) return;
    if (!jsb_arg_to_i32(ctx, 7, &sh)) return;
    if (!jsb_arg_to_i32(ctx, 8, &dx)) return;
    if (!jsb_arg_to_i32(ctx, 9, &dy)) return;
    if (!jsb_arg_to_i32(ctx, 10, &dw)) return;
    if (!jsb_arg_to_i32(ctx, 11, &dh)) return;

    /* Fetch the pixel ArrayBuffer (argv[1]) only after the conversions above:
       they can run user JS (valueOf) that detaches it and leaves a stale pointer. */
    size_t buf_size = 0;
    uint8_t *src_pixels = NULL;
    if (!arg_array_buffer(ctx, 1, &src_pixels, &buf_size) || !src_pixels) {
        jsb_throw_type(ctx, "drawImage: second arg must be ArrayBuffer");
        return;
    }

    if (src_w <= 0 || src_h <= 0 || src_w > MAX_PIXEL_DIM || src_h > MAX_PIXEL_DIM ||
        buf_size < (size_t)src_w * (size_t)src_h * 4) {
        jsb_throw_range(ctx, "drawImage: buffer size mismatch for %dx%d", src_w, src_h);
        return;
    }

    canvas2d_draw_image(handle, src_pixels, src_w, src_h,
                         sx, sy, sw, sh, dx, dy, dw, dh);
    jsb_ret_undefined(ctx);
}

/* drawImageHandle(handle, imageHandle, sx, sy, sw, sh, dx, dy, dw, dh)
 *
 * Why: like drawImage but takes an image_loader handle instead of a pixel
 * ArrayBuffer, so decoded images are drawn without a JS copy. An unknown
 * image returns undefined before the rectangle is converted, as upstream. */
static void js_c2d_draw_image_handle(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 10) {
        jsb_throw_type(ctx, "drawImageHandle requires 10 arguments");
        return;
    }

    uint32_t handle, img_handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_u32(ctx, 1, &img_handle)) return;

    ImageInfo info;
    const uint8_t *pixels = NULL;
    if (!image_get_info(img_handle, &info) ||
        !(pixels = image_get_pixels(img_handle))) {
        jsb_ret_undefined(ctx);
        return;
    }

    int32_t sx, sy, sw, sh, dx, dy, dw, dh;
    if (!jsb_arg_to_i32(ctx, 2, &sx)) return;
    if (!jsb_arg_to_i32(ctx, 3, &sy)) return;
    if (!jsb_arg_to_i32(ctx, 4, &sw)) return;
    if (!jsb_arg_to_i32(ctx, 5, &sh)) return;
    if (!jsb_arg_to_i32(ctx, 6, &dx)) return;
    if (!jsb_arg_to_i32(ctx, 7, &dy)) return;
    if (!jsb_arg_to_i32(ctx, 8, &dw)) return;
    if (!jsb_arg_to_i32(ctx, 9, &dh)) return;

    canvas2d_draw_image(handle, pixels, info.width, info.height,
                         sx, sy, sw, sh, dx, dy, dw, dh);
    jsb_ret_undefined(ctx);
}

/* Pixel manipulation */

/* getImageData(handle, x, y, w, h) -> ArrayBuffer (RGBA) | null
 *
 * Why: CanvasRenderingContext2D.getImageData; the shim wraps the bytes in
 * an ImageData. Returns a copy the engine owns. A non-positive size gives
 * null; a side over MAX_PIXEL_DIM or more than 64 MiB is a RangeError. */
static void js_c2d_get_image_data(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 5) {
        jsb_throw_type(ctx, "getImageData requires handle,x,y,w,h");
        return;
    }
    uint32_t handle; int32_t x, y, w, h;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    if (!jsb_arg_to_i32(ctx, 1, &x)) return;
    if (!jsb_arg_to_i32(ctx, 2, &y)) return;
    if (!jsb_arg_to_i32(ctx, 3, &w)) return;
    if (!jsb_arg_to_i32(ctx, 4, &h)) return;

    if (w <= 0 || h <= 0) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }
    if (w > MAX_PIXEL_DIM || h > MAX_PIXEL_DIM) {
        jsb_throw_range(ctx, "getImageData: dimensions too large %dx%d", w, h);
        return;
    }

    size_t buf_size = (size_t)w * (size_t)h * 4;
    if (buf_size > 64 * 1024 * 1024) {
        jsb_throw_range(ctx, "getImageData: total size too large (%zu bytes)", buf_size);
        return;
    }
    /* calloc: an invalid handle or out-of-range rect yields zeroed pixels, not uninitialized heap. */
    uint8_t *buf = calloc(buf_size, 1);
    if (!buf) {
        jsb_throw_internal(ctx, "out of memory");
        return;
    }

    canvas2d_get_image_data(handle, x, y, w, h, buf);

    jsb_val *ab = jsb_new_buffer_copy(ctx, buf, buf_size);
    free(buf);
    jsb_ret_val(ctx, ab);
}

/* putImageData(handle, arrayBuffer, x, y, w, h)
 *
 * Why: CanvasRenderingContext2D.putImageData with the shim's RGBA bytes,
 * borrowed for the call. A non-ArrayBuffer is a TypeError; a size outside
 * 1..MAX_PIXEL_DIM or fewer than w*h*4 bytes is a RangeError. */
static void js_c2d_put_image_data(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 6) {
        jsb_throw_type(ctx, "putImageData requires handle,data,x,y,w,h");
        return;
    }
    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;

    int32_t x, y, w, h;
    if (!jsb_arg_to_i32(ctx, 2, &x)) return;
    if (!jsb_arg_to_i32(ctx, 3, &y)) return;
    if (!jsb_arg_to_i32(ctx, 4, &w)) return;
    if (!jsb_arg_to_i32(ctx, 5, &h)) return;

    /* Fetch the ArrayBuffer only after the conversions above: they can run user
       JS (valueOf) that detaches it and leaves a stale pointer. */
    size_t buf_size = 0;
    uint8_t *data = NULL;
    if (!arg_array_buffer(ctx, 1, &data, &buf_size) || !data) {
        jsb_throw_type(ctx, "putImageData: second arg must be ArrayBuffer");
        return;
    }

    if (w <= 0 || h <= 0 || w > MAX_PIXEL_DIM || h > MAX_PIXEL_DIM ||
        buf_size < (size_t)w * (size_t)h * 4) {
        jsb_throw_range(ctx, "putImageData: buffer size mismatch for %dx%d", w, h);
        return;
    }

    canvas2d_put_image_data(handle, data, x, y, w, h);
    jsb_ret_undefined(ctx);
}

/* getPixels(handle) -> ArrayBuffer (full canvas RGBA) | null
 *
 * Why: lets the renderer shim upload a whole canvas as a texture. Returns a
 * copy the engine owns; null for an unknown handle. */
static void js_c2d_get_pixels(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "getPixels requires a handle");
        return;
    }
    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;

    int w, h;
    uint8_t *pixels = NULL;
    if (!canvas2d_get_size(handle, &w, &h) ||
        !(pixels = canvas2d_get_pixels(handle))) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    if (w <= 0 || h <= 0 || w > MAX_PIXEL_DIM || h > MAX_PIXEL_DIM) {
        jsb_throw_range(ctx, "getPixels: canvas dimensions out of range %dx%d", w, h);
        return;
    }

    jsb_ret_val(ctx, jsb_new_buffer_copy(ctx, pixels, (size_t)w * (size_t)h * 4));
}

/* Font and text */

/* loadFont(name, arrayBuffer) -> bool
 *
 * Why: FontFace loading from memory (compat: FontFace). `name` is ToString;
 * a non-ArrayBuffer is a TypeError; false when the font does not parse. The
 * bytes are borrowed; canvas2d copies what it keeps. */
static void js_c2d_load_font(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "loadFont requires name and ArrayBuffer");
        return;
    }

    char *name = jsb_arg_to_str(ctx, 0, NULL);
    if (!name) return;

    size_t buf_size = 0;
    uint8_t *data = NULL;
    if (!arg_array_buffer(ctx, 1, &data, &buf_size) || !data) {
        jsb_free_str(name);
        jsb_throw_type(ctx, "loadFont: second arg must be ArrayBuffer");
        return;
    }

    bool ok = canvas2d_load_font(name, data, buf_size);
    jsb_free_str(name);
    jsb_ret_bool(ctx, ok);
}

/* loadFontFile(name, path) -> bool
 *
 * Why: loads a game font by game-relative URL (compat: FontFace with a url()
 * source). Both are ToString; false when the path leaves the game root, the
 * file cannot be read or the font does not parse. */
static void js_c2d_load_font_file(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "loadFontFile requires name and path");
        return;
    }

    char *name = jsb_arg_to_str(ctx, 0, NULL);
    if (!name) return;

    char *path = jsb_arg_to_str(ctx, 1, NULL);
    if (!path) {
        jsb_free_str(name);
        return;
    }

    char resolved[PATH_BUF_SIZE];
    if (!file_io_resolve_path(path, resolved, sizeof(resolved))) {
        jsb_free_str(path);
        jsb_free_str(name);
        jsb_ret_bool(ctx, false);
        return;
    }
    jsb_free_str(path);

    size_t file_size = 0;
    uint8_t *data = file_io_read_binary(resolved, &file_size);
    if (!data) {
        jsb_free_str(name);
        jsb_ret_bool(ctx, false);
        return;
    }

    bool ok = canvas2d_load_font(name, data, file_size);
    free(data);
    jsb_free_str(name);
    jsb_ret_bool(ctx, ok);
}

/* setFont(handle, name, pixelSize)
 *
 * Why: CanvasRenderingContext2D.font after the shim parsed the CSS font
 * string. `name` is ToString, `pixelSize` ToNumber, in that order. */
static void js_c2d_set_font(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 3) {
        jsb_throw_type(ctx, "setFont requires handle, name, pixelSize");
        return;
    }
    uint32_t handle; double size;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    char *name = jsb_arg_to_str(ctx, 1, NULL);
    if (!name) return;
    if (!jsb_arg_to_f64(ctx, 2, &size)) {
        jsb_free_str(name);
        return;
    }
    canvas2d_set_font(handle, name, (float)size);
    jsb_free_str(name);
    jsb_ret_undefined(ctx);
}

/* fillText(handle, text, x, y)
 *
 * Why: CanvasRenderingContext2D.fillText (no maxWidth). `text` is ToString,
 * x and y ToNumber, converted in that order. */
static void js_c2d_fill_text(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) {
        jsb_throw_type(ctx, "fillText requires handle,text,x,y");
        return;
    }
    uint32_t handle; double x, y;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    char *text = jsb_arg_to_str(ctx, 1, NULL);
    if (!text) return;
    if (!jsb_arg_to_f64(ctx, 2, &x)) { jsb_free_str(text); return; }
    if (!jsb_arg_to_f64(ctx, 3, &y)) { jsb_free_str(text); return; }
    canvas2d_fill_text(handle, text, (float)x, (float)y);
    jsb_free_str(text);
    jsb_ret_undefined(ctx);
}

/* strokeText(handle, text, x, y)
 *
 * Why: CanvasRenderingContext2D.strokeText (no maxWidth); as fillText. */
static void js_c2d_stroke_text(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) {
        jsb_throw_type(ctx, "strokeText requires handle,text,x,y");
        return;
    }
    uint32_t handle; double x, y;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    char *text = jsb_arg_to_str(ctx, 1, NULL);
    if (!text) return;
    if (!jsb_arg_to_f64(ctx, 2, &x)) { jsb_free_str(text); return; }
    if (!jsb_arg_to_f64(ctx, 3, &y)) { jsb_free_str(text); return; }
    canvas2d_stroke_text(handle, text, (float)x, (float)y);
    jsb_free_str(text);
    jsb_ret_undefined(ctx);
}

/* measureText(handle, text) -> number (width in pixels)
 *
 * Why: CanvasRenderingContext2D.measureText().width; the shim builds the
 * TextMetrics object. 0 for an unknown handle or no font. */
static void js_c2d_measure_text(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "measureText requires handle and text");
        return;
    }
    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    char *text = jsb_arg_to_str(ctx, 1, NULL);
    if (!text) return;
    float w = canvas2d_measure_text(handle, text);
    jsb_free_str(text);
    jsb_ret_f64(ctx, (double)w);
}

/* Registration */

/* Fills __native_canvas2d in upstream's function-list order, with its
 * upstream arities as each function's `length`. Returns false with a
 * JS exception pending. */
static bool c2d_module_init(jsb_ctx *ctx, jsb_val *m)
{
    return jsb_set_fn_n(ctx, m, "create",          js_c2d_create, 2) &&
           jsb_set_fn_n(ctx, m, "destroy",         js_c2d_destroy, 1) &&
           jsb_set_fn_n(ctx, m, "resize",          js_c2d_resize, 3) &&
           jsb_set_fn_n(ctx, m, "getSize",         js_c2d_get_size, 1) &&
           jsb_set_fn_n(ctx, m, "setFillColor",    js_c2d_set_fill_color, 5) &&
           jsb_set_fn_n(ctx, m, "setStrokeColor",  js_c2d_set_stroke_color, 5) &&
           jsb_set_fn_n(ctx, m, "setGlobalAlpha",  js_c2d_set_global_alpha, 2) &&
           jsb_set_fn_n(ctx, m, "setCompositeOp",  js_c2d_set_composite_op, 2) &&
           jsb_set_fn_n(ctx, m, "setLineWidth",    js_c2d_set_line_width, 2) &&
           jsb_set_fn_n(ctx, m, "setTextAlign",    js_c2d_set_text_align, 2) &&
           jsb_set_fn_n(ctx, m, "setTextBaseline", js_c2d_set_text_baseline, 2) &&
           jsb_set_fn_n(ctx, m, "save",            js_c2d_save, 1) &&
           jsb_set_fn_n(ctx, m, "restore",         js_c2d_restore, 1) &&
           jsb_set_fn_n(ctx, m, "fillRect",        js_c2d_fill_rect, 5) &&
           jsb_set_fn_n(ctx, m, "clearRect",       js_c2d_clear_rect, 5) &&
           jsb_set_fn_n(ctx, m, "strokeRect",      js_c2d_stroke_rect, 5) &&
           jsb_set_fn_n(ctx, m, "drawImage",       js_c2d_draw_image, 12) &&
           jsb_set_fn_n(ctx, m, "drawImageHandle", js_c2d_draw_image_handle, 10) &&
           jsb_set_fn_n(ctx, m, "getImageData",    js_c2d_get_image_data, 5) &&
           jsb_set_fn_n(ctx, m, "putImageData",    js_c2d_put_image_data, 6) &&
           jsb_set_fn_n(ctx, m, "getPixels",       js_c2d_get_pixels, 1) &&
           jsb_set_fn_n(ctx, m, "loadFont",        js_c2d_load_font, 2) &&
           jsb_set_fn_n(ctx, m, "loadFontFile",    js_c2d_load_font_file, 2) &&
           jsb_set_fn_n(ctx, m, "setFont",         js_c2d_set_font, 3) &&
           jsb_set_fn_n(ctx, m, "fillText",        js_c2d_fill_text, 4) &&
           jsb_set_fn_n(ctx, m, "strokeText",      js_c2d_stroke_text, 4) &&
           jsb_set_fn_n(ctx, m, "measureText",     js_c2d_measure_text, 2);
}

/* jsb_runtime_setup callback that defines the module; `user` is unused. */
static bool c2d_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_canvas2d", c2d_module_init);
}

/* Defines the shim bridge on the borrowed runtime, on the main thread.
 * No handles survive setup; errors return false and are printed to stderr. */
bool bind_canvas2d_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_canvas2d_define: no jsb runtime\n");
        return false;
    }
    return jsb_runtime_setup(rt, c2d_define_setup, NULL);
}
