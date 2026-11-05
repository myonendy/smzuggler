/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: declare the jsb audio module and transitional entry.
 *
 * Native Web Audio shim bridge; see docs/js-binding.md for handle lifetimes.
 * Main thread only; the engine owns the runtime and all module handles.
 */

#ifndef RMMZ_BIND_AUDIO_H
#define RMMZ_BIND_AUDIO_H

#include "jsb/jsb.h"

/* Define __native_audio for the Web Audio shim in borrowed rt; retain no
 * handles. Main thread only; false reports setup failure with stderr context. */
bool bind_audio_define(jsb_runtime *rt);

#ifdef SMZ_JSB_ADOPT
struct JSContext;
/* Transitional entry for existing callers; borrows the engine context. */
void bind_audio_register(struct JSContext *ctx);
#endif

#endif /* RMMZ_BIND_AUDIO_H */
