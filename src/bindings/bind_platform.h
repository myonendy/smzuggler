/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: define __native_platform and __native_process through
 * the jsb seam (spec section 4.1, docs/js-binding.md) instead of QuickJS.
 *
 * Declares the platform bindings and holds the window-less __native_process
 * helpers inline, so tests can define them without linking SDL. jsb values
 * here are call-scoped handles; none is kept past the call that made it
 * (docs/js-binding.md, handle and lifetime rules). Main thread only.
 */

#ifndef RMMZ_BIND_PLATFORM_H
#define RMMZ_BIND_PLATFORM_H

#include "jsb/jsb.h"
#ifdef SMZ_JSB_ADOPT
#include "jsb/jsb_adopt.h"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <time.h>
extern char **environ;
#endif

typedef struct Platform Platform;

/* Define the __native_platform module (and __native_process) in `rt`.
   `platform` (borrowed, kept until the next call; NULL means no window, and
   every function then answers with its no-device default) must outlive JS
   use of the module. Call after platform_init(), on the main thread,
   outside any JS call. Returns false, after jsb has printed the pending
   exception, if a module could not be defined. */
bool bind_platform_define(jsb_runtime *rt, Platform *platform);

#ifdef SMZ_JSB_ADOPT
/* Transitional wrapper for callers holding the engine's QuickJS context
   (an opaque pointer here): bind_platform_define(jsb_runtime_of(ctx), ...).
   Removed by M1-T026 (DEC-M1-T014-JSB-ADOPT). */
void bind_platform_register(struct JSContext *ctx, Platform *platform);
#endif

/* Host facts behind storage_shim.js's process object (process.platform,
   process.arch, process.env, process.hrtime). They live here, inline and
   free of SDL, so that window-less contexts such as the storage tests can
   register them without a Platform. */

/* Node's process.platform spelling for the compile target. */
static inline const char *bind_platform_os_name(void)
{
#if defined(__ANDROID__)
    return "android";
#elif defined(_WIN32)
    return "win32";
#elif defined(__APPLE__)
    return "darwin";
#elif defined(__FreeBSD__)
    return "freebsd";
#elif defined(__OpenBSD__)
    return "openbsd";
#else
    return "linux";
#endif
}

/* Node's process.arch spelling for the compile target. */
static inline const char *bind_platform_arch_name(void)
{
#if defined(__x86_64__) || defined(_M_X64)
    return "x64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__i386__) || defined(_M_IX86)
    return "ia32";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#elif defined(__riscv) && __riscv_xlen == 64
    return "riscv64";
#else
    return "unknown";
#endif
}

/* Monotonic clock in nanoseconds from an arbitrary origin, like Node's
   process.hrtime(). Never goes backwards; not related to wall time.
   Returns 0 when the host clock call fails (or Windows' frequency is 0). */
static inline uint64_t bind_platform_monotonic_ns(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq, now;
    if (!QueryPerformanceFrequency(&freq) || freq.QuadPart == 0) return 0;
    if (!QueryPerformanceCounter(&now)) return 0;
    return (uint64_t)(now.QuadPart / freq.QuadPart) * 1000000000ull +
           (uint64_t)(now.QuadPart % freq.QuadPart) * 1000000000ull /
               (uint64_t)freq.QuadPart;
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}

/* hrtime() -> [seconds, nanoseconds] of the monotonic clock as a fresh
   Array; behind Node's process.hrtime() in storage_shim.js (Node docs:
   https://nodejs.org/api/process.html#processhrtimetime), without its
   optional `time` argument. Main/JS thread only. */
static inline void bind_platform_js_hrtime(jsb_ctx *ctx)
{
    uint64_t ns = bind_platform_monotonic_ns();
    jsb_val *arr = jsb_new_array(ctx);
    if (!jsb_set_index(ctx, arr, 0, jsb_new_f64(ctx, (double)(ns / 1000000000ull))))
        return;
    if (!jsb_set_index(ctx, arr, 1, jsb_new_i32(ctx, (int32_t)(ns % 1000000000ull))))
        return;
    jsb_ret_val(ctx, arr);
}

/* Sets obj[name] = value (a copied string), where `name` is the first
   `name_len` bytes of an environ entry and so not NUL-terminated there.
   The name is copied into a stack buffer, or heap memory when long; no
   pointer is kept. Returns false if memory ran out (InternalError
   "out of memory" pending) or the assignment threw. */
static inline bool bind_platform_set_env_entry(jsb_ctx *ctx, jsb_val *obj,
                                               const char *name, size_t name_len,
                                               const char *value)
{
    char small[256];
    char *key = name_len < sizeof(small) ? small : (char *)malloc(name_len + 1);
    if (!key) {
        jsb_throw_internal(ctx, "out of memory");
        return false;
    }
    memcpy(key, name, name_len);
    key[name_len] = '\0';
    bool ok = jsb_set_prop(ctx, obj, key, jsb_new_str(ctx, value));
    if (key != small) free(key);
    return ok;
}

/* environ() -> a fresh plain object of the host environment's NAME=value
   strings, behind Node's process.env in storage_shim.js (Node docs:
   https://nodejs.org/api/process.html#processenv). Entries without '='
   (or starting with it, as Windows' hidden "=C:" drive entries do) are
   skipped. Values are copied, never live. Main/JS thread only. */
static inline void bind_platform_js_environ(jsb_ctx *ctx)
{
#ifdef _WIN32
    char **env = _environ;
#else
    char **env = environ;
#endif
    jsb_val *obj = jsb_new_object(ctx);
    for (; env && *env; env++) {
        const char *eq = strchr(*env, '=');
        if (!eq || eq == *env) continue;
        if (!bind_platform_set_env_entry(ctx, obj, *env, (size_t)(eq - *env), eq + 1))
            return;
    }
    jsb_ret_val(ctx, obj);
}

/* Fills the __native_process module: platform, arch and engine strings
   (engine is the running engine's version, jsb_engine_version()), then
   environ() and hrtime() with length 0, in that property order. Returns
   false with a JS exception pending if a property could not be set. */
static inline bool bind_platform_process_init(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_prop(ctx, module, "platform", jsb_new_str(ctx, bind_platform_os_name())) &&
           jsb_set_prop(ctx, module, "arch", jsb_new_str(ctx, bind_platform_arch_name())) &&
           jsb_set_prop(ctx, module, "engine", jsb_new_str(ctx, jsb_engine_version())) &&
           jsb_set_fn_n(ctx, module, "environ", bind_platform_js_environ, 0) &&
           jsb_set_fn_n(ctx, module, "hrtime", bind_platform_js_hrtime, 0);
}

/* jsb_runtime_setup callback that defines the global __native_process.
   `user` is unused. Returns false if the module could not be defined. */
static inline bool bind_platform_process_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_process", bind_platform_process_init);
}

/* Define the global __native_process object in `rt` (borrowed, not kept).
   Safe to call without platform_init(). Main thread only, outside any JS
   call. Returns false, after jsb has printed the pending exception, if the
   module could not be defined. */
static inline bool bind_platform_define_process(jsb_runtime *rt)
{
    return jsb_runtime_setup(rt, bind_platform_process_setup, NULL);
}

#ifdef SMZ_JSB_ADOPT
/* Transitional wrapper for callers that still hold the engine's QuickJS
   context (tests/test_storage.c, tests/smz/test_process_shim.c); `ctx` is
   an opaque pointer here. Removed by M1-T026 (DEC-M1-T014-JSB-ADOPT). */
static inline void bind_platform_register_process(struct JSContext *ctx)
{
    (void)bind_platform_define_process(jsb_runtime_of(ctx));
}
#endif

#endif /* RMMZ_BIND_PLATFORM_H */
