<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The smzuggler contributors -->

# Canvas2D context capacity

The software rasterizer uses a growable registry, reusing inactive slots before
expanding it. Context handles stay valid across growth; callers must not retain
internal context pointers. Pixel buffers and saved drawing state remain owned
by the context until destruction or shutdown. Registry allocation failure still
returns an invalid handle, without disturbing existing canvases.

The former 256-live-context ceiling could silently leave later HUD, message,
and choice canvases without native handles. Such canvases contained no text
pixels, independent of render-target clipping or texture upload. Scenes may
legitimately keep more than 256 canvases alive, so increasing a fixed ceiling
would only move the failure to a later allocation.

`canvas2d_capacity` creates 1,025 simultaneous canvases and checks pixels, saved
state, destruction, slot reuse and a second complete lifecycle. It requires no
game files, installed fonts, display server or GPU. The existing `canvas2d` and
`pixi_shim` suites cover rasterization and texture/render-target integration.
