/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: define the module through jsb (bind_io_define).
 *
 * Entry points of the __native_io binding, the synchronous file I/O under
 * fs_shim.js's Node `fs` stand-in (docs/js-binding.md, "bind_io"). Main
 * thread only.
 */

#ifndef RMMZ_BIND_IO_H
#define RMMZ_BIND_IO_H

#include <stdbool.h>

#include "jsb/jsb.h"

/* Defines the __native_io global object exposing synchronous file I/O to JS,
 * in the global context of `rt`. All paths are resolved relative to the game
 * root, or checked by the save-path guard (io/file_io.h).
 *
 * Why: the jsb entry point every caller moves to (spec section 4.1). `rt`
 * is borrowed. Returns false, with the reason on stderr or a printed JS
 * exception, when `rt` is NULL or the module could not be defined. Main
 * thread only. */
bool bind_io_define(jsb_runtime *rt);

#ifdef SMZ_JSB_ADOPT
struct JSContext;
/* Keep the legacy caller spelling available without exposing engine internals. */
typedef struct JSContext JSContext;

/* TRANSITIONAL (removed by M1-T026): registers __native_io on the engine's
 * own context through its adopted jsb runtime, so callers that still hold a
 * QuickJS context need no change (DEC-M1-T014-JSB-ADOPT). `ctx` is an
 * opaque, borrowed pointer here. Main thread only. */
void bind_io_register(struct JSContext *ctx);
#endif

#endif /* RMMZ_BIND_IO_H */
