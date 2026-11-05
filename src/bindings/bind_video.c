/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: port the HTMLVideoElement bridge to jsb.
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 *
 * Main-thread native video bridge for the DOM shim; see docs/js-binding.md.
 * jsb handles are call-scoped and never retained (spec section 15.8).
 * Missing required arguments throw a TypeError. The pre-port binding returned
 * QuickJS's uninitialized exception sentinel, which V8 cannot represent; the
 * TypeError is the nearest safe behaviour and is pinned by the parity test.
 */

#include "bindings/bind_video.h"
#include "video/video_player.h"

#include <stdio.h>

/* Coerce a borrowed JS handle number for the HTMLVideoElement bridge.
 * Main thread; conversion errors propagate and no handles survive the call. */
static bool handle_arg(jsb_ctx *ctx, VideoHandle *out)
{
    int32_t h = 0;
    if (!jsb_arg_to_i32(ctx, 0, &h)) return false;
    *out = (VideoHandle)h;
    return true;
}

/* open(path) -> handle (0 on failure) */
/* Open a coerced path; return a numeric handle or zero on failure.
 * Backs HTMLVideoElement (https://html.spec.whatwg.org/multipage/media.html),
 * through dom_shim.js (`document.createElement` in docs/compat-surface.md).
 * Main thread only; borrowed
 * arguments and call-scoped results; conversion errors propagate unchanged
 * and a missing required argument throws a TypeError. */
static void js_video_open(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "open requires a path");
        return;
    }
    char *path = jsb_arg_to_str(ctx, 0, NULL);
    if (!path) return;
    VideoHandle h = video_open(path);
    jsb_free_str(path);
    jsb_ret_i32(ctx, h);
}

/* Release the native movie owned by a numeric handle; return undefined.
 * Backs HTMLVideoElement (https://html.spec.whatwg.org/multipage/media.html),
 * through dom_shim.js (`document.createElement` in docs/compat-surface.md).
 * Main thread only; borrowed
 * arguments and call-scoped results; conversion errors propagate unchanged
 * and a missing required argument throws a TypeError. */
static void js_video_close(jsb_ctx *ctx)
{
    VideoHandle h;
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "close requires a handle");
        return;
    }
    if (!handle_arg(ctx, &h)) return;
    video_close(h);
    jsb_ret_undefined(ctx);
}

/* Start playback without requiring an audio device; return undefined.
 * Backs HTMLVideoElement (https://html.spec.whatwg.org/multipage/media.html),
 * through dom_shim.js (`document.createElement` in docs/compat-surface.md).
 * Main thread only; borrowed
 * arguments and call-scoped results; conversion errors propagate unchanged
 * and a missing required argument throws a TypeError. */
static void js_video_play(jsb_ctx *ctx)
{
    VideoHandle h;
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "play requires a handle");
        return;
    }
    if (!handle_arg(ctx, &h)) return;
    video_play(h);
    jsb_ret_undefined(ctx);
}

/* Pause the handled movie; return undefined.
 * Backs HTMLVideoElement (https://html.spec.whatwg.org/multipage/media.html),
 * through dom_shim.js (`document.createElement` in docs/compat-surface.md).
 * Main thread only; borrowed
 * arguments and call-scoped results; conversion errors propagate unchanged
 * and a missing required argument throws a TypeError. */
static void js_video_pause(jsb_ctx *ctx)
{
    VideoHandle h;
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "pause requires a handle");
        return;
    }
    if (!handle_arg(ctx, &h)) return;
    video_pause(h);
    jsb_ret_undefined(ctx);
}

/* setVolume(handle, 0..1) */
/* Coerce volume (0..1, clamped natively) after the handle; return undefined.
 * Backs HTMLVideoElement (https://html.spec.whatwg.org/multipage/media.html),
 * through dom_shim.js (`document.createElement` in docs/compat-surface.md).
 * Main thread only; borrowed
 * arguments and call-scoped results; conversion errors propagate unchanged
 * and a missing required argument throws a TypeError. */
static void js_video_set_volume(jsb_ctx *ctx)
{
    VideoHandle h;
    double vol = 1.0;
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "setVolume requires a handle and a volume");
        return;
    }
    if (!handle_arg(ctx, &h) || !jsb_arg_to_f64(ctx, 1, &vol)) return;
    video_set_volume(h, (float)vol);
    jsb_ret_undefined(ctx);
}

/* Use JS truthiness for looping after coercing the handle; return undefined.
 * Backs HTMLVideoElement (https://html.spec.whatwg.org/multipage/media.html),
 * through dom_shim.js (`document.createElement` in docs/compat-surface.md).
 * Main thread only; borrowed
 * arguments and call-scoped results; conversion errors propagate unchanged
 * and a missing required argument throws a TypeError. */
static void js_video_set_loop(jsb_ctx *ctx)
{
    VideoHandle h;
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "setLoop requires a handle and a flag");
        return;
    }
    if (!handle_arg(ctx, &h)) return;
    video_set_loop(h, jsb_arg_truthy(ctx, 1));
    jsb_ret_undefined(ctx);
}

/* Use JS truthiness for visibility after coercing the handle; return undefined.
 * Backs HTMLVideoElement (https://html.spec.whatwg.org/multipage/media.html),
 * through dom_shim.js (`document.createElement` in docs/compat-surface.md).
 * Main thread only; borrowed
 * arguments and call-scoped results; conversion errors propagate unchanged
 * and a missing required argument throws a TypeError. */
static void js_video_set_visible(jsb_ctx *ctx)
{
    VideoHandle h;
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "setVisible requires a handle and a flag");
        return;
    }
    if (!handle_arg(ctx, &h)) return;
    video_set_visible(h, jsb_arg_truthy(ctx, 1));
    jsb_ret_undefined(ctx);
}

/* getState(handle) -> {playing, paused, ended, time, duration, width, height} | null */
/* Return a fresh state object (times in seconds), or null for an invalid handle.
 * Backs HTMLVideoElement (https://html.spec.whatwg.org/multipage/media.html),
 * through dom_shim.js (`document.createElement` in docs/compat-surface.md).
 * Main thread only; borrowed
 * arguments and call-scoped results; conversion errors propagate unchanged
 * and a missing required argument throws a TypeError. */
static void js_video_get_state(jsb_ctx *ctx)
{
    VideoHandle h;
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "getState requires a handle");
        return;
    }
    if (!handle_arg(ctx, &h)) return;
    VideoState st;
    if (!video_get_state(h, &st)) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }
    jsb_val *obj = jsb_new_object(ctx);
    jsb_set_prop(ctx, obj, "playing",  jsb_new_bool(ctx, st.playing));
    jsb_set_prop(ctx, obj, "paused",   jsb_new_bool(ctx, st.paused));
    jsb_set_prop(ctx, obj, "ended",    jsb_new_bool(ctx, st.ended));
    jsb_set_prop(ctx, obj, "time",     jsb_new_f64(ctx, st.time));
    jsb_set_prop(ctx, obj, "duration", jsb_new_f64(ctx, st.duration));
    jsb_set_prop(ctx, obj, "width",    jsb_new_i32(ctx, st.width));
    jsb_set_prop(ctx, obj, "height",   jsb_new_i32(ctx, st.height));
    jsb_ret_val(ctx, obj);
}

/* Keep upstream function order and arities; the runtime owns all functions. */
static bool video_module_init(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn_n(ctx, module, "open", js_video_open, 1) &&
           jsb_set_fn_n(ctx, module, "close", js_video_close, 1) &&
           jsb_set_fn_n(ctx, module, "play", js_video_play, 1) &&
           jsb_set_fn_n(ctx, module, "pause", js_video_pause, 1) &&
           jsb_set_fn_n(ctx, module, "setVolume", js_video_set_volume, 2) &&
           jsb_set_fn_n(ctx, module, "setLoop", js_video_set_loop, 2) &&
           jsb_set_fn_n(ctx, module, "setVisible", js_video_set_visible, 2) &&
           jsb_set_fn_n(ctx, module, "getState", js_video_get_state, 1);
}

/* Define the module inside setup's handle scope; user is unused. */
static bool video_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_video", video_module_init);
}

/* smzuggler: restore upstream's non-enumerable function properties.
 * Missing arguments formerly returned an uninitialized exception sentinel;
 * jsb cannot represent it. The parity test exposes this unresolved seam gap. */
static const char video_surface_js[] =
    "(function(m){Object.getOwnPropertyNames(m).forEach(function(k){"
    "Object.defineProperty(m,k,{enumerable:false})});})(__native_video);";

/* See bind_video.h: borrow rt and report definition errors on the main thread. */
bool bind_video_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_video_define: no jsb runtime\n");
        return false;
    }
    if (!jsb_runtime_setup(rt, video_define_setup, NULL)) return false;
    char error[256];
    if (!jsb_runtime_eval(rt, video_surface_js, "bind_video.c", error, sizeof error)) {
        fprintf(stderr, "bind_video_define: %s\n", error);
        return false;
    }
    return true;
}
