/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: define the module through jsb (bind_renderer_define).
 *
 * Entry points of the __native_renderer binding, the high-level sprite-batch
 * renderer API the PIXI and tilemap shims drive (docs/js-binding.md,
 * "bind_renderer"). Main thread only.
 */

#ifndef RMMZ_BIND_RENDERER_H
#define RMMZ_BIND_RENDERER_H

#include <stdbool.h>

#include "jsb/jsb.h"

/* Defines the __native_renderer global object exposing frame lifecycle,
 * textures, render textures, quad drawing, scissor and readPixels to JS, in
 * the global context of `rt`.
 *
 * Why: the jsb entry point every caller moves to (spec section 4.1). `rt`
 * is borrowed. Returns false, with the reason on stderr or a printed JS
 * exception, when `rt` is NULL or the module could not be defined. Main
 * thread only; the renderer itself is created lazily by init(). */
bool bind_renderer_define(jsb_runtime *rt);

#ifdef SMZ_JSB_ADOPT
struct JSContext;

/* TRANSITIONAL (removed by M1-T026): registers __native_renderer on the
 * engine's own context through its adopted jsb runtime, so callers that
 * still hold a QuickJS context need no change (DEC-M1-T014-JSB-ADOPT).
 * `ctx` is an opaque, borrowed pointer here. Main thread only. */
void bind_renderer_register(struct JSContext *ctx);
#endif

#endif /* RMMZ_BIND_RENDERER_H */
