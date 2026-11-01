<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Module overview of src/jsb, the C binding seam (spec section 15.1).
-->
# src/jsb: the C binding seam

**Purpose.** `jsb.h` is the plain-C API that native bindings use to talk to the JS engine
(spec §4.1), so Outsider's QuickJS bindings port to V8 by near-mechanical rewriting.

**Entry points.** `jsb.h` (the API); `jsb_v8.cc` (its V8 backend, the CMake target
`smz_jsb`, built only with `SMZ_JS_ENGINE=v8`).

**Data flow.** The host makes a `jsb_runtime`, defines `__native_*` modules in
`jsb_runtime_setup`, and runs scripts. JS calls into a `jsb_fn` through one trampoline that
opens a HandleScope and builds a call-scoped `jsb_ctx`. A binding that must run JS later (audio and video
events) keeps a `jsb_callback` and the host calls it with `jsb_callback_call`; exceptions
go to the runtime's error handler, never into C.

**Ownership.** The host owns the `jsb_runtime`. A `jsb_ctx` and its `jsb_val`s live only for
one call. Strings are copied in and out (caller buffers). A `jsb_callback` has one owner, which frees it;
`jsb_runtime_destroy` releases any left. See
[docs/js-binding.md](../../docs/js-binding.md) for the handle rules (§15.8).

**Tests.** `tests/smz/test_jsb_core.c` (ctest `jsb_core`, v8 mode) and the C11 header check
`tests/smz/test_jsb_header.c` (both modes), registered by `tests/smz/jsb_core.cmake`.
`tests/smz/test_jsb_callbacks.c` (ctest `jsb_callbacks`, both modes) covers persistent
callbacks.

**Compat-table rows.** None: jsb is internal and implements no web API itself.
