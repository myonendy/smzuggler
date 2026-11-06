<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
The WebGL2 target and the GLES provider it runs on.
Spec §4.2 and §11 Q5. Q5 is open: this page describes the switch, not a ruling.
-->

# WebGL2 and the GLES provider

Real PIXI 5.3.12 and UltraMode7 drive the full WebGL2 API, so the runtime
targets a WebGL2 implementation over GLES 3.0 (spec §4.2). WebGL2 is GLES 3.0
plus validation, which keeps the mapping close enough that the same code can
run on Mesa, on ANGLE/D3D11 and on ANGLE/Metal. This page describes the
provider switch; the WebGL2 entry points themselves are a later milestone.

## What this milestone provides

- A CMake switch, `-DMZ_GLES=system|angle`, that resolves one provider into
  the `smz_gles` target.
- The runtime links that target, so the selected provider is what the future
  WebGL2 entry points will call.
- An `angle_switch` ctest that runs a binary built against the selected
  provider and confirms from the EGL client extensions that the linked library
  is the one the switch asked for.
- Pinned ANGLE metadata and a documented Linux CI oracle job, in
  [Building smzuggler](build.md).

It deliberately does **not** implement `getContext('webgl2')`, the WebGL2
state machine, buffers, textures or draws, and it does not claim that PIXI runs
on it yet.

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

It creates no window and no GPU device, so it runs on a headless CI runner.
The system build's link line contains the platform EGL/GLES libraries only, so
it has no ANGLE dependency at run time.

## Q5 is still open

Spec §11 Q5 asks whether ANGLE should be used everywhere or only on platforms
without good native GL, with the Linux CI oracle also using it. §4.2 recommends
the second option and this milestone implements the switch it describes, but
the choice belongs to the owner. Nothing here forecloses "ANGLE everywhere":
the same `-DMZ_GLES=angle` build would then become the measured build too.
