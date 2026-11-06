<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Why WebGL2 registration goes through a linker wrap and where the startup call
must move (spec section 4.2, card M2-T001).
-->
## DEC-M2-T001 — WebGL2 registration rides the Canvas2D define call by linker wrap

**Context.** `__native_webgl` must exist before `dom_shim.js` runs, but
`src/main.c`, which calls `bind_canvas2d_define`, is outside this card's `owns`.
Without a hook, `getContext('webgl2')` would always return null.

**Decision.** On Linux, `CMakeLists.txt` adds
`-Wl,--wrap=bind_canvas2d_define`; `src/webgl/webgl_runtime.c` implements the
wrapper, calls the real Canvas2D define, then `webgl_define`. Elsewhere no wrap
is added because `--wrap` is a GNU ld feature, so Windows and macOS builds get
WebGL only once `src/main.c` calls `webgl_define` explicitly.

**Why.** It keeps registration out of a protected file, lets the module be
tested now, and makes the platform gap visible in one place.

**Rejected.** A constructor function would run before SDL/V8 setup and cannot
see the jsb runtime; editing `src/main.c` is out of scope; a second Canvas2D
define would register the module twice.

**Follow-up.** A later card with `src/main.c` in `owns` should call
`webgl_define`, `webgl_drain` and `webgl_shutdown` explicitly and remove the
linker wrap.
