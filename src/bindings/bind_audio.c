/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: port the native audio bridge to jsb.
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 *
 * Implements the native mixer bridge for src/shims/webaudio_shim.js, backing
 * the AudioContext, AudioBufferSourceNode and AudioParam compat-surface rows.
 * Reference: https://webaudio.github.io/web-audio-api/
 * See docs/js-binding.md for seam and lifetime rules. Main thread only:
 * handles and borrowed buffer bytes never outlive a native call; SoLoud
 * copies audio bytes before return. No callbacks or persistent JS handles.
 * Bus indices outside 0..3 throw RangeError; other native errors return
 * invalid IDs or defaults. Missing arguments retain the shim defaults.
 */

#include "bindings/bind_audio.h"
#include "audio/audio_engine.h"
#include "io/file_io.h"

#include <stdio.h>
#include <string.h>

#define PATH_BUF_SIZE 4096

/* Coerce path argument index and resolve inside the game root, main thread.
 * Returns an owned string (caller frees) or NULL with conversion/TypeError;
 * out is a caller-owned path buffer of out_size bytes. No pointer is retained. */
static char *resolve_audio_path(jsb_ctx *ctx, int index,
                                char *out, size_t out_size)
{
    char *raw = jsb_arg_to_str(ctx, index, NULL);
    if (!raw) return NULL;

    if (!file_io_resolve_path(raw, out, out_size)) {
        jsb_throw_type(ctx, "audio path resolution failed: %s", raw);
        jsb_free_str(raw);
        return NULL;
    }
    return raw;
}

/* Start the shim mixer; no arguments, returns success as a Boolean.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_init(jsb_ctx *ctx)
{
    jsb_ret_bool(ctx, audio_engine_init());
}

/* Release mixer resources for AudioContext.close; returns undefined.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_shutdown(jsb_ctx *ctx)
{
    (void)ctx;
    audio_engine_shutdown();
    return;
}

/* Report whether AudioContext has a mixer; returns a Boolean.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_is_initialized(jsb_ctx *ctx)
{
    jsb_ret_bool(ctx, audio_engine_is_initialized());
}

/* Decode an AudioBuffer from a coerced path; returns a source ID (0 on failure).
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_load(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "load requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_audio_path(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    AudioSourceHandle h = audio_load(resolved);
    jsb_ret_val(ctx, jsb_new_u32(ctx, h));
}

/* Open streamed audio from a coerced path; returns a source ID (0 on failure).
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_load_stream(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "loadStream requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_audio_path(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    AudioSourceHandle h = audio_load_stream(resolved);
    jsb_ret_val(ctx, jsb_new_u32(ctx, h));
}

/* Decode an AudioBuffer from borrowed bytes; returns a source ID (0 on failure).
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_load_from_memory(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "loadFromMemory requires an ArrayBuffer");
        return;
    }

    size_t size = 0;
    uint8_t *data;
    if (!jsb_arg_bytes(ctx, 0, &data, &size)) {
        jsb_clear_exception(ctx);
        jsb_throw_type(ctx, "argument must be an ArrayBuffer");
        return;
    }

    if (size > UINT32_MAX) {
        jsb_throw_range(ctx, "audio buffer too large");
        return;
    }
    AudioSourceHandle h = audio_load_from_memory(data, (uint32_t)size);
    jsb_ret_val(ctx, jsb_new_u32(ctx, h));
}

/* Release a source ID for AudioBuffer disposal; missing ID is a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_free_source(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    uint32_t h;
    if (!jsb_arg_to_u32(ctx, 0, &h)) return;
    audio_source_free(h);
    return;
}

/* Start AudioBufferSourceNode: source ID, bus 0..3, gain, rate, pan, loop; returns voice ID.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_play(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 6) {
        jsb_throw_type(ctx, "play requires 6 arguments");
        return;
    }

    uint32_t source;
    int32_t bus;
    double volume, pitch, pan;
    bool loop;

    if (!jsb_arg_to_u32(ctx, 0, &source)) return;
    if (!jsb_arg_to_i32(ctx, 1, &bus)) return;
    if (bus < 0 || bus >= AUDIO_BUS_COUNT) {
        jsb_throw_range(ctx, "invalid audio bus: %d", bus);
        return;
    }
    if (!jsb_arg_to_f64(ctx, 2, &volume)) return;
    if (!jsb_arg_to_f64(ctx, 3, &pitch)) return;
    if (!jsb_arg_to_f64(ctx, 4, &pan)) return;
    loop = jsb_arg_truthy(ctx, 5);

    AudioVoice voice = audio_play(source, (AudioBus)bus, (float)volume,
                                  (float)pitch, (float)pan, loop != 0);
    jsb_ret_val(ctx, jsb_new_u32(ctx, voice));
}

/* Stop an AudioBufferSourceNode by voice ID; missing ID is a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_stop(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    uint32_t v;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    audio_stop(v);
    return;
}

/* Stop voices on bus 0..3; missing bus is a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_stop_bus(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    int32_t bus;
    if (!jsb_arg_to_i32(ctx, 0, &bus)) return;
    if (bus < 0 || bus >= AUDIO_BUS_COUNT) {
        jsb_throw_range(ctx, "invalid audio bus: %d", bus);
        return;
    }
    audio_stop_bus((AudioBus)bus);
    return;
}

/* Stop all mixer voices for AudioContext teardown; returns undefined.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_stop_all(jsb_ctx *ctx)
{
    (void)ctx;
    audio_stop_all();
    return;
}

/* Suspend a voice ID for the shim; missing ID is a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_pause(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    uint32_t v;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    audio_pause(v);
    return;
}

/* Resume a suspended voice ID for the shim; missing ID is a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_resume(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    uint32_t v;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    audio_resume(v);
    return;
}

/* Suspend bus 0..3 for the shim; missing bus is a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_pause_bus(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    int32_t bus;
    if (!jsb_arg_to_i32(ctx, 0, &bus)) return;
    if (bus < 0 || bus >= AUDIO_BUS_COUNT) {
        jsb_throw_range(ctx, "invalid audio bus: %d", bus);
        return;
    }
    audio_pause_bus((AudioBus)bus);
    return;
}

/* Resume bus 0..3 for the shim; missing bus is a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_resume_bus(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) return;
    int32_t bus;
    if (!jsb_arg_to_i32(ctx, 0, &bus)) return;
    if (bus < 0 || bus >= AUDIO_BUS_COUNT) {
        jsb_throw_range(ctx, "invalid audio bus: %d", bus);
        return;
    }
    audio_resume_bus((AudioBus)bus);
    return;
}

/* Set AudioParam gain by voice ID and linear gain; missing args are a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_set_volume(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) return;
    uint32_t v;
    double vol;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    if (!jsb_arg_to_f64(ctx, 1, &vol)) return;
    audio_set_volume(v, (float)vol);
    return;
}

/* Set playbackRate by voice ID and rate multiplier; missing args are a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_set_pitch(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) return;
    uint32_t v;
    double p;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    if (!jsb_arg_to_f64(ctx, 1, &p)) return;
    audio_set_pitch(v, (float)p);
    return;
}

/* Set StereoPannerNode pan by voice ID and pan; missing args are a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_set_pan(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) return;
    uint32_t v;
    double pan;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    if (!jsb_arg_to_f64(ctx, 1, &pan)) return;
    audio_set_pan(v, (float)pan);
    return;
}

/* Set bus 0..3 linear gain; missing args are a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_set_bus_volume(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) return;
    int32_t bus;
    double vol;
    if (!jsb_arg_to_i32(ctx, 0, &bus)) return;
    if (bus < 0 || bus >= AUDIO_BUS_COUNT) {
        jsb_throw_range(ctx, "invalid audio bus: %d", bus);
        return;
    }
    if (!jsb_arg_to_f64(ctx, 1, &vol)) return;
    audio_set_bus_volume((AudioBus)bus, (float)vol);
    return;
}

/* Ramp a voice ID to linear gain over seconds; missing args are a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_fade_volume(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 3) return;
    uint32_t v;
    double target, dur;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    if (!jsb_arg_to_f64(ctx, 1, &target)) return;
    if (!jsb_arg_to_f64(ctx, 2, &dur)) return;
    audio_fade_volume(v, (float)target, (float)dur);
    return;
}

/* Ramp bus 0..3 to linear gain over seconds; missing args are a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_fade_bus_volume(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 3) return;
    int32_t bus;
    double target, dur;
    if (!jsb_arg_to_i32(ctx, 0, &bus)) return;
    if (bus < 0 || bus >= AUDIO_BUS_COUNT) {
        jsb_throw_range(ctx, "invalid audio bus: %d", bus);
        return;
    }
    if (!jsb_arg_to_f64(ctx, 1, &target)) return;
    if (!jsb_arg_to_f64(ctx, 2, &dur)) return;
    audio_fade_bus_volume((AudioBus)bus, (float)target, (float)dur);
    return;
}

/* Query playback by voice ID; missing ID returns false.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_is_playing(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_ret_bool(ctx, false);
        return;
    }
    uint32_t v;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    jsb_ret_bool(ctx, audio_is_playing(v));
}

/* Query suspension by voice ID; missing ID returns false.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_is_paused(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_ret_bool(ctx, false);
        return;
    }
    uint32_t v;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    jsb_ret_bool(ctx, audio_is_paused(v));
}

/* Read voice ID linear gain; missing ID returns zero.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_get_volume(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_ret_f64(ctx, 0.0);
        return;
    }
    uint32_t v;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    jsb_ret_f64(ctx, audio_get_volume(v));
}

/* Read voice ID playback position in seconds; missing ID returns zero.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_get_position(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_ret_f64(ctx, 0.0);
        return;
    }
    uint32_t v;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    jsb_ret_f64(ctx, audio_get_position(v));
}

/* Seek voice ID to a position in seconds; missing args are a no-op.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_seek(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) return;
    uint32_t v;
    double pos;
    if (!jsb_arg_to_u32(ctx, 0, &v)) return;
    if (!jsb_arg_to_f64(ctx, 1, &pos)) return;
    audio_seek(v, pos);
    return;
}

/* Read AudioBuffer source ID duration in seconds; missing ID returns zero.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_get_duration(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_ret_f64(ctx, 0.0);
        return;
    }
    uint32_t h;
    if (!jsb_arg_to_u32(ctx, 0, &h)) return;
    jsb_ret_f64(ctx, audio_get_duration(h));
}

/* Report mixer voice count as an integer Number; ignores extra args.
 * Main thread; retains no JS handles. Coercion exceptions propagate.
 * See the file contract for the Web Audio shim mapping. */
static void js_audio_get_active_voice_count(jsb_ctx *ctx)
{
    jsb_ret_i32(ctx, audio_get_active_voice_count());
}

/* Registration */

/* Static native entry points keep upstream ordering and JS arities. */
static const struct {
    const char *name;
    int length;
    jsb_fn fn;
} js_audio_funcs[] = {
    {"init", 0, js_audio_init},
    {"shutdown", 0, js_audio_shutdown},
    {"isInitialized", 0, js_audio_is_initialized},
    {"load", 1, js_audio_load},
    {"loadStream", 1, js_audio_load_stream},
    {"loadFromMemory", 1, js_audio_load_from_memory},
    {"freeSource", 1, js_audio_free_source},
    {"play", 6, js_audio_play},
    {"stop", 1, js_audio_stop},
    {"stopBus", 1, js_audio_stop_bus},
    {"stopAll", 0, js_audio_stop_all},
    {"pause", 1, js_audio_pause},
    {"resume", 1, js_audio_resume},
    {"pauseBus", 1, js_audio_pause_bus},
    {"resumeBus", 1, js_audio_resume_bus},
    {"setVolume", 2, js_audio_set_volume},
    {"setPitch", 2, js_audio_set_pitch},
    {"setPan", 2, js_audio_set_pan},
    {"setBusVolume", 2, js_audio_set_bus_volume},
    {"fadeVolume", 3, js_audio_fade_volume},
    {"fadeBusVolume", 3, js_audio_fade_bus_volume},
    {"isPlaying", 1, js_audio_is_playing},
    {"isPaused", 1, js_audio_is_paused},
    {"getVolume", 1, js_audio_get_volume},
    {"getPosition", 1, js_audio_get_position},
    {"seek", 2, js_audio_seek},
    {"getDuration", 1, js_audio_get_duration},
    {"getActiveVoiceCount", 0, js_audio_get_active_voice_count},
};

/* Define the Web Audio bridge in a call-scoped module; false propagates
 * property-definition failure. Constants retain their enumerable shape. */
static bool audio_module_init(jsb_ctx *ctx, jsb_val *module)
{
    for (size_t i = 0; i < sizeof(js_audio_funcs) / sizeof(js_audio_funcs[0]); i++) {
        if (!jsb_set_fn_n(ctx, module, js_audio_funcs[i].name,
                          js_audio_funcs[i].fn, js_audio_funcs[i].length))
            return false;
    }
    return jsb_set_prop(ctx, module, "BUS_BGM", jsb_new_i32(ctx, AUDIO_BUS_BGM)) &&
           jsb_set_prop(ctx, module, "BUS_BGS", jsb_new_i32(ctx, AUDIO_BUS_BGS)) &&
           jsb_set_prop(ctx, module, "BUS_ME", jsb_new_i32(ctx, AUDIO_BUS_ME)) &&
           jsb_set_prop(ctx, module, "BUS_SE", jsb_new_i32(ctx, AUDIO_BUS_SE));
}

/* Enter setup's handle scope; rt owns the module, user is unused. */
static bool audio_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_audio", audio_module_init);
}

/* smzuggler: preserve the ArrayBuffer-only brand check and non-enumerable
 * native functions, as in bind_font. Capture the intrinsic before game JS;
 * no JS executes while C borrows buffer bytes. Method syntax keeps length,
 * name and non-constructibility of the original native entry point. */
static const char audio_surface_js[] =
    "(function(m){"
    "var nativeLoad=m.loadFromMemory;"
    "var byteLength=Object.getOwnPropertyDescriptor(ArrayBuffer.prototype,'byteLength').get;"
    "m.loadFromMemory={loadFromMemory(buffer){"
    "if(arguments.length<1)return nativeLoad.apply(this,arguments);"
    "try{byteLength.call(buffer)}catch(e){"
    "throw new TypeError('argument must be an ArrayBuffer')}"
    "return nativeLoad(buffer)}}.loadFromMemory;"
    "Object.getOwnPropertyNames(m).forEach(function(k){"
    "if(typeof m[k]==='function')Object.defineProperty(m,k,{enumerable:false})});"
    "})(__native_audio);";

/* See bind_audio.h: borrow rt, report registration errors on the main thread. */
bool bind_audio_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_audio_define: no jsb runtime\n");
        return false;
    }
    if (!jsb_runtime_setup(rt, audio_define_setup, NULL))
        return false;
    char error[256];
    if (!jsb_runtime_eval(rt, audio_surface_js, "bind_audio.c", error, sizeof error)) {
        fprintf(stderr, "bind_audio_define: %s\n", error);
        return false;
    }
    return true;
}
