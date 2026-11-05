/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: expose the jsb entry point for the filter binding.
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 * See docs/js-binding.md; every entry point is main-thread only and no JS
 * handle outlives the call that made it.
 */

#ifndef RMMZ_BIND_FILTERS_H
#define RMMZ_BIND_FILTERS_H

#include "jsb/jsb.h"

/* Define the __native_filters global (shader compilation and rendering
 * functions) in borrowed rt. Returns false, with diagnostics printed, when
 * the runtime is missing or setup fails. */
bool bind_filters_define(jsb_runtime *rt);

/* Clean up built-in shader programs compiled during JS-level init().
 * Call from C shutdown path before filters_shutdown(). */
void bind_filters_shutdown(void);

#endif /* RMMZ_BIND_FILTERS_H */
