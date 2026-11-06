<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-11-06: document real-PIXI versus --renderer=outsider (M2-T005).
The WebGL2 context, state, buffers, VAOs, textures, framebuffers,
renderbuffers, draws, shader pipeline, extensions and parameters over GLES 3.0,
and the GLES provider switch they run on. Spec §4.2 and §11 Q5.
-->

# WebGL2 over GLES 3.0

The WebGL2 surface real PIXI and UltraMode7 drive, implemented in
`src/webgl/webgl.cc` on GLES 3.0 / GL 3.3 core (spec §4.2). This page covers the
GLES provider switch, the context, state, buffer, vertex-array and draw entry
points, textures and pixel uploads, framebuffers and renderbuffers, stencil,
readback, shaders and the GLSL ES rewrite, extensions, parameters and the
object lifetime rules. Spec §4.2 and §11 Q5: Q5 is open, so this page describes
the switch, not a ruling.

## What this milestone provides

- A CMake switch, `-DMZ_GLES=system|angle`, that resolves one provider into
  the `smz_gles` target. The runtime links that target, so the selected
  provider is what the WebGL2 entry points call.
- A native WebGL2 context in `src/webgl/`: `getContext('webgl2')` returns it
  instead of a stub, with full context state, buffers, VAOs and
  `drawArrays`/`drawElements`.
- Textures from typed arrays, `ArrayBufferView`s, Canvas2D canvases, images and
  `ImageData`, with `texSubImage2D` updates and WebGL2's flip/premultiply
  unpack rules.
- Framebuffers with texture or renderbuffer attachments, depth-stencil
  renderbuffers, `readPixels` into a typed array, and the stencil test.
- GLSL ES 1.00 and 3.00 shader compilation and linking, including the
  desktop-GL 3.3 core rewrite described below, with compile errors that name
  the author's source line.
- `getExtension`/`getSupportedExtensions` over a driver capability probe, and
  Chrome-typed `getParameter` values for the limits PIXI reads.
- V8 Fast API registration for the hot calls listed below.
- An `angle_switch` ctest that runs a binary built against the selected
  provider and confirms from the EGL client extensions that the linked library
  is the one the switch asked for.
- A `webgl_textures` ctest that uploads known pixels from a typed array and a
  Canvas2D canvas and reads them back; a `webgl_fbo` ctest that draws into a
  texture plus depth-stencil renderbuffer and checks that a stencil-failed draw
  leaves colour unchanged.
- Pinned ANGLE metadata and a documented Linux CI oracle job, in
  [Building smzuggler](build.md).
- `webgl_shaders` and `webgl_extensions` ctests covering both GLSL ES
  versions, the compile-error log, the recorded PIXI extension probes and the
  parameter types and limits.
- Real-PIXI mode as the startup default, with `--renderer=outsider` keeping
  the pre-WebGL2 shim route, and a `real_pixi_loader` ctest over both planned
  script lists.

## Real-PIXI mode and `--renderer=outsider`

`src/main.c` defaults to `--renderer=real-pixi` and passes that choice to
`script_loader_load_all_for`. In this mode the loader skips `pixi_shim.js` and
`tilemap_shim.js` and loads the game's own `js/libs/pixi.js` before
`pako.min.js` and the RMMZ core scripts. Real PIXI asks
`getContext('webgl2')`, which returns the context described on this page, so
the game's graphics, including UltraMode7, run against the same API Chrome
exposes.

`--renderer=outsider` keeps the previous route: both shims load, the game's
`pixi.js` is not forced in, and the sprite-batch renderer path is unchanged.
It exists for comparison and for tracking upstream (§8.3); it is not a second
WebGL2 implementation. The accepted
spellings are `real-pixi`, `real_pixi`, `realpixi` and `outsider`; an unknown
value fails startup rather than silently choosing a mode.
`script_loader_get_load_order_for` returns each mode's planned list without
evaluating anything, which `tests/smz/test_real_pixi_loader.c` uses to pin
the split. Which path actually renders correctly on the owner's machine is a
GUI run on the test copy (title, Map 92 and the Mode 7 floor), not something
this test claims.

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
`gpu` backend has no GL context, so the module creates an EGL offscreen context (or a hidden utility window if
EGL is unavailable) until real-PIXI/ANGLE replaces it; screen presentation
through SDL3 GPU is not claimed here. `getError()` returns and clears the native GL error queue (and any
synthetic error, see below), and a new context starts with
depth/stencil/cull/blend/scissor off, a transparent clear colour and a viewport
equal to the drawing buffer.

## Textures and pixel uploads

`createTexture`, `deleteTexture`, `bindTexture`, `texParameteri`,
`texImage2D`, `texSubImage2D` and `pixelStorei` are implemented over the GLES
entry points. Both `texImage2D` shapes are accepted: the nine-argument
allocation/`ArrayBufferView` form and the six-argument host-source form.
`texSubImage2D` likewise takes nine or seven arguments. A null pixel pointer
allocates storage; a typed array is borrowed for the duration of the call only.

Host sources are an `HTMLCanvasElement` (its live Canvas2D raster), an
`HTMLImageElement` (`_imageHandle`), or an `ImageData`-like object
(`{width, height, data}`). The canvas and image pixels come through the existing
`__native_canvas2d.getPixels` / `__native_image.getImagePixels` bindings, so the
WebGL module owns no image or canvas memory.

`UNPACK_FLIP_Y_WEBGL` and `UNPACK_PREMULTIPLY_ALPHA_WEBGL` follow WebGL2
§5.14.8: they transform RGBA8 host sources (rows reversed, RGB multiplied by
alpha) and are ignored for `ArrayBufferView` uploads. `UNPACK_ALIGNMENT`,
`PACK_ALIGNMENT`, `UNPACK_ROW_LENGTH` and `PACK_ROW_LENGTH` are stored and
forwarded to GL. Alignment must be 1, 2, 4 or 8 and row length must be
nonnegative; invalid values record `INVALID_VALUE` without changing state.
Array views must cover `stride * (height - 1) + width * bytesPerPixel`, where
`stride = align_up((rowLength || width) * bytesPerPixel, alignment)`; empty
regions require no bytes. Readback uses PACK state and uploads use UNPACK state.
Short views or overflowing sizes record `INVALID_OPERATION` before GL can
access their memory. Unknown formats or types (including currently unsupported
RED/RG, integer, luminance and packed pixel layouts) record `INVALID_ENUM`
instead of bypassing validation.

Host uploads accept RGBA/UNSIGNED_BYTE rasters; other layouts record
`INVALID_OPERATION`. They temporarily use row length 0 and alignment 1,
then restore the caller's UNPACK state, so odd-width canvases remain tightly
packed regardless of the typed-array layout.

## Framebuffers, renderbuffers and stencil

`createFramebuffer`, `bindFramebuffer`, `framebufferTexture2D`,
`framebufferRenderbuffer`, `checkFramebufferStatus`, `createRenderbuffer`,
`bindRenderbuffer`, `renderbufferStorage` and `delete*` cover the render-target
path PIXI and VisuMZ use. `bindFramebuffer(target, null)` restores framebuffer
`0`; a depth24-stencil8 renderbuffer can be attached to
`DEPTH_STENCIL_ATTACHMENT`, and separate stencil or depth attachments also work.

`stencilFunc`, `stencilOp`, `stencilMask`, `clearStencil`, `depthFunc`,
`depthMask` and `colorMask` are forwarded. A draw that fails the stencil test
leaves the colour attachment unchanged, exactly as the hardware does; the
`webgl_fbo` ctest proves that path before passing the same draw.

## Readback, shaders and GLSL ES rewriting

`readPixels(x, y, width, height, format, type, view)` copies into the caller's
typed array, which stays JS-owned. Its byte length must cover the PACK layout
described above or the context records `INVALID_OPERATION`.
Readback works from a bound framebuffer and from the default framebuffer after
`bindFramebuffer(FRAMEBUFFER, null)`.

`createShader`, `shaderSource`, `compileShader`, `createProgram`,
`attachShader`, `bindAttribLocation`, `linkProgram`, `getShaderParameter`,
`getProgramParameter`, `getShaderInfoLog`, `getProgramInfoLog`,
`getUniformLocation`, `getAttribLocation`, `getActiveUniform`,
`getActiveAttrib`, the `uniform*` setters and `delete*` cover the path PIXI and
UltraMode7 drive.

WebGL shaders are GLSL ES. On the GLES 3.0 provider the driver compiles them
unchanged, as Chrome does. A desktop GL 3.3 core context cannot accept
`#version 300 es` or the GLSL ES 1.00 spellings, so `glsl_for_target` rewrites
the source only on that path:

- `#version 300 es` becomes `#version 330 core`; a shader with no version (or
  `#version 100`) is treated as GLSL ES 1.00 and gets the desktop version line.
- `attribute` becomes `in`, `varying` becomes `out` in a vertex shader and
  `in` in a fragment shader, `texture2D`/`textureCube` become `texture`, and
  `gl_FragColor` becomes the declared `smz_FragColor` output.
- Standalone `precision` statements and inline `highp`/`mediump`/`lowp` are
  removed; `GL_OES_standard_derivatives` is enabled in core so its directive
  is dropped.
- `#line 1` after the prologue keeps driver error numbers aligned with the
  author's source lines.

A compile failure logs the driver message plus the numbered original source to
stderr, and `getShaderInfoLog` returns the same numbered source after the
driver text. That is a deliberate superset of Chrome's log so plugin authors
get the line that §15.8 requires; `webgl_shaders` checks it.

## Extensions and parameters

`getExtension(name)` returns an object only when the current driver was probed
at startup and really provides the capability; every other name returns
`null`, including WebGL1-only names that are core in WebGL2. Policies:

| Probe | Backed when | Chrome 117 (WebGL2) |
|---|---|---|
| `EXT_color_buffer_float` | GLES has `GL_EXT_color_buffer_float`, or desktop core (core feature) | yes |
| `EXT_texture_filter_anisotropic` | driver lists `GL_EXT_texture_filter_anisotropic` | yes |
| `OES_texture_float_linear` | GLES has `GL_OES_texture_float_linear`, or desktop core (core feature) | yes |
| `WEBGL_lose_context` | not backed yet; the runtime cannot honestly lose/restore the SDL context | yes |

The anisotropic object exposes `TEXTURE_MAX_ANISOTROPY_EXT` and
`MAX_TEXTURE_MAX_ANISOTROPY_EXT`. `getSupportedExtensions()` lists exactly the
backed names, so it never advertises a probe the driver cannot honour.
`tests/fixtures/webgl_pixi_extensions_chrome117.txt` records every name PIXI
5.3.12 probes on a WebGL2 context and Chrome 117's answer; `webgl_extensions`
fails if the supported set invents a name, and requires Chrome's WebGL2-only
`null` answers to stay `null`.

`getParameter` forwards limits to `glGetIntegerv`, so values are the driver's
real limits rather than invented constants: `MAX_TEXTURE_SIZE`,
`MAX_VERTEX_ATTRIBS`, `MAX_TEXTURE_IMAGE_UNITS`,
`MAX_COMBINED_TEXTURE_IMAGE_UNITS`, `MAX_RENDERBUFFER_SIZE` and the rest.
State arrays (`VIEWPORT`, `SCISSOR_BOX`, `MAX_VIEWPORT_DIMS`) come back as
`Int32Array`, float state (`COLOR_CLEAR_VALUE`, depth range, line width) as
`Float32Array` or a number, and boolean state as a boolean, matching Chrome's
IDL types. `MAX_TEXTURE_MAX_ANISOTROPY_EXT` works when the anisotropic
extension is backed.

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

Every GL object wrapper owns exactly one GL name: buffers, vertex arrays,
textures, framebuffers, renderbuffers, shaders and programs. `delete*()` frees
the name now and marks the wrapper dead; GC only queues the name, and the main
thread drains that queue through `webgl_drain()` or the next GL call. The frame
loop must call `webgl_drain()` at end of frame to satisfy §15.8; that integration
still requires a follow-up card owning `game_loop.c`. Using a dead or foreign-context
wrapper reports `INVALID_OPERATION` on the next `getError()`, exactly like a
WebGL call after `delete*()`. Deleting or forgetting a wrapper never frees a
`Local` held elsewhere. Player code never sees a raw GL name.

A collected context is likewise only queued. The next `createContext`, an
explicit `webgl_drain()`, and teardown perform the GL deletes on the main
thread, because spec §15.8 forbids GL calls in a GC finalizer. On context
destruction, names still held by live wrappers are deleted and the wrappers are
marked dead; if the GL context cannot be made current (lost device or closed
window), names are forgotten instead of calling GL. No `Local` is stored in C.
Teardown calls `webgl_shutdown` to release the per-isolate context template.
This is the short form of spec §15.8 and
[js-binding.md](js-binding.md#handle-rules).

## Tests

`tests/smz/test_webgl_context.c` creates a context through `dom_shim.js`, sets
state, uploads buffers, binds a VAO and clears without a GL error; it also
checks that a canvas resize updates `drawingBufferWidth` and that using a
deleted buffer yields `INVALID_OPERATION`. `tests/smz/test_webgl_textures.c`
uploads known pixels from a typed array, a two-row Canvas2D canvas and an
`ImageData`-like object, reads each back through an FBO, and checks that
FLIP_Y/PREMULTIPLY transform a host source but not a typed array.
`tests/smz/test_webgl_fbo.c` builds a texture plus depth24-stencil8
renderbuffer, draws through a shader, proves a stencil-failed draw leaves colour
unchanged, then restores framebuffer `0` and reads it back.
`tests/smz/test_webgl_shaders.c` compiles and links a GLSL ES 3.00 program and
a GLSL ES 1.00 program, draws with each and reads the result back, then checks
that a syntax error's log names the author's source line.
`tests/smz/test_webgl_extensions.c` walks the recorded PIXI 5.3.12 probe list,
checks that unbacked and unknown names return `null`, and checks the parameter
types and limits. Every device test
skips only when the backend reports no GL device or entry point; the fast-api
test needs no device. Rows for every entry point are in
[compat-surface.md](compat-surface.md).

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
