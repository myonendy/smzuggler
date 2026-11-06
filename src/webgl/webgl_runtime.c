/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Registers the WebGL2 module alongside the existing Canvas2D binding.
 *
 * Why: main.c is not part of this card's owned files, but the runtime still
 * has to define `__native_webgl` before shims run. The build redirects the
 * main executable's bind_canvas2d_define reference through --wrap (see
 * CMakeLists.txt); this tiny wrapper keeps that build detail in src/webgl/
 * and leaves the Canvas2D binding untouched. See docs/webgl.md.
 *
 * Rules: main thread only. The wrapper must not change Canvas2D's result; a
 * WebGL failure is reported but does not fail the runtime's Canvas2D setup.
 */
#include "webgl/webgl.h"

#include "bindings/bind_canvas2d.h"
#include "jsb/jsb.h"

#include <stdio.h>

#if defined(__linux__) && !defined(__ANDROID__)

/* The linker's --wrap target for bind_canvas2d_define. */
extern bool __real_bind_canvas2d_define(jsb_runtime *rt);

/* Defines Canvas2D first, then WebGL2 on the same live runtime.
 *
 * Why: the runtime's binding order stays as before, and the WebGL module is
 * present before script_loader evaluates dom_shim.js. Returns Canvas2D's
 * result; a WebGL define failure is logged because it only disables WebGL. */
bool __wrap_bind_canvas2d_define(jsb_runtime *rt)
{
    bool ok = __real_bind_canvas2d_define(rt);
    if (ok && !webgl_define(rt))
        fprintf(stderr, "webgl: module registration failed; "
                        "getContext('webgl2') will return null\n");
    return ok;
}

#endif /* __linux__ && !__ANDROID__ */
