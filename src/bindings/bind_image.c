/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: ported from QuickJS to the jsb seam (spec section 4.1).
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 * Modified 2026-11-05: use jsb_arg_is_array_buffer and rely on non-enumerable jsb functions (card M1-T026b).
 *
 * __native_image: decodes images (files, in-memory blobs, APNG) for the
 * HTMLImageElement and Bitmap shims and exposes the image cache
 * (docs/js-binding.md, "bind_image").
 *
 * smzuggler: the port keeps Outsider's structure function by function so
 * upstream fixes re-apply by hand (docs/UPSTREAM.md). Every jsb_val here is
 * a call-scoped handle, never kept past the native call that made it, and
 * buffer bytes from jsb_arg_bytes are used only before the call returns
 * (spec section 15.8, docs/js-binding.md "Handle and lifetime rules").
 * Main thread only.
 */

#include "bindings/bind_image.h"
#include "rendering/image_loader.h"
#include "rendering/apng.h"
#include "io/file_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_BUF_SIZE 4096

/* Resolve JS argument `index` to an absolute path under the game root.
 *
 * Why: loadImage takes game-relative URLs; file_io keeps them inside the
 * game folder. The argument is coerced with ToString, as the original binding did.
 * Returns the converted string (caller frees with jsb_free_str) and fills
 * `out` (`out_size` bytes), or NULL with a pending exception: the
 * conversion's own, or a TypeError when the path escapes the root. */
static char *resolve_path_arg(jsb_ctx *ctx, int index,
                              char *out, size_t out_size)
{
    char *raw = jsb_arg_to_str(ctx, index, NULL);
    if (!raw) return NULL;

    if (!file_io_resolve_path(raw, out, out_size)) {
        jsb_throw_type(ctx, "path resolution failed: %s", raw);
        jsb_free_str(raw);
        return NULL;
    }
    return raw;
}

/* Build a {width, height, handle, glTexture} JS object from an ImageHandle.
 *
 * Why: the shape loadImage and loadImageFromMemory share. Returns a
 * call-scoped handle, or NULL after an InternalError when the cache no
 * longer knows `handle`. */
static jsb_val *make_image_result(jsb_ctx *ctx, ImageHandle handle)
{
    ImageInfo info;
    if (!image_get_info(handle, &info)) {
        jsb_throw_internal(ctx, "failed to query image info");
        return NULL;
    }

    jsb_val *obj = jsb_new_object(ctx);
    jsb_set_prop(ctx, obj, "width",     jsb_new_i32(ctx, info.width));
    jsb_set_prop(ctx, obj, "height",    jsb_new_i32(ctx, info.height));
    jsb_set_prop(ctx, obj, "handle",    jsb_new_u32(ctx, handle));
    jsb_set_prop(ctx, obj, "glTexture", jsb_new_u32(ctx, info.gl_texture));
    return obj;
}

/* Loads `index`'s bytes into `*buf` / `*size` when it is an ArrayBuffer.
 *
 * Why: keeps upstream's "ArrayBuffer only" rule (see jsb_arg_is_array_buffer).
 * Returns false, with the pending exception dropped, for any other value,
 * so the caller throws upstream's own TypeError. The bytes are borrowed
 * from JS for the rest of the call. */
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

/* loadImage(path) -> {width, height, handle, glTexture} | null
 *
 * Why: backs HTMLImageElement.src for game files (compat: Image). The path
 * is coerced with ToString. Fewer than one argument is a TypeError; a path
 * outside the game root, or an image that fails to decode, gives null. */
static void js_load_image(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "loadImage requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_path_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) {
        /* A path outside the game root is a load failure, not a JS error: the
           image shim expects null so it can fire onerror. */
        jsb_clear_exception(ctx);
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }
    jsb_free_str(raw);

    ImageHandle handle = image_load(resolved);
    if (handle == IMAGE_HANDLE_INVALID) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    jsb_val *result = make_image_result(ctx, handle);
    if (result) jsb_ret_val(ctx, result);
}

/* loadImageFromMemory(key, arrayBuffer) -> {width, height, handle, glTexture} | null
 *
 * Why: backs images made from Blobs and data URLs; `key` (ToString) is the
 * cache key, e.g. a blob: URL. Fewer than two arguments, or a second that
 * is not an ArrayBuffer, is a TypeError; a decode failure gives null. */
static void js_load_image_from_memory(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "loadImageFromMemory requires key and ArrayBuffer");
        return;
    }

    char *key = jsb_arg_to_str(ctx, 0, NULL);
    if (!key) return;

    size_t size = 0;
    uint8_t *buf = NULL;
    if (!arg_array_buffer(ctx, 1, &buf, &size)) {
        jsb_free_str(key);
        jsb_throw_type(ctx, "second argument must be an ArrayBuffer");
        return;
    }

    ImageHandle handle = image_load_from_memory(key, buf, size);
    jsb_free_str(key);

    if (handle == IMAGE_HANDLE_INVALID) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    jsb_val *result = make_image_result(ctx, handle);
    if (result) jsb_ret_val(ctx, result);
}

/* getImageInfo(handle) -> {width, height, glTexture} | null
 *
 * Why: lets the shims re-read a cached image's size and texture. `handle`
 * is coerced with ToUint32; no argument is a TypeError, an unknown handle
 * gives null. */
static void js_get_image_info(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "getImageInfo requires a handle argument");
        return;
    }

    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle))
        return;

    ImageInfo info;
    if (!image_get_info(handle, &info)) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    jsb_val *obj = jsb_new_object(ctx);
    jsb_set_prop(ctx, obj, "width",     jsb_new_i32(ctx, info.width));
    jsb_set_prop(ctx, obj, "height",    jsb_new_i32(ctx, info.height));
    jsb_set_prop(ctx, obj, "glTexture", jsb_new_u32(ctx, info.gl_texture));
    jsb_ret_val(ctx, obj);
}

/* getImagePixels(handle) -> ArrayBuffer (RGBA) | null
 *
 * Why: backs getImageData on canvases drawn from an image. Returns a copy
 * (the engine owns it) of width*height*4 bytes. `handle` is ToUint32; no
 * argument is a TypeError, an unknown handle or no pixels gives null. */
static void js_get_image_pixels(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "getImagePixels requires a handle argument");
        return;
    }

    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle))
        return;

    ImageInfo info;
    if (!image_get_info(handle, &info)) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    const uint8_t *pixels = image_get_pixels(handle);
    if (!pixels) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    size_t byte_size = (size_t)info.width * (size_t)info.height * 4;
    jsb_val *ab = jsb_new_buffer_copy(ctx, pixels, byte_size);
    if (ab) jsb_ret_val(ctx, ab);
}

/* freeImage(handle)
 *
 * Why: drops the shim's reference to a cached image. `handle` is
 * ToUint32; no argument is a TypeError. Returns undefined. */
static void js_free_image(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "freeImage requires a handle argument");
        return;
    }

    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle))
        return;

    image_free(handle);
    jsb_ret_undefined(ctx);
}

/* cacheCount() -> number
 *
 * Why: lets tests and diagnostics see the cache size. Arguments ignored. */
static void js_cache_count(jsb_ctx *ctx)
{
    jsb_ret_val(ctx, jsb_new_u32(ctx, (uint32_t)image_cache_count()));
}

/* clearCache()
 *
 * Why: frees every cached image, e.g. between scenes. Arguments ignored;
 * returns undefined. */
static void js_clear_cache(jsb_ctx *ctx)
{
    image_cache_clear();
    jsb_ret_undefined(ctx);
}

/* Reads decodeApng's argument: an ArrayBuffer or a TypedArray view.
 *
 * Why: upstream tried an ArrayBuffer, then a TypedArray, so a
 * DataView was refused; jsb_arg_bytes accepts it, so it is refused here
 * (a DataView has `byteOffset` but no `BYTES_PER_ELEMENT`). Returns false
 * with no exception pending for anything else. Bytes are borrowed. */
static bool apng_arg_bytes(jsb_ctx *ctx, uint8_t **buf, size_t *size)
{
    if (!jsb_arg_is_array_buffer(ctx, 0)) {
        jsb_val *bpe = jsb_get_prop(ctx, jsb_arg_val(ctx, 0), "BYTES_PER_ELEMENT");
        if (!bpe || jsb_val_type(ctx, bpe) == JSB_TYPE_UNDEFINED) {
            jsb_clear_exception(ctx);
            return false;
        }
    }
    /* Read bytes last: property getters can detach the backing buffer. */
    if (!jsb_arg_bytes(ctx, 0, buf, size)) {
        jsb_clear_exception(ctx);
        return false;
    }
    return true;
}

/* decodeApng(ArrayBuffer) -> {width, height, numPlays,
                               frames: [{delay, glTexture}]} | null
   Frames are composed RGBA textures owned by the caller (delete them through
   __native_renderer.deleteTexture). null when the buffer is not an APNG or
   fails to decode, so the caller can fall back to its own decoder.

   Why: animated PNGs (compat: Image, APNG) need per-frame textures the
   browser would compose itself. No argument, or one that is neither an
   ArrayBuffer nor a TypedArray, is a TypeError. */
static void js_decode_apng(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "decodeApng requires an ArrayBuffer");
        return;
    }

    size_t size = 0;
    uint8_t *buf = NULL;
    if (!apng_arg_bytes(ctx, &buf, &size)) {
        jsb_throw_type(ctx, "decodeApng requires an ArrayBuffer");
        return;
    }

    ApngImage *img = apng_decode(buf, size);
    if (!img) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    jsb_val *frames = jsb_new_array(ctx);
    for (int i = 0; i < img->num_frames; i++) {
        jsb_val *f = jsb_new_object(ctx);
        uint32_t tex = image_upload_rgba_texture(img->frames[i].rgba, img->width, img->height);
        jsb_set_prop(ctx, f, "delay",     jsb_new_i32(ctx, img->frames[i].delay_ms));
        jsb_set_prop(ctx, f, "glTexture", jsb_new_u32(ctx, tex));
        jsb_set_index(ctx, frames, (uint32_t)i, f);
    }

    jsb_val *obj = jsb_new_object(ctx);
    jsb_set_prop(ctx, obj, "width",    jsb_new_i32(ctx, img->width));
    jsb_set_prop(ctx, obj, "height",   jsb_new_i32(ctx, img->height));
    jsb_set_prop(ctx, obj, "numPlays", jsb_new_i32(ctx, img->num_plays));
    jsb_set_prop(ctx, obj, "frames",   frames);
    apng_free(img);
    jsb_ret_val(ctx, obj);
}

/* Registration */

/* Fills __native_image in upstream's function-list order, with its arities
 * as each function's `length`. Returns false with a JS exception pending. */
static bool image_module_init(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn_n(ctx, module, "decodeApng",          js_decode_apng, 1) &&
           jsb_set_fn_n(ctx, module, "loadImage",           js_load_image, 1) &&
           jsb_set_fn_n(ctx, module, "loadImageFromMemory", js_load_image_from_memory, 2) &&
           jsb_set_fn_n(ctx, module, "getImageInfo",        js_get_image_info, 1) &&
           jsb_set_fn_n(ctx, module, "getImagePixels",      js_get_image_pixels, 1) &&
           jsb_set_fn_n(ctx, module, "freeImage",           js_free_image, 1) &&
           jsb_set_fn_n(ctx, module, "cacheCount",          js_cache_count, 0) &&
           jsb_set_fn_n(ctx, module, "clearCache",          js_clear_cache, 0);
}

/* jsb_runtime_setup callback that defines the module; `user` is unused. */
static bool image_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_image", image_module_init);
}

/* Define the shim bridge on the borrowed runtime, on the main thread.
 * No handles survive setup; errors return false and are printed to stderr. */
bool bind_image_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_image_define: no jsb runtime\n");
        return false;
    }
    return jsb_runtime_setup(rt, image_define_setup, NULL);
}
