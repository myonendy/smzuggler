/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: define the font module through jsb.
 *
 * Exposes the native FontFace/Canvas2D services without engine-specific types.
 * Setup runs on the main thread after engine/font initialization; the engine
 * owns the runtime. See docs/js-binding.md for module and handle lifetimes.
 */

#ifndef RMMZ_BIND_FONT_H
#define RMMZ_BIND_FONT_H

#include "jsb/jsb.h"

/* Define __native_font for the FontFace/Canvas2D shims on the main thread.
 * Borrows rt without retaining it. Call after engine/font initialization;
 * false means setup failed (diagnostic printed or JS exception reported).
 * The engine owns all module handles; no graphics device is required. */
bool bind_font_define(jsb_runtime *rt);

#ifdef SMZ_JSB_ADOPT
struct JSContext;
/* Transitional entry for existing callers; borrows ctx and its adopted runtime.
 * Main thread after engine/font initialization; failures are reported by define. */
void bind_font_register(struct JSContext *ctx);
#endif

#endif /* RMMZ_BIND_FONT_H */
