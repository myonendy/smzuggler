/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: define the module through jsb (bind_image_define).
 *
 * Entry points of the __native_image binding, which decodes images and
 * exposes the image cache to the image shims (docs/js-binding.md,
 * "bind_image"). Main thread only.
 */

#ifndef RMMZ_BIND_IMAGE_H
#define RMMZ_BIND_IMAGE_H

#include <stdbool.h>

#include "jsb/jsb.h"

/* Defines the __native_image global object exposing image loading and the
 * image cache to JS, in the global context of `rt`.
 *
 * Why: the jsb entry point every caller moves to (spec section 4.1). `rt`
 * is borrowed. Returns false, with the reason on stderr or a printed JS
 * exception, when `rt` is NULL or the module could not be defined. Main
 * thread only; image_loader_init must have run before JS calls it. */
bool bind_image_define(jsb_runtime *rt);

#ifdef SMZ_JSB_ADOPT
struct JSContext;

/* TRANSITIONAL (removed by M1-T026): registers __native_image on the
 * engine's own context through its adopted jsb runtime, so callers that
 * still hold a QuickJS context need no change (DEC-M1-T014-JSB-ADOPT).
 * `ctx` is an opaque, borrowed pointer here. Main thread only. */
void bind_image_register(struct JSContext *ctx);
#endif

#endif /* RMMZ_BIND_IMAGE_H */
