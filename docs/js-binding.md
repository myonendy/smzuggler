<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
How native C bindings talk to the JS engine through jsb.h, and the handle rules.
-->
# JS binding: the `jsb` seam

`src/jsb/jsb.h` is the C API that native bindings (`src/bindings/`) use to expose
`__native_*` modules to the shims. It replaces direct QuickJS calls so that Outsider's
bindings move to V8 by near-mechanical rewriting (spec §4.1). It is **not** an engine
abstraction: its one real implementation is `src/jsb/jsb_v8.cc`.

Status: core API only (M1-T003). ArrayBuffer/TypedArray views and persistent callback
handles (§4.1) come in later cards. The library target is `smz_jsb` in both engine
modes; `jsb.h` itself is plain C11 and includes no C++ or V8 header (checked by
`tests/smz/test_jsb_header.c` in both engine modes).

## Transitional QuickJS backend (removed by M1-T027)

`src/jsb/jsb_quickjs.c` implements `jsb.h` on QuickJS-ng, and `smz_jsb` is built from it
when `SMZ_JS_ENGINE=quickjs` (the default). It exists only so bindings can be ported to
`jsb` one at a time while the game still runs on QuickJS and both engine modes stay green
(docs/SPEC_ISSUES.md SI-11, DEC-SETUP-008). **Card M1-T027 deletes it** together with
QuickJS. Rules while it lives:

- It must behave exactly like `jsb_v8.cc`: the unchanged `tests/smz/test_jsb_core.c` runs
  against it as ctest `jsb_core` in quickjs mode (`tests/smz/jsb_core_quickjs.cmake`).
  Any new jsb call lands in both backends in the same change.
- It adds no API of its own; nothing outside `src/jsb/` may include `quickjs.h` through it.
- `jsb_runtime_create` ignores `argv0` and `icu_data_path` (QuickJS has no ICU).
- Handles: each `jsb_val` owns one QuickJS reference, kept in fixed chunks owned by the
  call's `jsb_ctx` and freed when the call returns, so the "valid only during the call"
  rule below holds exactly as with V8 `Local`s. A native function is registered through a
  per-runtime table indexed by the QuickJS function's 16-bit magic, so a runtime holds at
  most 32,767 distinct `jsb_fn`s.
- Known difference: a string argument holding a lone UTF-16 surrogate is read out in
  QuickJS's encoding of it, where V8 writes U+FFFD.

## API

### Host side
| Call | Purpose |
| --- | --- |
| `jsb_set_flags(flags)` | Engine flags such as `--expose-gc`, process-wide. Only before the first runtime (V8 freezes its flags); false afterwards. The QuickJS backend honours `--expose-gc` and ignores V8-only flags. |
| `jsb_runtime_create(argv0, icu_data_path)` | One isolate and one context. The first call starts V8 for the process. NULL on failure, reason on stderr. |
| `jsb_runtime_destroy(rt)` | Frees the runtime. |
| `jsb_runtime_setup(rt, fn, user)` | Runs `fn(ctx, user)` in a fresh handle scope, for defining modules. |
| `jsb_runtime_eval(rt, src, file, out, cap)` | Runs a classic script; writes `String(result)` or the thrown value's string to `out`. True when it completed. |

### Inside a native function (`void fn(jsb_ctx *ctx)`)
| Call | Behaviour |
| --- | --- |
| `jsb_arg_count` | Number of arguments the caller passed. |
| `jsb_arg_i32(ctx, i, &out)` | Needs a Number; converted like ToInt32 (wraps, NaN is 0). |
| `jsb_arg_f64(ctx, i, &out)` | Needs a Number. |
| `jsb_arg_bool(ctx, i, &out)` | Needs a Boolean; no truthiness. |
| `jsb_arg_str(ctx, i, buf, cap, &len)` | Needs a String; UTF-8 into `buf`, NUL-terminated. RangeError if it needs more than `cap - 1` bytes. |
| `jsb_arg_obj(ctx, i)` | Needs an Object (functions and arrays count, `null` does not). |
| `jsb_arg_val(ctx, i)` | Any value; never throws. |
| `jsb_arg_bytes(ctx, i, &data, &len)` | Needs an ArrayBuffer, TypedArray or DataView; gives its bytes in place (see "Buffers"). TypeError if detached. |
| `jsb_new_external_buffer(ctx, data, len, free_fn, user)` | Wraps native memory as an ArrayBuffer without copying; JS then owns it. |
| `jsb_adjust_external_memory(ctx, change)` | Reports native memory held by JS wrappers to the GC; returns the runtime's total. |
| `jsb_ret_undefined/_bool/_i32/_f64/_str/_val` | Set the result; the last call wins; ignored after a throw. |
| `jsb_throw_type/_range(ctx, fmt, ...)` | Throw TypeError/RangeError with a printf-style message. Only the first throw of a call counts. |
| `jsb_new_object`, `jsb_set_fn`, `jsb_set_prop`, `jsb_get_prop` | Build and read objects. Set and get are ordinary `[[Set]]`/`[[Get]]`, so accessors run. |
| `jsb_define_module(ctx, name, init)` | Makes a fresh object, fills it with `init`, then sets the global `name`. A failed `init` leaves no global. |

**Missing arguments** count as `undefined`, so every typed getter throws
`TypeError: argument N must be ...` (N counts from 1) for them. Getters never coerce
(DEC-M1-T003). When a getter returns false or NULL, an exception is pending: return at
once. A function that sets no result returns `undefined`.

```c
static void fn_add(jsb_ctx *ctx) {
    double a, b;
    if (!jsb_arg_f64(ctx, 0, &a) || !jsb_arg_f64(ctx, 1, &b)) return;
    jsb_ret_f64(ctx, a + b);
}
static bool init_math(jsb_ctx *ctx, jsb_val *m) {
    return jsb_set_fn(ctx, m, "add", fn_add);
}
/* in a jsb_runtime_setup callback: */
jsb_define_module(ctx, "__native_math", init_math);
```

## Ownership of strings
* **Strings in** (`name`, `fmt`, `jsb_ret_str` text, script source) are borrowed for the
  call and copied into the engine. The caller keeps and frees them.
* **Strings out** go into a caller buffer (`jsb_arg_str`, `jsb_runtime_eval`). jsb never
  returns a pointer to engine-owned text, so there is nothing to free.
* `jsb_arg_str` refuses to truncate (RangeError); `jsb_runtime_eval` truncates its
  diagnostic output to fit. Both always NUL-terminate. A string may hold embedded NULs:
  use the `len` out-parameter, not `strlen`.
* Text is UTF-8. Invalid UTF-8 given to jsb becomes U+FFFD.

## Buffers (zero-copy)
Pixel, audio and file data cross the seam without copies (DEC-M1-T005).

* **`jsb_arg_bytes`** gives a pointer to the first byte of the view (its byte offset
  applied) and its length in **bytes**, not elements. The memory belongs to JS. The
  pointer is valid only until the native function returns, and only while it runs no JS
  (JS could detach the buffer). Writes through it are visible to JS at once. `data` may be
  NULL when `len` is 0. A detached ArrayBuffer, or a view on one, throws a TypeError.
  SharedArrayBuffer is not accepted.
* **`jsb_new_external_buffer`** returns an ArrayBuffer over native memory the caller
  allocated. On success JS owns the memory, and `free_fn(data, len, user)` runs
  **exactly once**: after the buffer is garbage-collected or detached, or at the latest
  in `jsb_runtime_destroy`. Until then C may read and write the bytes on the main thread,
  and JS sees the writes. NULL `data` or `free_fn` is a TypeError, an oversize `len` a
  RangeError; on failure the caller still owns `data`.
* **The free function may run on a V8 GC helper thread**, in the middle of a GC. It must
  only release memory: no jsb or V8 calls, no locks the main thread may hold.
* **External memory accounting.** V8 counts every ArrayBuffer backing store, including
  external ones, toward its GC heuristics, so `jsb_new_external_buffer` reports nothing.
  Native memory that JS keeps alive through some *other* object (a texture or decoder
  behind a small wrapper) is reported with `jsb_adjust_external_memory`: add the size when
  it is made and subtract the same amount when it is freed, on the main thread. On V8 this
  calls `Isolate::AdjustAmountOfExternalAllocatedMemory`. A change that would make the
  total negative is refused with -1 and a message on stderr. The QuickJS backend keeps
  the same total but has no GC hook to give it to.

```c
static void free_pixels(void *data, size_t len, void *user) { (void)len; (void)user; free(data); }
/* in a native function: JS owns `pixels` once this succeeds */
jsb_val *buf = jsb_new_external_buffer(ctx, pixels, w * h * 4, free_pixels, NULL);
if (!buf) { free(pixels); return; }
jsb_ret_val(ctx, buf);
```

## Handle rules (§15.8)
These are the rules for code that touches V8 handles, in jsb and elsewhere (WebGL has
its own GL-object rules in `webgl.md`).

* A `Local<T>` is a temporary handle, valid only until the `HandleScope` it was created
  in ends. **Never store a `Local` in a C struct, a global or a static.**
* Every native function called from JS and every callback we drive (timers, rAF, audio
  and video events) opens its own `HandleScope`. A loop that creates many handles opens
  one scope per iteration.
* Return a `Local` out of a scope only with `EscapableHandleScope::Escape`.
* To keep a JS object beyond the current call, use `v8::Global<T>`. Each Global has
  exactly one owner, which calls `Reset()` when done; a leaked Global is leaked memory.
* Weak Globals with finalizers only release native resources; the finalizer never runs
  JS or touches GL.
* Every `ArrayBuffer` backing store says who owns its memory (V8 or our allocator), and
  external memory is reported with `AdjustAmountOfExternalAllocatedMemory`.
* Only the main thread enters the isolate. Other threads post work to it.

How jsb applies them:

* `jsb_ctx` and `jsb_val` are the C face of these rules. Both are valid only until the
  native call (or setup callback) that received the ctx returns. **Never keep a
  `jsb_ctx *` or `jsb_val *`** in a struct, global or static, and never pass one to
  another call. Persistent handles for callbacks are a later jsb API built on `Global`.
* The trampoline in `jsb_v8.cc` opens a `HandleScope` per call before it builds the
  `jsb_ctx` on its stack. The ctx holds the call's Locals in a deque (so `jsb_val`
  pointers stay put) and is destroyed before the scope closes.
* `jsb_runtime_setup` and `jsb_runtime_eval` each open their own scope; a loop that
  creates many values should run in separate setup calls or JS calls.
* The only Global jsb owns is the runtime's context; `jsb_runtime_destroy` resets it
  before disposing of the isolate.
* jsb is main-thread only. The one exception is a `jsb_free_fn`, which V8 may call from
  a GC helper thread (see "Buffers").

## js_engine on V8
`src/engine/js_engine_v8.cc` implements `src/engine/js_engine.h` on V8 (§4.1 item 3,
card M1-T007). In v8 mode CMake builds it as `smz_js_engine_v8`, which defines
`SMZ_JS_ENGINE_V8` for its users. Until `game_loop.c`, `error_handler.c` and the bindings
are ported (M1-T009, M1-T011, M1-T016..T025) the runtime executable still links
`js_engine.c`, because those files call QuickJS through `js_engine_get_context`; the
v8-mode library serves every caller that needs only `js_engine.h`, starting with tests.

- **Isolate and context** come from `jsb_runtime_create`; the engine picks them up inside
  `jsb_runtime_setup`, so `js_engine_get_jsb()` returns a runtime whose modules land in
  the same global object as the game's scripts. Isolate data slot 0 is jsb's, slot 1 the
  engine's. ICU data comes from `SMZ_V8_ICU_DATA_PATH` (the `icudtl.dat` of
  `SMZ_V8_DIR`).
- **Script names**: every compile gets a `ScriptOrigin` with line and column offset 0, so
  traces read `name:line:col` like NW.js DevTools. A file is named by its path relative
  to the game root when the root is known (`js_engine_set_cache_dir`), else by the path
  given. A SyntaxError's `stack` has no frame in V8, so the reported stack gets
  `    at name:line:col` from the exception message.
- **Errors** are reported as in QuickJS mode: the exception's `String()` form, then its
  `stack`, both at level `error` through the console callback (or stderr).
- **Microtasks**: `MicrotasksPolicy::kExplicit`; `js_engine_execute_pending_jobs` runs one
  checkpoint, then reports rejections still unhandled (through
  `__error_handler_dispatchRejection` when the shim is loaded). Its count is the number
  of promise reaction jobs, taken from a promise hook; `queueMicrotask` callbacks are
  not counted. It returns -1 if a microtask threw uncaught.
- **`.jsc` cache**: same paths and mtime rule as `js_engine.c`. The file holds the
  source and V8's code cache, because a V8 code cache only works with its own source
  and `js_engine.h` promises that a hit runs what was cached. M1-T012 replaces it with
  the hashed cache under `$XDG_CACHE_HOME`.
- **Watchdog**: only the limit is stored; scripts are not interrupted on V8 until
  M1-T008.
- **`js_engine_get_context`** is declared only without `SMZ_JS_ENGINE_V8`. In quickjs
  mode `js_engine_get_jsb()` returns NULL: the transitional backend creates its own
  `JSContext` and has no way to adopt the engine's; that needs a change to
  `jsb_quickjs.c`, so quickjs-mode callers keep `js_engine_get_context` meanwhile.
- **Handles** (§15.8): every entry point opens its own `HandleScope`; the engine owns
  exactly two kinds of `Global` (its context and pending rejections) and Resets them in
  `js_engine_shutdown` before the isolate is disposed of.

`tests/smz/test_js_engine_v8.c` (ctest `js_engine_stack`, both modes) checks that a
throw in `foo.js` line 3 reports `foo.js:3` from a file, a string and a SyntaxError; in
v8 mode also the game-relative name and that `js_engine_get_jsb()` sees the engine's
globals. In v8 mode the unchanged `tests/test_js_engine.c` also runs on V8 as ctest
`js_engine_on_v8` (`tests/smz/js_engine_v8.cmake`).

## Tests
`tests/smz/test_jsb_core.c` (ctest `jsb_core`, both engine modes) defines a module from C, calls
every getter, return and throw from JS, and checks values, wrong-type and
missing-argument TypeErrors, the string-size RangeError and module failure.

`tests/smz/test_jsb_buffers.c` (ctest `jsb_buffers`, both engine modes) checks
`jsb_arg_bytes` on ArrayBuffers, TypedArrays and DataViews with byte offsets, the
TypeError for non-buffers and detached buffers, C writes seen by JS and JS writes seen by
C through an external buffer, that its free function runs exactly once after `gc()`
(from `jsb_set_flags("--expose-gc")`) and once more at destroy for a buffer still alive,
and the `jsb_adjust_external_memory` totals.
