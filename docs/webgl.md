<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
The WebGL2 context, state, buffers, VAOs and draws over GLES 3.0, and the
GLES provider switch they run on. Spec §4.2 and §11 Q5.
-->

# WebGL2 over GLES 3.0

The WebGL2 surface real PIXI and UltraMode7 drive, implemented in
`src/webgl/webgl.cc` on GLES 3.0 / GL 3.3 core (spec §4.2). This page covers the
GLES provider switch and the context, state, buffer, vertex-array and draw
entry points. Textures, framebuffers, shaders and extensions follow in M2-T002
and M2-T003. Spec §4.2 and §11 Q5: Q5 is open, so this page describes the
switch, not a ruling.

## What this milestone provides

- A CMake switch, `-DMZ_GLES=system|angle`, that resolves one provider into
  the `smz_gles` target. The runtime links that target, so the selected
  provider is what the WebGL2 entry points call.
- A native WebGL2 context in `src/webgl/`: `getContext('webgl2')` returns it
  instead of a stub, with full context state, buffers, VAOs and
  `drawArrays`/`drawElements`.
- V8 Fast API registration for the hot calls listed below.
- An `angle_switch` ctest that runs a binary built against the selected
  provider and confirms from the EGL client extensions that the linked library
  is the one the switch asked for.
- Pinned ANGLE metadata and a documented Linux CI oracle job, in
  [Building smzuggler](build.md).

It deliberately does **not** implement textures, framebuffers, shaders or
extensions yet (M2-T002 and M2-T003), and it does not claim that PIXI runs on
it yet.

## Context binding

`src/shims/dom_shim.js` maps `getContext('webgl2')`, `'webgl'` and
`'experimental-webgl'` to `__native_webgl.createContext(width, height, attrs)`.
`src/webgl/webgl_runtime.c` registers that module at startup beside Canvas2D;
on Linux the linker routes Canvas2D's registration through it because
`src/main.c` is outside this card's files. Windows/macOS registration lands
with ANGLE (M2-T004). When no GL device or entry point exists,
`createContext` returns `null`, the same honest answer Chrome gives.

The returned object is built directly on V8 with the WebGL2 methods and
constants, so PIXI sees `gl.createBuffer`, `gl.ARRAY_BUFFER` and so on without
a JS wrapper. `ctx.canvas` is set by the shim, and the module also exposes
`fastApiInfo()` for the Fast API test.

`HTMLCanvasElement`'s `width`/`height` setters call `__native_webgl.resize`, so
`drawingBufferWidth`/`drawingBufferHeight` follow the canvas. WebGL does not
change the viewport when the drawing buffer is resized, so neither does the
binding; callers keep their `viewport` call. The properties are prototype
accessors reading the live context, not snapshot values.

## Default framebuffer

WebGL renders into framebuffer name `0`; `bindFramebuffer(target, null)` maps to
`0`. With `RMMZ_RENDER_BACKEND=gl` the backend uses the platform window's
existing OpenGL context, so WebGL draws into the presented surface. The default
`gpu` backend has no GL context, so the module creates a hidden GL window until
real-PIXI/ANGLE replaces it; screen presentation through SDL3 GPU is not claimed
here. `getError()` returns and clears the native GL error queue (and any
synthetic error, see below), and a new context starts with
depth/stencil/cull/blend/scissor off, a transparent clear colour and a viewport
equal to the drawing buffer.

## Hot calls and lifetime

`bindBuffer`, `bufferSubData`, `drawElements`, `bindTexture`,
`bindVertexArray`, `useProgram`, `uniform1f`, `uniform1i`, `uniform4f`,
`uniform1fv`, `uniform4fv` and `uniformMatrix4fv` are registered with
`v8::CFunction` (V8 Fast API) as well as a slow validation callback (spec §4.2);
`tests/smz/test_webgl_fast_api.cc` checks the registration counter.

Fast calls must not allocate on the JS heap, so typed-array bytes are read with
`ArrayBufferView::HasBuffer`/`CopyContents` into a small stack buffer (V8's
on-heap typed arrays are at most 64 bytes in this pin) or borrowed directly from
an off-heap backing store. A foreign receiver and invalid `bufferSubData` data
throw the same `TypeError` from both paths, so whether TurboFan has optimised a
call site cannot change JS-visible behaviour.

Every buffer/VAO wrapper owns one GL name: `delete*()` frees it now, GC only
queues it, and the context drains that queue when destroyed. Using a deleted or
foreign-context wrapper records `INVALID_OPERATION` in the context (returned by
the next `getError()`) instead of silently binding name `0`. A collected context
is likewise only queued; the next `createContext` and the exported
`webgl_drain()` (for the frame loop and teardown) perform the GL deletes on the
main thread, because spec §15.8 forbids GL calls in a GC finalizer. On context
destruction, names still held by live wrappers are deleted and the wrappers are
marked dead; if the GL context cannot be made current (lost device or closed
window), the names are forgotten instead of calling GL. No `Local` is stored in
C. Teardown calls `webgl_shutdown` to release the per-isolate context template.
This is the short form of spec §15.8 and
[js-binding.md](js-binding.md#handle-rules); M2-T002 adds the full lifetime rules
for textures and framebuffers.

## Tests

`tests/smz/test_webgl_context.c` creates a context through `dom_shim.js`, sets
state, uploads buffers, binds a VAO and clears without a GL error; it also
checks that a canvas resize updates `drawingBufferWidth` and that using a
deleted buffer yields `INVALID_OPERATION`. It skips only when the backend
reports no GL device or entry point. The fast-api test needs no device. Rows for
every entry point are in [compat-surface.md](compat-surface.md).

## Platform matrix

| Platform | Intended provider | Why |
|---|---|---|
| Linux, Steam Deck | `system` (Mesa EGL/GLES) | The §14 frame budgets are measured on this path, and it needs no extra binary size |
| Windows | `angle` (D3D11) | Native GLES does not exist; ANGLE is the Chrome-exact route |
| macOS | `angle` (Metal) | Apple caps OpenGL at 4.1 and has no GLES; Metal through ANGLE is the supported route |
| Android | `system` (EGL/GLES) | Deferred by spec §17; the switch already resolves EGL/GLES |
| Linux CI oracle | `angle` | Same provider Chrome uses, without changing the measured Linux target |

The default is `system` everywhere, so a machine without ANGLE still configures
and builds. Windows and macOS release builds pass `-DMZ_GLES=angle` and
`-DSMZ_ANGLE_DIR=<staged ANGLE>`; the configure step names whichever variable
is missing. On Linux, `-DMZ_GLES=angle` builds the CI reference oracle.

## Link-time identity

EGL and GLES have one ABI, so ANGLE and Mesa are indistinguishable by symbol
names or by a successful link. The client extension query is the difference:
ANGLE advertises `EGL_ANGLE_*` platform extensions even before a display is
created, while Mesa does not. `tests/smz/test_angle_switch.c` therefore:

1. prints the provider recorded at compile time (`SMZ_GLES_ANGLE` or
   `SMZ_GLES_SYSTEM`) and the EGL client extension string;
2. fails an ANGLE build whose client does not advertise `EGL_ANGLE_*`;
3. fails a system build whose client *does* advertise it.

Failures exit nonzero independently of assertions, including in Release builds.
A regression test supplies missing and mismatched EGL responses with `NDEBUG`
defined to verify that ctest cannot mistake those failures for success.

It creates no window and no GPU device, so it runs on a headless CI runner.
The system build's link line contains the platform EGL/GLES libraries only, so
it has no ANGLE dependency at run time.

## Q5 is still open

Spec §11 Q5 asks whether ANGLE should be used everywhere or only on platforms
without good native GL, with the Linux CI oracle also using it. §4.2 recommends
the second option and this milestone implements the switch it describes, but
the choice belongs to the owner. Nothing here forecloses "ANGLE everywhere":
the same `-DMZ_GLES=angle` build would then become the measured build too.
