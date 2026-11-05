/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: expose the jsb entry point for the video DOM bridge.
 * See docs/js-binding.md; all entry points are main-thread only.
 */

#ifndef RMMZ_BIND_VIDEO_H
#define RMMZ_BIND_VIDEO_H

#include "jsb/jsb.h"

/* Define the HTMLVideoElement shim's module in borrowed rt. Returns false
 * with diagnostics on setup failure; no JS handles escape registration. */
bool bind_video_define(jsb_runtime *rt);

#ifdef SMZ_JSB_ADOPT
struct JSContext;
/* Transitional main-thread wrapper (M1-T026); ctx is borrowed and opaque. */
void bind_video_register(struct JSContext *ctx);
#endif

#endif /* RMMZ_BIND_VIDEO_H */
