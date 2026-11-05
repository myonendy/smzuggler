/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: ported from QuickJS to the jsb seam (spec section 4.1).
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 *
 * __native_tilemap: draws packed map layers through the renderer's sprite
 * batch for tilemap_shim.js (docs/js-binding.md, "bind_tilemap").
 *
 * smzuggler: the port keeps Outsider's structure line by line so upstream
 * fixes re-apply by hand (docs/UPSTREAM.md). Every jsb_val here is a
 * call-scoped handle, never kept past the native call that made it (spec
 * section 15.8, docs/js-binding.md "Handle and lifetime rules"). Main
 * thread only; the renderer itself owns the sprite batch.
 */

#include "bindings/bind_tilemap.h"
#include "rendering/tilemap.h"
#include "rendering/renderer.h"

#include <stdio.h>
#include <stdlib.h>

/* Global renderer, defined in bind_renderer.c; drawTiles uses its sprite batch. */
extern Renderer *s_renderer;

/* Returns whether argument `index` is a plain ArrayBuffer, not a view.
 *
 * Why: upstream read the element buffer with an engine getter that accepts
 * only an ArrayBuffer, while jsb_arg_bytes also takes TypedArrays and
 * DataViews. Among those three, only views have a `byteOffset` (an inherited
 * getter). Fails, with the exception pending, only if a getter threw. */
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
 * Why: keeps upstream's "ArrayBuffer only" rule for drawTiles (see
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

/* drawTiles(elementsF32, count, textureIds, texWidths, texHeights,
 *           numTilesets, offsetX, offsetY)
 *
 * Why: tilemap_shim.js packs every visible tile into one Float32Array of 7
 * floats [setNumber, sx, sy, dx, dy, w, h] and draws the layer in one call.
 * The tileset arrays are indexed by setNumber and hold the GL texture id,
 * width and height. `count` and `numTilesets` are coerced with ToInt32, the
 * tileset values with ToUint32/ToInt32 and the offsets with ToNumber, as
 * upstream did. Returns undefined with no renderer, no tiles to draw or no
 * tileset, and throws upstream's TypeErrors/RangeError for bad arguments.
 * `elementsF32` must be an ArrayBuffer; its bytes are borrowed only for this
 * call, after every other conversion has run (those can execute user JS that
 * detaches or resizes it). Heap tileset storage is freed on every exit. */
static void js_tilemap_draw_tiles(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 8) {
        jsb_throw_type(ctx, "drawTiles requires 8 arguments");
        return;
    }
    if (!s_renderer) {
        jsb_ret_undefined(ctx);
        return;
    }

    /* arg 1: count */
    int32_t count;
    if (!jsb_arg_to_i32(ctx, 1, &count)) return;
    if (count <= 0) {
        jsb_ret_undefined(ctx);
        return;
    }

    /* arg 2: textureIds — Array of uint32 */
    /* arg 3: texWidths — Array of int */
    /* arg 4: texHeights — Array of int */
    /* arg 5: numTilesets */
    int32_t num_tilesets;
    if (!jsb_arg_to_i32(ctx, 5, &num_tilesets)) return;
    if (num_tilesets <= 0) {
        jsb_ret_undefined(ctx);
        return;
    }

    /* Allocate tilesets info on the stack for small counts, heap for large. */
    TilesetInfo stack_ts[32];
    TilesetInfo *tilesets = (num_tilesets <= 32)
        ? stack_ts
        : malloc(sizeof(TilesetInfo) * (size_t)num_tilesets);
    if (!tilesets) {
        jsb_throw_internal(ctx, "out of memory");
        return;
    }

    /* Read tileset arrays. */
    for (int i = 0; i < num_tilesets; i++) {
        jsb_val *tid_val = jsb_get_index(ctx, jsb_arg_val(ctx, 2),
                                         (uint32_t)i);
        jsb_val *tw_val = jsb_get_index(ctx, jsb_arg_val(ctx, 3),
                                        (uint32_t)i);
        jsb_val *th_val = jsb_get_index(ctx, jsb_arg_val(ctx, 4),
                                        (uint32_t)i);

        uint32_t gl_tex = 0;
        int32_t tw = 0, th = 0;
        bool conv_err = !tid_val || !tw_val || !th_val ||
                        !jsb_val_to_u32(ctx, tid_val, &gl_tex) ||
                        !jsb_val_to_i32(ctx, tw_val, &tw) ||
                        !jsb_val_to_i32(ctx, th_val, &th);

        if (conv_err) {
            if (tilesets != stack_ts) free(tilesets);
            return;
        }

        tilesets[i].gl_texture = gl_tex;
        tilesets[i].width = tw;
        tilesets[i].height = th;
    }

    /* arg 6: offsetX, arg 7: offsetY */
    double offset_x, offset_y;
    if (!jsb_arg_to_f64(ctx, 6, &offset_x)) {
        if (tilesets != stack_ts) free(tilesets);
        return;
    }
    if (!jsb_arg_to_f64(ctx, 7, &offset_y)) {
        if (tilesets != stack_ts) free(tilesets);
        return;
    }

    /* arg 0: elementsF32 (ArrayBuffer). Capture the backing pointer only after
       every other argument is converted: those conversions can run user JS
       (valueOf/getters/Proxy traps) that detaches or resizes this buffer. */
    size_t elem_size = 0;
    uint8_t *elem_buf = NULL;
    if (!arg_array_buffer(ctx, 0, &elem_buf, &elem_size)) {
        if (tilesets != stack_ts) free(tilesets);
        jsb_throw_type(ctx, "drawTiles: arg 0 must be an ArrayBuffer");
        return;
    }
    size_t expected = (size_t)count * 7 * sizeof(float);
    if (elem_size < expected) {
        if (tilesets != stack_ts) free(tilesets);
        jsb_throw_range(ctx, "drawTiles: buffer too small for %d elements",
                        (int)count);
        return;
    }
    const float *elements = (const float *)elem_buf;

    SpriteBatch *batch = renderer_get_batch(s_renderer);
    if (batch) {
        tilemap_draw_tiles(elements, count, tilesets, num_tilesets,
                           (float)offset_x, (float)offset_y, batch);
    }

    if (tilesets != stack_ts) free(tilesets);
    jsb_ret_undefined(ctx);
}

/* Registration */

/* Fills __native_tilemap in upstream's function-list order, with its arity as
 * the function's `length`. Returns false with a JS exception pending. */
static bool tilemap_module_init(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn_n(ctx, module, "drawTiles", js_tilemap_draw_tiles, 8);
}

/* jsb_runtime_setup callback that defines the module; `user` is unused. */
static bool tilemap_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_tilemap", tilemap_module_init);
}

/* smzuggler: upstream's function list defined drawTiles non-enumerable
 * (writable, configurable); jsb_set_fn_n assigns it, which makes it
 * enumerable. This restores the old attributes so the JS API stays
 * byte-for-byte. Run once by bind_tilemap_define, after the module exists. */
static const char tilemap_hide_fns_js[] =
    "(function (m) {\n"
    "  Object.getOwnPropertyNames(m).forEach(function (k) {\n"
    "    Object.defineProperty(m, k, { enumerable: false });\n"
    "  });\n"
    "})(__native_tilemap);\n";

/* See bind_tilemap.h: setup borrows rt and reports registration failures
 * loudly. Main thread only. */
bool bind_tilemap_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_tilemap_define: no jsb runtime\n");
        return false;
    }
    if (!jsb_runtime_setup(rt, tilemap_define_setup, NULL))
        return false;
    char err[256];
    if (!jsb_runtime_eval(rt, tilemap_hide_fns_js, "bind_tilemap.c", err,
                          sizeof err)) {
        fprintf(stderr, "bind_tilemap_define: %s\n", err);
        return false;
    }
    return true;
}
