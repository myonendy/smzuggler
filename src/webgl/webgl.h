/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Entry points of the native WebGL2 binding under src/webgl/.
 *
 * Why: HTMLCanvasElement.getContext('webgl2') needs a native module, and the
 * Fast API test needs the list of hot calls registered with v8::CFunction.
 * See docs/webgl.md and the module README.
 *
 * Rules: main thread only; `rt` is borrowed and must be a live jsb runtime.
 */
#ifndef SMZ_WEBGL_H
#define SMZ_WEBGL_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct jsb_runtime;

/* Defines `__native_webgl` in `rt`'s context. Returns false after printing
 * the reason when `rt` is NULL or V8 setup fails. Main thread only. */
bool webgl_define(struct jsb_runtime *rt);

/* Returns how many hot calls were registered through v8::CFunction. */
size_t webgl_fast_api_count(void);

/* Returns the `index`-th registered hot-call name, or NULL past the end. */
const char *webgl_fast_api_name(size_t index);

#ifdef __cplusplus
}
#endif

#endif /* SMZ_WEBGL_H */
