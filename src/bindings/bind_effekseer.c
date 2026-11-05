/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: ported to the jsb seam (bind_effekseer_define).
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 * Modified 2026-11-05: rely on jsb_set_fn_n for non-enumerable functions (card M1-T026b).
 *
 * The __native_effekseer binding: the native bridge under effekseer_shim.js,
 * which stands in for the Effekseer WebGL runtime RMMZ loads in NW.js. See
 * docs/js-binding.md, "bind_effekseer", and the handle-lifetime rules there
 * (spec section 15.8): every jsb_val here is valid only during its call and
 * is never stored. Main thread only.
 */

#include "bindings/bind_effekseer.h"
#include "effects/effekseer_backend.h"
#include "io/file_io.h"

#include <stdio.h>
#include <string.h>

#define PATH_BUF_SIZE 4096

/* Resolve JS argument `index` to an absolute path in `out` (`out_size`
   bytes). Returns the argument's string (caller frees with jsb_free_str) or
   NULL with a pending exception: the ToString error, or a TypeError when the
   path does not resolve under the game root. */
static char *resolve_effect_path(jsb_ctx *ctx, int index,
                                 char *out, size_t out_size)
{
    char *raw = jsb_arg_to_str(ctx, index, NULL);
    if (!raw) return NULL;

    if (!file_io_resolve_path(raw, out, out_size)) {
        jsb_throw_type(ctx, "effect path resolution failed: %s", raw);
        jsb_free_str(raw);
        return NULL;
    }
    return raw;
}

/* Read the Array in argument 0 into the 16 floats of `m`, element by element
   with Number coercion, in index order (missing entries become NaN). A
   non-Array is the TypeError "<name> expects a 16-element array". Returns
   false with the exception pending on any failure. */
static bool read_matrix_arg(jsb_ctx *ctx, const char *name, float m[16])
{
    jsb_val *arr = jsb_arg_val(ctx, 0);
    if (jsb_arg_count(ctx) < 1 || !jsb_is_array(ctx, arr)) {
        jsb_throw_type(ctx, "%s expects a 16-element array", name);
        return false;
    }

    for (int i = 0; i < 16; i++) {
        jsb_val *elem = jsb_get_index(ctx, arr, (uint32_t)i);
        double v = 0;
        if (!elem || !jsb_val_to_f64(ctx, elem, &v)) return false;
        m[i] = (float)v;
    }
    return true;
}

/* Return a uint32 handle to JS as a Number (effect instances can use the
   top bit, so this is unsigned). */
static void ret_u32(jsb_ctx *ctx, uint32_t value)
{
    jsb_ret_val(ctx, jsb_new_u32(ctx, value));
}

/* init(screenWidth, screenHeight, maxSprites) -> bool
   Why: creates the renderer the shim draws into (defaults: the 816x624 RMMZ
   screen, 8000 sprites). Arguments use ToInt32 coercion. */
static void js_effekseer_init(jsb_ctx *ctx)
{
    int32_t w = 816, h = 624, max_sprites = 8000;
    int argc = jsb_arg_count(ctx);
    if (argc >= 1 && !jsb_arg_to_i32(ctx, 0, &w)) return;
    if (argc >= 2 && !jsb_arg_to_i32(ctx, 1, &h)) return;
    if (argc >= 3 && !jsb_arg_to_i32(ctx, 2, &max_sprites)) return;
    jsb_ret_bool(ctx, effekseer_init(w, h, max_sprites));
}

/* shutdown()
   Why: frees the renderer and every loaded effect. Arguments are ignored. */
static void js_effekseer_shutdown(jsb_ctx *ctx)
{
    (void)ctx;
    effekseer_shutdown();
}

/* isInitialized() -> bool
   Why: lets the shim skip drawing until init succeeded. */
static void js_effekseer_is_initialized(jsb_ctx *ctx)
{
    jsb_ret_bool(ctx, effekseer_is_initialized());
}

/* load(path, scale) -> effectHandle
   Why: Effekseer.Context.loadEffect for a game-relative path. A path that
   cannot be converted or resolved returns 0, not a throw; a missing path is
   a TypeError and a bad scale throws. */
static void js_effekseer_load(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "load requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_effect_path(ctx, 0, resolved, sizeof(resolved));
    if (!raw) {
        /* The shim checks for a 0 handle; a throw would bypass that. */
        jsb_clear_exception(ctx);
        ret_u32(ctx, 0);
        return;
    }
    jsb_free_str(raw);

    double scale = 1.0;
    if (jsb_arg_count(ctx) >= 2 && !jsb_arg_to_f64(ctx, 1, &scale)) return;

    EffectHandle h = effekseer_load(resolved, (float)scale);
    ret_u32(ctx, h);
}

/* release(effectHandle)
   Why: frees one loaded effect; no argument is a no-op. */
static void js_effekseer_release(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    effekseer_release(handle);
}

/* isLoaded(effectHandle) -> bool
   Why: lets the shim poll whether an effect is ready; no argument is false. */
static void js_effekseer_is_loaded(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_ret_bool(ctx, false);
        return;
    }
    uint32_t handle;
    if (!jsb_arg_to_u32(ctx, 0, &handle)) return;
    jsb_ret_bool(ctx, effekseer_is_loaded(handle));
}

/* play(effectHandle, x, y, z) -> instanceHandle
   Why: Effekseer.Context.play; EFFECT_INSTANCE_INVALID (-1) without an
   effect argument or when the effect cannot play. */
static void js_effekseer_play(jsb_ctx *ctx)
{
    int argc = jsb_arg_count(ctx);
    if (argc < 1) {
        jsb_ret_i32(ctx, EFFECT_INSTANCE_INVALID);
        return;
    }

    uint32_t effect;
    if (!jsb_arg_to_u32(ctx, 0, &effect)) return;

    double x = 0, y = 0, z = 0;
    if (argc >= 2 && !jsb_arg_to_f64(ctx, 1, &x)) return;
    if (argc >= 3 && !jsb_arg_to_f64(ctx, 2, &y)) return;
    if (argc >= 4 && !jsb_arg_to_f64(ctx, 3, &z)) return;

    EffectInstanceHandle inst = effekseer_play(effect, (float)x, (float)y, (float)z);
    jsb_ret_i32(ctx, inst);
}

/* stop(instanceHandle)
   Why: EffekseerHandle.stop; no argument is a no-op. */
static void js_effekseer_stop(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    int32_t inst;
    if (!jsb_arg_to_i32(ctx, 0, &inst)) return;
    effekseer_stop(inst);
}

/* stopRoot(instanceHandle)
   Why: EffekseerHandle.stopRoot; no argument is a no-op. */
static void js_effekseer_stop_root(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    int32_t inst;
    if (!jsb_arg_to_i32(ctx, 0, &inst)) return;
    effekseer_stop_root(inst);
}

/* stopAll()
   Why: Effekseer.Context.stopAll. Arguments are ignored. */
static void js_effekseer_stop_all(jsb_ctx *ctx)
{
    (void)ctx;
    effekseer_stop_all();
}

/* setPosition(instanceHandle, x, y, z)
   Why: EffekseerHandle.setLocation; fewer than 4 arguments is a no-op. */
static void js_effekseer_set_position(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) return;
    int32_t inst;
    double x, y, z;
    if (!jsb_arg_to_i32(ctx, 0, &inst)) return;
    if (!jsb_arg_to_f64(ctx, 1, &x)) return;
    if (!jsb_arg_to_f64(ctx, 2, &y)) return;
    if (!jsb_arg_to_f64(ctx, 3, &z)) return;
    effekseer_set_position(inst, (float)x, (float)y, (float)z);
}

/* setRotation(instanceHandle, x, y, z)
   Why: EffekseerHandle.setRotation (radians); fewer than 4 is a no-op. */
static void js_effekseer_set_rotation(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) return;
    int32_t inst;
    double x, y, z;
    if (!jsb_arg_to_i32(ctx, 0, &inst)) return;
    if (!jsb_arg_to_f64(ctx, 1, &x)) return;
    if (!jsb_arg_to_f64(ctx, 2, &y)) return;
    if (!jsb_arg_to_f64(ctx, 3, &z)) return;
    effekseer_set_rotation(inst, (float)x, (float)y, (float)z);
}

/* setScale(instanceHandle, x, y, z)
   Why: EffekseerHandle.setScale; fewer than 4 arguments is a no-op. */
static void js_effekseer_set_scale(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 4) return;
    int32_t inst;
    double x, y, z;
    if (!jsb_arg_to_i32(ctx, 0, &inst)) return;
    if (!jsb_arg_to_f64(ctx, 1, &x)) return;
    if (!jsb_arg_to_f64(ctx, 2, &y)) return;
    if (!jsb_arg_to_f64(ctx, 3, &z)) return;
    effekseer_set_scale(inst, (float)x, (float)y, (float)z);
}

/* setSpeed(instanceHandle, speed)
   Why: EffekseerHandle.setSpeed; fewer than 2 arguments is a no-op. */
static void js_effekseer_set_speed(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) return;
    int32_t inst;
    double speed;
    if (!jsb_arg_to_i32(ctx, 0, &inst)) return;
    if (!jsb_arg_to_f64(ctx, 1, &speed)) return;
    effekseer_set_speed(inst, (float)speed);
}

/* exists(instanceHandle) -> bool
   Why: EffekseerHandle.exists, polled by the shim; no argument is false. */
static void js_effekseer_exists(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_ret_bool(ctx, false);
        return;
    }
    int32_t inst;
    if (!jsb_arg_to_i32(ctx, 0, &inst)) return;
    jsb_ret_bool(ctx, effekseer_exists(inst));
}

/* setProjectionMatrix(array) - expects a 16-element JS array
   Why: Effekseer.Context.setProjectionMatrix; read as read_matrix_arg. */
static void js_effekseer_set_projection_matrix(jsb_ctx *ctx)
{
    float m[16];
    if (!read_matrix_arg(ctx, "setProjectionMatrix", m)) return;
    effekseer_set_projection_matrix(m);
}

/* setCameraMatrix(array) - expects a 16-element JS array
   Why: Effekseer.Context.setCameraMatrix; read as read_matrix_arg. */
static void js_effekseer_set_camera_matrix(jsb_ctx *ctx)
{
    float m[16];
    if (!read_matrix_arg(ctx, "setCameraMatrix", m)) return;
    effekseer_set_camera_matrix(m);
}

/* update(deltaFrames)
   Why: Effekseer.Context.update; advances effects by `deltaFrames` 60 Hz
   frames (default 1). */
static void js_effekseer_update(jsb_ctx *ctx)
{
    double delta = 1.0;
    if (jsb_arg_count(ctx) >= 1 && !jsb_arg_to_f64(ctx, 0, &delta)) return;
    effekseer_update((float)delta);
}

/* beginDraw()
   Why: Effekseer.Context.beginDraw. Arguments are ignored. */
static void js_effekseer_begin_draw(jsb_ctx *ctx)
{
    (void)ctx;
    effekseer_begin_draw();
}

/* drawHandle(instanceHandle)
   Why: Effekseer.Context.drawHandle; no argument is a no-op. */
static void js_effekseer_draw_handle(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    int32_t inst;
    if (!jsb_arg_to_i32(ctx, 0, &inst)) return;
    effekseer_draw_handle(inst);
}

/* endDraw()
   Why: Effekseer.Context.endDraw. Arguments are ignored. */
static void js_effekseer_end_draw(jsb_ctx *ctx)
{
    (void)ctx;
    effekseer_end_draw();
}

/* setRestorationOfStatesFlag(flag)
   Why: Effekseer.Context.setRestorationOfStatesFlag; the flag is ToBoolean
   of the argument, default true. */
static void js_effekseer_set_restoration_flag(jsb_ctx *ctx)
{
    bool flag = true;
    if (jsb_arg_count(ctx) >= 1) flag = jsb_arg_truthy(ctx, 0);
    effekseer_set_restoration_of_states_flag(flag);
}

/* setBackground(glTexture)
   Why: Effekseer.Context.setBackground for distortion effects. The GL
   texture name (default 0) is borrowed, not owned. */
static void js_effekseer_set_background(jsb_ctx *ctx)
{
    uint32_t tex = 0;
    if (jsb_arg_count(ctx) >= 1 && !jsb_arg_to_u32(ctx, 0, &tex)) return;
    effekseer_set_background(tex);
}

/* resetBackground()
   Why: Effekseer.Context.resetBackground. Arguments are ignored. */
static void js_effekseer_reset_background(jsb_ctx *ctx)
{
    (void)ctx;
    effekseer_reset_background();
}

/* getLoadedCount() -> int
   Why: diagnostics for the shim and tests. */
static void js_effekseer_get_loaded_count(jsb_ctx *ctx)
{
    jsb_ret_i32(ctx, effekseer_get_loaded_count());
}

/* getPlayingCount() -> int
   Why: diagnostics for the shim and tests. */
static void js_effekseer_get_playing_count(jsb_ctx *ctx)
{
    jsb_ret_i32(ctx, effekseer_get_playing_count());
}

/* Registration */

/* Fill __native_effekseer in upstream's function-list order, with its
   arities as each function's `length`. Returns false with a JS exception
   pending. */
static bool effekseer_module_init(jsb_ctx *ctx, jsb_val *m)
{
    return jsb_set_fn_n(ctx, m, "init", js_effekseer_init, 3) &&
           jsb_set_fn_n(ctx, m, "shutdown", js_effekseer_shutdown, 0) &&
           jsb_set_fn_n(ctx, m, "isInitialized", js_effekseer_is_initialized, 0) &&
           jsb_set_fn_n(ctx, m, "load", js_effekseer_load, 2) &&
           jsb_set_fn_n(ctx, m, "release", js_effekseer_release, 1) &&
           jsb_set_fn_n(ctx, m, "isLoaded", js_effekseer_is_loaded, 1) &&
           jsb_set_fn_n(ctx, m, "play", js_effekseer_play, 4) &&
           jsb_set_fn_n(ctx, m, "stop", js_effekseer_stop, 1) &&
           jsb_set_fn_n(ctx, m, "stopRoot", js_effekseer_stop_root, 1) &&
           jsb_set_fn_n(ctx, m, "stopAll", js_effekseer_stop_all, 0) &&
           jsb_set_fn_n(ctx, m, "setPosition", js_effekseer_set_position, 4) &&
           jsb_set_fn_n(ctx, m, "setRotation", js_effekseer_set_rotation, 4) &&
           jsb_set_fn_n(ctx, m, "setScale", js_effekseer_set_scale, 4) &&
           jsb_set_fn_n(ctx, m, "setSpeed", js_effekseer_set_speed, 2) &&
           jsb_set_fn_n(ctx, m, "exists", js_effekseer_exists, 1) &&
           jsb_set_fn_n(ctx, m, "setProjectionMatrix", js_effekseer_set_projection_matrix, 1) &&
           jsb_set_fn_n(ctx, m, "setCameraMatrix", js_effekseer_set_camera_matrix, 1) &&
           jsb_set_fn_n(ctx, m, "update", js_effekseer_update, 1) &&
           jsb_set_fn_n(ctx, m, "beginDraw", js_effekseer_begin_draw, 0) &&
           jsb_set_fn_n(ctx, m, "drawHandle", js_effekseer_draw_handle, 1) &&
           jsb_set_fn_n(ctx, m, "endDraw", js_effekseer_end_draw, 0) &&
           jsb_set_fn_n(ctx, m, "setRestorationOfStatesFlag", js_effekseer_set_restoration_flag, 1) &&
           jsb_set_fn_n(ctx, m, "setBackground", js_effekseer_set_background, 1) &&
           jsb_set_fn_n(ctx, m, "resetBackground", js_effekseer_reset_background, 0) &&
           jsb_set_fn_n(ctx, m, "getLoadedCount", js_effekseer_get_loaded_count, 0) &&
           jsb_set_fn_n(ctx, m, "getPlayingCount", js_effekseer_get_playing_count, 0);
}

/* jsb_runtime_setup callback that defines the module; `user` is unused. */
static bool effekseer_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_effekseer", effekseer_module_init);
}

bool bind_effekseer_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_effekseer_define: no jsb runtime\n");
        return false;
    }
    return jsb_runtime_setup(rt, effekseer_define_setup, NULL);
}
