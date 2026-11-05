/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: define the module through jsb (bind_effekseer_define).
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 *
 * Entry points of the __native_effekseer binding, the native particle-effect
 * bridge under effekseer_shim.js (docs/js-binding.md, "bind_effekseer").
 * Main thread only.
 */

#ifndef RMMZ_BIND_EFFEKSEER_H
#define RMMZ_BIND_EFFEKSEER_H

#include <stdbool.h>

#include "jsb/jsb.h"

/* Defines the __native_effekseer global object exposing Effekseer effect
 * loading, playback and drawing to JS, in the global context of `rt`.
 *
 * Why: the jsb entry point every caller moves to (spec section 4.1). `rt`
 * is borrowed. Returns false, with the reason on stderr or a printed JS
 * exception, when `rt` is NULL or the module could not be defined. Main
 * thread only. */
bool bind_effekseer_define(jsb_runtime *rt);

#endif /* RMMZ_BIND_EFFEKSEER_H */
