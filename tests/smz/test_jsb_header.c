/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Compile-only check that src/jsb/jsb.h is self-contained, strict C11.
 *
 * Why: the bindings that include jsb.h are C, so the header must never pull
 * in C++ or V8 (spec section 4.1, docs/js-binding.md). tests/smz/jsb_core.cmake
 * builds this file with -std=c11 -pedantic-errors in both engine modes; a
 * V8 or C++ include, or a V8 type in a declaration, fails that build.
 */
#include "jsb.h"

#ifdef __cplusplus
#error "this check must be compiled as C"
#endif

/* A binding-shaped function, so the jsb_fn typedef is exercised as C. */
static void check_fn(jsb_ctx *ctx) { jsb_ret_undefined(ctx); }

/* Exported so the compiler keeps the references; never called. */
jsb_fn jsb_header_check_fn(void) { return check_fn; }
