/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: define the module through jsb (bind_canvas2d_define).
 *
 * Entry points of the __native_canvas2d binding, the CPU rasteriser behind
 * the Canvas2D shim (docs/js-binding.md, "bind_canvas2d"). Main thread only.
 */

#ifndef RMMZ_BIND_CANVAS2D_H
#define RMMZ_BIND_CANVAS2D_H

#include <stdbool.h>

#include "jsb/jsb.h"

/* Defines the __native_canvas2d global object with the Canvas2D functions,
 * in the global context of `rt`.
 *
 * Why: the jsb entry point every caller moves to (spec section 4.1). `rt`
 * is borrowed. Returns false, with the reason on stderr, when `rt` is NULL
 * or the module could not be defined. Main thread only; canvas2d_init must
 * have run before JS calls the functions. */
bool bind_canvas2d_define(jsb_runtime *rt);

#ifdef SMZ_JSB_ADOPT
struct JSContext;

/* TRANSITIONAL (removed by M1-T026): registers __native_canvas2d on the
 * engine's own context through its adopted jsb runtime, so callers that
 * still hold a QuickJS context need no change (DEC-M1-T014-JSB-ADOPT).
 * `ctx` is an opaque, borrowed pointer here. Main thread only. */
void bind_canvas2d_register(struct JSContext *ctx);
#endif

#endif /* RMMZ_BIND_CANVAS2D_H */
