<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
-->

# src/webgl

Native WebGL2 over GLES 3.0 / GL 3.3 core (spec §4.2). See
[docs/webgl.md](../../docs/webgl.md) and
[js-binding.md](../../docs/js-binding.md#handle-rules).
`webgl.cc` owns the GL context, C state, V8 objects and Fast API callbacks;
`webgl_define` defines `__native_webgl`, and `webgl_runtime.c` calls it beside
Canvas2D at startup. `dom_shim.js` asks `__native_webgl.createContext` and
`__native_webgl.resize`; JS calls run `slow_method`, while the §4.2 hot calls
also have a `v8::CFunction` fast path that allocates nothing on the JS heap.
Buffer/VAO wrappers own one GL name each: explicit deletes free it now, GC only
queues it, and a collected context is queued for `webgl_drain()` (or the next
`createContext`) because §15.8 forbids GL in a finalizer. No `Local` is stored
in C. Shaders keep both the author's source and the text handed to the driver;
on desktop GL 3.3 core that text is the GLSL ES 1.00/3.00 rewrite described in
[docs/webgl.md](../../docs/webgl.md). Extensions are advertised only after a
driver probe. Tests: `tests/smz/test_webgl_context.c`,
`tests/smz/test_webgl_shaders.c`, `tests/smz/test_webgl_extensions.c` and
`tests/smz/test_webgl_fast_api.cc`.
