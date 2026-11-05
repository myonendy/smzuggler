/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-10-30: use the configurable runtime identity.
 * Modified 2026-11-04: port to the jsb seam (spec section 4.1); the
 * JS-visible API is unchanged.
 *
 * Platform bindings for browser window operations; main thread only.
 * See docs/DECISIONS.md (DEC-M0-T001) for identity ownership.
 * jsb handles are call-scoped and never stored here; see docs/js-binding.md
 * (handle and lifetime rules).
 */

#include "bindings/bind_platform.h"
#include "platform/platform.h"

#include <SDL3/SDL.h>
#include <stdio.h>

static Platform *s_platform = NULL;

/* Fullscreen */

/* setFullscreen(bool): the window side of the Fullscreen API
 * (requestFullscreen/exitFullscreen, MDN:
 * https://developer.mozilla.org/en-US/docs/Web/API/Fullscreen_API).
 * The argument is coerced with ToBoolean. Without a window it does nothing;
 * with one, a missing argument is a TypeError. */
static void js_platform_set_fullscreen(jsb_ctx *ctx)
{
    if (!s_platform) return;
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "setFullscreen requires a boolean");
        return;
    }

    bool fs = jsb_arg_truthy(ctx, 0);
    platform_set_fullscreen(s_platform, fs);
}

/* isFullscreen() -> bool, behind document.fullscreenElement checks; false
 * without a window. */
static void js_platform_is_fullscreen(jsb_ctx *ctx)
{
    if (!s_platform) {
        jsb_ret_bool(ctx, false);
        return;
    }
    jsb_ret_bool(ctx, platform_is_fullscreen(s_platform));
}

/* Display information */

/* Returns a fresh {width, height} object of int32 values (call-scoped
 * handle), or NULL with an exception pending if a property set threw. */
static jsb_val *js_platform_size_object(jsb_ctx *ctx, int w, int h)
{
    jsb_val *obj = jsb_new_object(ctx);
    if (!jsb_set_prop(ctx, obj, "width", jsb_new_i32(ctx, w))) return NULL;
    if (!jsb_set_prop(ctx, obj, "height", jsb_new_i32(ctx, h))) return NULL;
    return obj;
}

/* getDisplaySize() -> {width, height}, behind window.screen.width/height
 * (MDN: https://developer.mozilla.org/en-US/docs/Web/API/Screen);
 * 1920x1080 without a window. */
static void js_platform_get_display_size(jsb_ctx *ctx)
{
    int w = 1920, h = 1080;
    if (s_platform) platform_get_display_size(s_platform, &w, &h);

    jsb_val *obj = js_platform_size_object(ctx, w, h);
    if (obj) jsb_ret_val(ctx, obj);
}

/* getDisplayScale() -> number, behind window.devicePixelRatio (MDN:
 * https://developer.mozilla.org/en-US/docs/Web/API/Window/devicePixelRatio);
 * 1 without a window. */
static void js_platform_get_display_scale(jsb_ctx *ctx)
{
    float scale = 1.0f;
    if (s_platform) scale = platform_get_display_scale(s_platform);
    jsb_ret_f64(ctx, (double)scale);
}

/* getWindowSize() -> {width, height}, behind window.innerWidth/innerHeight;
 * 0x0 without a window. */
static void js_platform_get_window_size(jsb_ctx *ctx)
{
    int w = 0, h = 0;
    if (s_platform) platform_get_window_size(s_platform, &w, &h);

    jsb_val *obj = js_platform_size_object(ctx, w, h);
    if (obj) jsb_ret_val(ctx, obj);
}

/** Implement the document.title setter on the JS/main thread (MDN:
 * https://developer.mozilla.org/en-US/docs/Web/API/Document/title).
 * Coerce argument 0 with ToString into a temporary copy, add the runtime
 * suffix, and free the copy. Return undefined or the conversion exception.
 * Empty titles use the runtime name. Compat row: platform/window title. */
static void js_platform_set_window_title(jsb_ctx *ctx)
{
    if (!s_platform || jsb_arg_count(ctx) < 1) return;
    char *title = jsb_arg_to_str(ctx, 0, NULL);
    if (!title) return;
    char full[512];
    if (title[0] != '\0') {
        snprintf(full, sizeof(full), "%s [%s]", title, RMMZ_APP_NAME);
    } else {
        snprintf(full, sizeof(full), "%s", RMMZ_APP_NAME);
    }
    jsb_free_str(title);
    platform_set_window_title(s_platform, full);
}

/* quit() - push an SDL_EVENT_QUIT event to terminate the game loop, as
 * nw.App.quit / window.close do. Returns undefined; a failed push is
 * ignored, as before. */
static void js_platform_quit(jsb_ctx *ctx)
{
    (void)ctx;
    SDL_Event event;
    SDL_memset(&event, 0, sizeof(event));
    event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&event);
}

/* Registration */

/* One __native_platform function and its pre-port arity (`length`). */
typedef struct {
    const char *name;
    int length;
    jsb_fn fn;
} PlatformFuncDef;

static const PlatformFuncDef js_platform_funcs[] = {
    { "setFullscreen",    1, js_platform_set_fullscreen },
    { "isFullscreen",     0, js_platform_is_fullscreen },
    { "getDisplaySize",   0, js_platform_get_display_size },
    { "getDisplayScale",  0, js_platform_get_display_scale },
    { "getWindowSize",    0, js_platform_get_window_size },
    { "setWindowTitle",   1, js_platform_set_window_title },
    { "quit",             0, js_platform_quit },
};

/* jsb_define_module init: adds js_platform_funcs to `module` in table order.
 * Returns false with an exception pending if a function could not be set. */
static bool js_platform_module_init(jsb_ctx *ctx, jsb_val *module)
{
    size_t n = sizeof(js_platform_funcs) / sizeof(js_platform_funcs[0]);
    for (size_t i = 0; i < n; i++) {
        const PlatformFuncDef *def = &js_platform_funcs[i];
        if (!jsb_set_fn_n(ctx, module, def->name, def->fn, def->length))
            return false;
    }
    return true;
}

/* jsb_runtime_setup callback that defines the global __native_platform.
 * `user` is unused. */
static bool js_platform_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_platform", js_platform_module_init);
}

bool bind_platform_define(jsb_runtime *rt, Platform *platform)
{
    s_platform = platform;

    if (!jsb_runtime_setup(rt, js_platform_setup, NULL)) return false;

    /* storage_shim.js builds process.platform/arch/env/hrtime from this. */
    return bind_platform_define_process(rt);
}

#ifdef SMZ_JSB_ADOPT
void bind_platform_register(struct JSContext *ctx, Platform *platform)
{
    (void)bind_platform_define(jsb_runtime_of(ctx), platform);
}
#endif
