/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: expose the font API through jsb.
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 * Modified 2026-11-05: rely on jsb_set_fn_n for non-enumerable functions (card M1-T026b).
 *
 * Supplies font loading and measurement for the FontFace/Canvas2D shims.
 * Main thread only; no JS handle or borrowed buffer survives a call.
 * Both font caches copy input bytes. See docs/js-binding.md for lifetimes
 * and docs/compat-surface.md entries FontFace, FontFaceSet and
 * CanvasRenderingContext2D.prototype.measureText for the shim contracts.
 */

#include "bindings/bind_font.h"
#include "rendering/font_manager.h"
#include "rendering/canvas2d.h"
#include "io/file_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_BUF_SIZE 4096

/* Font loading */

/* FontFace loading for the font shim: family is coerced, bytes are borrowed
 * only until both caches copy them. Main thread; missing/wrong args throw.
 * Returns a Boolean. See https://drafts.csswg.org/css-font-loading/#fontface-interface
 * and docs/js-binding.md; this internal API is not a browser global. */
static void js_font_load(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "loadFont requires family and ArrayBuffer");
        return;
    }

    char *family = jsb_arg_to_str(ctx, 0, NULL);
    if (!family) return;

    size_t buf_size = 0;
    uint8_t *data = NULL;
    if (!jsb_arg_bytes(ctx, 1, &data, &buf_size)) {
        jsb_free_str(family);
        jsb_clear_exception(ctx);
        jsb_throw_type(ctx, "loadFont: second arg must be ArrayBuffer");
        return;
    }

    /* Load into both font_manager and canvas2d font caches. */
    bool ok = font_manager_load(family, data, buf_size);
    if (ok) {
        canvas2d_load_font(family, data, buf_size);
    }

    jsb_free_str(family);
    jsb_ret_bool(ctx, ok);
}

/* FontFace file loading for the shim; family/path use string coercion.
 * Main thread; owns/frees file bytes and strings. Returns false for IO failure,
 * or throws on missing args/conversion failure. Same reference as loadFont. */
static void js_font_load_file(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "loadFontFile requires family and path");
        return;
    }

    char *family = jsb_arg_to_str(ctx, 0, NULL);
    if (!family) return;

    char *path = jsb_arg_to_str(ctx, 1, NULL);
    if (!path) {
        jsb_free_str(family);
        return;
    }

    char resolved[PATH_BUF_SIZE];
    if (!file_io_resolve_path(path, resolved, sizeof(resolved))) {
        jsb_free_str(path);
        jsb_free_str(family);
        jsb_ret_bool(ctx, false);
        return;
    }
    jsb_free_str(path);

    size_t file_size = 0;
    uint8_t *data = file_io_read_binary(resolved, &file_size);
    if (!data) {
        jsb_free_str(family);
        jsb_ret_bool(ctx, false);
        return;
    }

    /* Load into both font_manager and canvas2d font caches. */
    bool ok = font_manager_load(family, data, file_size);
    if (ok) {
        canvas2d_load_font(family, data, file_size);
    }

    free(data);
    jsb_free_str(family);
    jsb_ret_bool(ctx, ok);
}

/* Font queries */

/* Implements the font shim's family availability query (FontFaceSet.check).
 * Main thread; owns/frees the coerced family string, returns bool or throws.
 * Reference: https://drafts.csswg.org/css-font-loading/#font-face-set-check */
static void js_font_has(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "hasFont requires family name");
        return;
    }

    char *family = jsb_arg_to_str(ctx, 0, NULL);
    if (!family) return;

    bool has = font_manager_has_font(family);
    jsb_free_str(family);
    jsb_ret_bool(ctx, has);
}

/* Supplies Canvas2D text metrics in CSS pixels at the coerced size.
 * Main thread; frees family, returns a call-scoped object or null if absent;
 * missing args/conversion failures throw. Internal counterpart of TextMetrics:
 * https://html.spec.whatwg.org/multipage/canvas.html#textmetrics */
static void js_font_get_metrics(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "getMetrics requires family and pixelSize");
        return;
    }

    char *family = jsb_arg_to_str(ctx, 0, NULL);
    if (!family) return;

    double size;
    if (!jsb_arg_to_f64(ctx, 1, &size)) {
        jsb_free_str(family);
        return;
    }

    FontMetrics m;
    if (!font_manager_get_metrics(family, (float)size, &m)) {
        jsb_free_str(family);
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    jsb_free_str(family);

    jsb_val *obj = jsb_new_object(ctx);
    jsb_set_prop(ctx, obj, "ascent", jsb_new_f64(ctx, m.ascent));
    jsb_set_prop(ctx, obj, "descent", jsb_new_f64(ctx, m.descent));
    jsb_set_prop(ctx, obj, "lineGap", jsb_new_f64(ctx, m.line_gap));
    jsb_set_prop(ctx, obj, "lineHeight", jsb_new_f64(ctx, m.line_height));
    jsb_ret_val(ctx, obj);
}

/* Supplies Canvas2D measureText width in CSS pixels (same reference above).
 * Family/text strings are owned and freed here; size uses number coercion.
 * Main thread; returns width, or throws on missing args/conversion failure. */
static void js_font_measure_text(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 3) {
        jsb_throw_type(ctx, "measureText requires family, pixelSize, text");
        return;
    }

    char *family = jsb_arg_to_str(ctx, 0, NULL);
    if (!family) return;

    double size;
    if (!jsb_arg_to_f64(ctx, 1, &size)) {
        jsb_free_str(family);
        return;
    }

    char *text = jsb_arg_to_str(ctx, 2, NULL);
    if (!text) {
        jsb_free_str(family);
        return;
    }

    float w = font_manager_measure_text(family, (float)size, text);
    jsb_free_str(text);
    jsb_free_str(family);
    jsb_ret_f64(ctx, (double)w);
}

/* Supplies the font shim's cache size; main thread, no retained JS state.
 * Returns an integer Number; extra arguments are ignored. Internal API. */
static void js_font_count(jsb_ctx *ctx)
{
    jsb_ret_i32(ctx, font_manager_font_count());
}

/* Supplies loaded family names to the font shim; main thread, internal API.
 * Returns a call-scoped array of copied strings; extra arguments are ignored. */
static void js_font_names(jsb_ctx *ctx)
{

    int count = font_manager_font_count();
    jsb_val *arr = jsb_new_array(ctx);
    int idx = 0;
    for (int i = 0; i < count; i++) {
        const char *name = font_manager_font_name(i);
        if (name) {
            jsb_set_index(ctx, arr, idx++, jsb_new_str(ctx, name));
        }
    }
    jsb_ret_val(ctx, arr);
}

/* Registration */

/* Preserve upstream order and arities; failure leaves the module unpublished.
 * All handles belong to this main-thread setup scope (docs/js-binding.md). */
static bool font_module_init(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn_n(ctx, module, "loadFont", js_font_load, 2) &&
           jsb_set_fn_n(ctx, module, "loadFontFile", js_font_load_file, 2) &&
           jsb_set_fn_n(ctx, module, "hasFont", js_font_has, 1) &&
           jsb_set_fn_n(ctx, module, "getMetrics", js_font_get_metrics, 2) &&
           jsb_set_fn_n(ctx, module, "measureText", js_font_measure_text, 3) &&
           jsb_set_fn_n(ctx, module, "fontCount", js_font_count, 0) &&
           jsb_set_fn_n(ctx, module, "fontNames", js_font_names, 0);
}

/* Define the module in a fresh setup scope; no user data is retained. */
static bool font_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_font", font_module_init);
}

/* smzuggler: keep upstream's ArrayBuffer-only input. jsb's byte reader also
 * accepts views. Capture the intrinsic brand getter, and coerce family
 * first, just as the native binding did. Method syntax preserves the native
 * function's lack of a constructor/prototype property; jsb_set_fn_n already
 * made its property non-enumerable. */
static const char font_surface_js[] =
    "(function(m){"
    "var nativeLoad=m.loadFont;"
    "var byteLength=Object.getOwnPropertyDescriptor(ArrayBuffer.prototype,'byteLength').get;"
    "m.loadFont={loadFont(family,buffer){"
    "if(arguments.length<2)return nativeLoad.apply(this,arguments);"
    "family=`${family}`;"
    "try{byteLength.call(buffer)}catch(e){"
    "throw new TypeError('loadFont: second arg must be ArrayBuffer')}"
    "return nativeLoad(family,buffer)}}.loadFont;"
    "})(__native_font);";

/* See bind_font.h: setup borrows rt and reports registration failures loudly. */
bool bind_font_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_font_define: no jsb runtime\n");
        return false;
    }
    if (!jsb_runtime_setup(rt, font_define_setup, NULL))
        return false;
    char error[256];
    if (!jsb_runtime_eval(rt, font_surface_js, "bind_font.c", error, sizeof error)) {
        fprintf(stderr, "bind_font_define: %s\n", error);
        return false;
    }
    return true;
}
