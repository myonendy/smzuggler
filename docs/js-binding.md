<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
How native C bindings talk to the JS engine through jsb.h, and the handle rules.
-->
# JS binding: the `jsb` seam

`src/jsb/jsb.h` is the C API that native bindings (`src/bindings/`) use to expose
`__native_*` modules to the shims. It replaces direct QuickJS calls so that Outsider's
bindings move to V8 by near-mechanical rewriting (spec §4.1). It is **not** an engine
abstraction: its one implementation is `src/jsb/jsb_v8.cc`. A transitional QuickJS
backend for the M1 port is planned in DEC-SETUP-008 (card M1-T004).

Status: core API only (M1-T003). ArrayBuffer/TypedArray views and persistent callback
handles (§4.1) come in later cards. The library target is `smz_jsb`, built only with
`-DSMZ_JS_ENGINE=v8`; `jsb.h` itself is plain C11 and includes no C++ or V8 header
(checked by `tests/smz/test_jsb_header.c` in both engine modes).

## API

### Host side
| Call | Purpose |
| --- | --- |
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
* jsb is main-thread only.

## Tests
`tests/smz/test_jsb_core.c` (ctest `jsb_core`, v8 mode) defines a module from C, calls
every getter, return and throw from JS, and checks values, wrong-type and
missing-argument TypeErrors, the string-size RangeError and module failure.
