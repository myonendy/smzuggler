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
Canvas2D at startup. `dom_shim.js` asks `__native_webgl.createContext`; JS calls
run `slow_method`, while the §4.2 hot calls also have a `v8::CFunction` fast
path. Buffer/VAO wrappers own one GL name each: explicit deletes free it now,
GC only queues it for the frame drain, and no `Local` is stored in C. Tests are
`tests/smz/test_webgl_context.c` and `tests/smz/test_webgl_fast_api.cc`.
