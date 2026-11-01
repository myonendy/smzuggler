/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 */

#ifndef RMMZ_BIND_PLATFORM_H
#define RMMZ_BIND_PLATFORM_H

#include <quickjs.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdlib.h>
#else
#include <time.h>
extern char **environ;
#endif

typedef struct Platform Platform;

/* Register __native_platform bindings in the given JS context.
   Must be called after platform_init(). Also registers __native_process. */
void bind_platform_register(JSContext *ctx, Platform *platform);

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
   process.hrtime(). Never goes backwards; not related to wall time. */
static inline uint64_t bind_platform_monotonic_ns(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (uint64_t)(now.QuadPart / freq.QuadPart) * 1000000000ull +
           (uint64_t)(now.QuadPart % freq.QuadPart) * 1000000000ull /
               (uint64_t)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}

/* hrtime() -> [seconds, nanoseconds] of the monotonic clock. */
static inline JSValue bind_platform_js_hrtime(JSContext *ctx, JSValueConst this_val,
                                              int argc, JSValueConst *argv)
{
    (void)this_val; (void)argc; (void)argv;
    uint64_t ns = bind_platform_monotonic_ns();
    JSValue arr = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, arr, 0, JS_NewFloat64(ctx, (double)(ns / 1000000000ull)));
    JS_SetPropertyUint32(ctx, arr, 1, JS_NewInt32(ctx, (int32_t)(ns % 1000000000ull)));
    return arr;
}

/* environ() -> a fresh plain object of the host environment's NAME=value
   strings. Entries without '=' (or starting with it, as Windows' hidden
   "=C:" drive entries do) are skipped. Values are copied, never live. */
static inline JSValue bind_platform_js_environ(JSContext *ctx, JSValueConst this_val,
                                               int argc, JSValueConst *argv)
{
    (void)this_val; (void)argc; (void)argv;
#ifdef _WIN32
    char **env = _environ;
#else
    char **env = environ;
#endif
    JSValue obj = JS_NewObject(ctx);
    for (; env && *env; env++) {
        const char *eq = strchr(*env, '=');
        if (!eq || eq == *env) continue;
        JSAtom key = JS_NewAtomLen(ctx, *env, (size_t)(eq - *env));
        JS_SetProperty(ctx, obj, key, JS_NewString(ctx, eq + 1));
        JS_FreeAtom(ctx, key);
    }
    return obj;
}

/* Register the global __native_process object: platform and arch strings,
   plus environ() and hrtime(). Safe to call without platform_init(). */
static inline void bind_platform_register_process(JSContext *ctx)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "platform", JS_NewString(ctx, bind_platform_os_name()));
    JS_SetPropertyStr(ctx, obj, "arch", JS_NewString(ctx, bind_platform_arch_name()));
    JS_SetPropertyStr(ctx, obj, "engine", JS_NewString(ctx, JS_GetVersion()));
    JS_SetPropertyStr(ctx, obj, "environ",
                      JS_NewCFunction(ctx, bind_platform_js_environ, "environ", 0));
    JS_SetPropertyStr(ctx, obj, "hrtime",
                      JS_NewCFunction(ctx, bind_platform_js_hrtime, "hrtime", 0));
    JS_SetPropertyStr(ctx, global, "__native_process", obj);
    JS_FreeValue(ctx, global);
}

#endif /* RMMZ_BIND_PLATFORM_H */
