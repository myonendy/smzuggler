<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The smzuggler contributors -->
# Canvas2D hot-handle lookup

The growable canvas registry must not turn repeated draws on one canvas into
work proportional to every canvas created earlier. Software radial-gradient
path fills emit a native set-color and one-pixel fill for every covered pixel.
Recreating map lighting after a transfer or menu close therefore exposed the
linear search in `find_ctx`, rather than expensive canvas allocation itself.

The runtime-thread-only lookup caches a slot index. It validates both the live
flag and handle on every hit. An index survives registry reallocation; validating
the handle prevents a destroyed handle from reaching a subsequently reused slot.
Shutdown invalidates the cache. Cold lookups still search the registry; no limit
is reintroduced, and no canvas ownership or drawing semantics change.

## Regression and local evidence

`canvas2d_lookup` compares CPU time for 100,000 set-color/one-pixel-fill pairs on
an early canvas and one behind 4,095 other live canvases. It allows a generous
12x ratio plus 2 ms scheduling/clock slack, but rejects per-call registry scans.
It also checks growth across a cached slot, destruction, stale handles, slot
reuse, resize, invalid handles, and shutdown/reinitialization. The regression
failed before the change and passes afterward at approximately 3 ms for both
sparse and dense registries. `canvas2d_capacity` still checks 1,025 live canvases.

An isolated, read-only game-copy probe on dev-laptop counted about 295,600
set-color/fill pairs while rebuilding map lighting. With identical timing
wrappers, three map-load callbacks fell from 1.98/2.78/2.36 s to
0.49/0.47/0.48 s. Two menu-return callbacks fell from 2.87/3.47 s to
0.60/0.78 s. Instrumentation and rendering affect these measurements; they are
not a claim of hitch-free transitions or a 60 Hz performance-gate pass.
Menu-open snapshots remained roughly 50-90 ms. Further optimization of bulk
gradient rasterization, cold registry lookups and canvas lifetime is separate.
The previously observed black menu background and intermittent empty dialogue
were not established to share this cause.
