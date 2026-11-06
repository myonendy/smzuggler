<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
-->

# WebGL2 over GLES 3.0

The WebGL2 surface real PIXI and UltraMode7 drive, implemented in
`src/webgl/webgl.cc` on GLES 3.0 / GL 3.3 core (spec §4.2). This card covers
context state, buffers, vertex arrays and the draw entry points. Textures,
framebuffers, shaders and extensions follow in M2-T002 and M2-T003.

## Context binding

`src/shims/dom_shim.js` maps `getContext('webgl2')`, `'webgl'` and
`'experimental-webgl'` to `__native_webgl.createContext(width, height, attrs)`.
`src/webgl/webgl_runtime.c` registers that module at startup beside Canvas2D.
When no GL device or entry point exists, `createContext` returns `null`, the
same honest answer Chrome gives; the old hollow stub is gone.

The returned object is built directly on V8 with the WebGL2 methods and
constants, so PIXI sees `gl.createBuffer`, `gl.ARRAY_BUFFER` and so on without
a JS wrapper. `ctx.canvas` is set by the shim; changing `canvas.width` or
`canvas.height` calls the internal `_resize` method. The module also exposes
`fastApiInfo()` for the Fast API test.

## Default framebuffer

WebGL renders into framebuffer name `0`; `bindFramebuffer(target, null)` maps to
`0`. With `RMMZ_RENDER_BACKEND=gl` the backend uses the platform window's
existing OpenGL context, so WebGL draws into the presented surface. The default
`gpu` backend has no GL context, so the module creates a hidden GL window until
the real-PIXI/ANGLE integration replaces it. Screen presentation through the
SDL3 GPU backend is not claimed here.

`getError()` returns and clears the GL error queue. The context applies WebGL's
initial state (depth/stencil/cull/blend/scissor off, transparent clear colour,
viewport equal to the drawing buffer).

## Hot calls and lifetime

`bindBuffer`, `bufferSubData`, `drawElements`, `bindTexture`,
`bindVertexArray`, `useProgram`, `uniform1f`, `uniform1i`, `uniform4f`,
`uniform1fv`, `uniform4fv` and `uniformMatrix4fv` are registered with
`v8::CFunction` (V8 Fast API) as well as a slow validation callback
(spec §4.2). `tests/smz/test_webgl_fast_api.cc` checks the registration counter.

Every buffer/VAO wrapper owns one GL name: `delete*()` frees it now, GC only
queues it, and the context drains that queue at frame end. No `Local` is stored
in C. This is the short form of spec §15.8 and
[js-binding.md](js-binding.md#handle-rules); M2-T002 completes the texture and
framebuffer rules.

## Tests

`tests/smz/test_webgl_context.c` creates a context through `dom_shim.js`, sets
state, uploads buffers, binds a VAO, clears without a GL error and resizes; it
skips only when the backend reports no GL device or entry point.
`tests/smz/test_webgl_fast_api.cc` needs no device. Rows for every entry point
are in [compat-surface.md](compat-surface.md).

