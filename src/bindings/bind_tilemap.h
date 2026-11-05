/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: define the module through jsb (bind_tilemap_define).
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 *
 * Entry points of the __native_tilemap binding, which lets tilemap_shim.js
 * draw packed map layers through the renderer (docs/js-binding.md,
 * "bind_tilemap"). Main thread only.
 */

#ifndef RMMZ_BIND_TILEMAP_H
#define RMMZ_BIND_TILEMAP_H

#include <stdbool.h>

#include "jsb/jsb.h"

/* Defines the __native_tilemap global object in the global context of `rt`.
 *
 * Why: the jsb entry point every caller moves to (spec section 4.1). drawTiles
 * takes an ArrayBuffer of 7 floats per tile [setNumber, sx, sy, dx, dy, w, h],
 * per-tileset texture id/width/height arrays indexed by setNumber, and a
 * layer offset. `rt` is borrowed. Returns false, with the reason on stderr or
 * a printed JS exception, when `rt` is NULL or the module could not be
 * defined. Main thread only. */
bool bind_tilemap_define(jsb_runtime *rt);

#endif /* RMMZ_BIND_TILEMAP_H */
