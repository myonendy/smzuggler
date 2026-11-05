// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The smzuggler contributors
// Modified 2026-11-05: remove former engine migration contracts.
// Modified 2026-11-04: the seam calls of card M1-T007b; native calls run
// inside a TryCatch so jsb_clear_exception can drop an error.
// Modified 2026-11-05: share the ArrayBuffer brand check and define native
// functions non-enumerably (card M1-T026b).
// Modified 2026-11-05: install non-constructable functions whose only own
// properties are length and name (card M1-T026c).
//
// V8 implementation of the jsb seam declared in src/jsb/jsb.h.
//
// Why: the C bindings inherited from Outsider must call V8, whose API is
// C++ and template-heavy. This file is the only place that translates
// between the two (spec section 4.1), so no V8 type reaches C code.
// See docs/js-binding.md.
//
// Handle rules (spec section 15.8, docs/js-binding.md): every native call
// and every setup or eval opens its own HandleScope before building a
// jsb_ctx on the stack. The ctx keeps the call's Locals in a deque so that
// jsb_val pointers stay stable; the ctx is destroyed before its scope ends,
// so no Local outlives its HandleScope. Only jsb_runtime holds v8::Globals
// (the context and one per live jsb_callback); jsb_callback_free resets a
// callback's and jsb_runtime_destroy resets them all. Main thread
// only, except ext_release, which V8 may call from a GC helper thread.
// Backing stores: ordinary ArrayBuffers use V8's default allocator (owned by
// jsb_runtime); external buffers wrap the caller's memory and give it back
// through its jsb_free_fn.

#include "jsb.h"

#include <libplatform/libplatform.h>
#include <v8.h>

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <utility>
#include <vector>

// A JS value handle; lives in its jsb_ctx's deque.
struct jsb_val {
    v8::Local<v8::Value> value;
};

// Per-call state. `info` is NULL for setup callbacks, where there are no
// arguments and no return slot. `tc` is the TryCatch around the call, so
// jsb_clear_exception can drop what it caught.
struct jsb_ctx {
    v8::Isolate *isolate;
    v8::Local<v8::Context> context;
    const v8::FunctionCallbackInfo<v8::Value> *info;
    std::deque<jsb_val> vals;
    bool threw;
    v8::TryCatch *tc;
};

// "No free callback slot" in jsb_runtime's free list.
constexpr uint32_t kNoSlot = UINT32_MAX;

// Most arguments jsb_callback_call takes (jsb.h); events pass a few.
constexpr int kMaxCbArgs = 64;

// One jsb_callback slot. A live slot owns the Global to its function (one
// owner, reset on free); a free one links the free list through
// `next_free`. `gen` changes on every reuse, so stale handles fail.
struct CbSlot {
    v8::Global<v8::Function> fn;
    uint32_t gen = 0;
    bool live = false;
    uint32_t next_free = kNoSlot;
};

// The engine instance. The isolate's data slot 0 points back at it, so a
// jsb_ctx can reach it. `external_bytes` is the jsb_adjust_external_memory
// total, kept here because V8's own figure also counts other embedders.
// `cbs` holds the jsb_callback slots (`cb_free` heads the free list) and
// `on_error` the jsb_set_error_handler handler, NULL for the default.
struct jsb_runtime {
    v8::Isolate *isolate;
    v8::ArrayBuffer::Allocator *allocator;
    v8::Global<v8::Context> context;
    int64_t external_bytes;
    std::vector<CbSlot> cbs;
    uint32_t cb_free = kNoSlot;
    jsb_error_fn on_error = nullptr;
    void *error_user = nullptr;
};

namespace {

// What an external buffer's backing-store deleter needs. Owned by the
// backing store and deleted by ext_release.
struct ExtHolder {
    jsb_free_fn free_fn;
    void *user;
};

// The BackingStore deleter of jsb_new_external_buffer: hands the memory
// back through the jsb_free_fn, once. V8 may call it on any thread, during
// GC or isolate disposal, so it touches nothing but `deleter_data`.
void ext_release(void *data, size_t length, void *deleter_data) {
    ExtHolder *holder = static_cast<ExtHolder *>(deleter_data);
    holder->free_fn(data, length, holder->user);
    delete holder;
}

// The process-wide V8 platform. V8 cannot be initialised twice, so it is
// created on the first jsb_runtime_create and kept until exit.
std::unique_ptr<v8::Platform> g_platform;

// Initialises ICU, the platform and V8 once per process. Returns false
// (after printing why) when ICU data cannot be loaded or V8 fails to start.
bool init_v8_once(const char *argv0, const char *icu_data_path) {
    if (g_platform) return true;
    if (!v8::V8::InitializeICUDefaultLocation(argv0, icu_data_path)) {
        std::fprintf(stderr, "jsb: cannot load ICU data from %s\n",
                     icu_data_path ? icu_data_path : "(default)");
        return false;
    }
    g_platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(g_platform.get());
    if (!v8::V8::Initialize()) {
        std::fprintf(stderr, "jsb: V8::Initialize failed\n");
        return false;
    }
    return true;
}

// Makes a call-scoped jsb_val for `value`; the pointer stays valid until
// `ctx` is destroyed (deque push_back never moves existing elements).
jsb_val *wrap(jsb_ctx *ctx, v8::Local<v8::Value> value) {
    ctx->vals.push_back(jsb_val{value});
    return &ctx->vals.back();
}

// Converts NUL-terminated UTF-8 to a JS string. V8 replaces invalid
// sequences with U+FFFD; only a string over V8's maximum length fails,
// which is reported as the empty string.
v8::Local<v8::String> utf8(v8::Isolate *isolate, const char *text) {
    v8::Local<v8::String> s;
    if (!v8::String::NewFromUtf8(isolate, text).ToLocal(&s))
        s = v8::String::Empty(isolate);
    return s;
}

// The error constructors jsb_throw_* use.
enum ErrKind { kErrType, kErrRange, kErrPlain, kErrInternal };

// Makes the error of `kind`. V8 has no InternalError, so that one is an
// Error whose own `name` says so (jsb.h: jsb_throw_internal).
v8::Local<v8::Value> make_error(jsb_ctx *ctx, ErrKind kind,
                                v8::Local<v8::String> text) {
    switch (kind) {
    case kErrRange: return v8::Exception::RangeError(text);
    case kErrPlain: return v8::Exception::Error(text);
    case kErrType: return v8::Exception::TypeError(text);
    case kErrInternal: break;
    }
    v8::Local<v8::Value> err = v8::Exception::Error(text);
    err.As<v8::Object>()
        ->Set(ctx->context, utf8(ctx->isolate, "name"),
              utf8(ctx->isolate, "InternalError"))
        .Check();
    return err;
}

// Throws a new error of `kind` with a printf-style message, unless the
// call already threw: the first error is the specific one, later ones are
// usually fallout.
void throw_with(jsb_ctx *ctx, ErrKind kind, const char *fmt, va_list ap) {
    if (ctx->threw) return;
    char message[256];
    std::vsnprintf(message, sizeof message, fmt, ap);
    v8::Local<v8::String> text = utf8(ctx->isolate, message);
    ctx->threw = true;
    ctx->isolate->ThrowException(make_error(ctx, kind, text));
}

// Returns argument `index`, undefined when missing or outside a call.
v8::Local<v8::Value> arg(jsb_ctx *ctx, int index) {
    if (!ctx->info || index < 0 || index >= ctx->info->Length())
        return v8::Undefined(ctx->isolate);
    return (*ctx->info)[index];
}

// Throws the getters' common TypeError for argument `index`.
void throw_arg_type(jsb_ctx *ctx, int index, const char *expected) {
    jsb_throw_type(ctx, "argument %d must be %s", index + 1, expected);
}

// Returns whether a return value may still be set on this call.
bool can_return(jsb_ctx *ctx) { return ctx->info && !ctx->threw; }

// Returns `val` as an Object, or an empty handle when `val` is NULL or not
// an object, so callers can throw a TypeError instead of crashing.
v8::Local<v8::Object> as_object(jsb_val *val) {
    if (!val || !val->value->IsObject()) return v8::Local<v8::Object>();
    return val->value.As<v8::Object>();
}

// Entry point V8 calls for every jsb_set_fn function. Opens the call's
// HandleScope, builds its jsb_ctx and runs the C function stored in Data.
// A TryCatch holds what the call throws, so jsb_clear_exception can drop
// it; anything still caught at the end is rethrown to the JS caller. A
// termination needs no rethrow: the TryCatch passes it on by itself.
void trampoline(const v8::FunctionCallbackInfo<v8::Value> &info) {
    v8::Isolate *isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::TryCatch tc(isolate);
    jsb_fn fn = reinterpret_cast<jsb_fn>(
        info.Data().As<v8::External>()->Value());
    jsb_ctx ctx{isolate, isolate->GetCurrentContext(), &info, {}, false, &tc};
    fn(&ctx);
    if (tc.HasCaught() && !tc.HasTerminated()) tc.ReThrow();
}

// Writes String(value) as UTF-8 into `out` (truncated, NUL-terminated);
// does nothing if `out` is NULL or `cap` is 0.
void to_buffer(v8::Isolate *isolate, v8::Local<v8::Value> value, char *out,
               size_t cap) {
    if (!out || cap == 0) return;
    v8::String::Utf8Value text(isolate, value);
    const char *s = *text ? *text : "(unconvertible value)";
    size_t n = *text ? static_cast<size_t>(text.length()) : std::strlen(s);
    if (n > cap - 1) n = cap - 1;
    std::memcpy(out, s, n);
    out[n] = '\0';
}

// Compiles and runs `source` in `context`; on success stores the result
// in `result`. On failure `result` is untouched and `tc` holds the error.
bool run_script(v8::Isolate *isolate, v8::Local<v8::Context> context,
                const char *source, const char *file_name,
                v8::Local<v8::Value> *result) {
    v8::ScriptOrigin origin(utf8(isolate, file_name ? file_name : "<eval>"));
    v8::Local<v8::Script> script;
    if (!v8::Script::Compile(context, utf8(isolate, source), &origin)
             .ToLocal(&script))
        return false;
    return script->Run(context).ToLocal(result);
}

}  // namespace

extern "C" {

// Creates the runtime's one context and keeps it in `rt->context`. Opens
// and leaves its own isolate and handle scopes, so the caller may dispose
// of the isolate on failure. Returns false if V8 could not make a context.
static bool make_context(jsb_runtime *rt) {
    v8::Isolate::Scope isolate_scope(rt->isolate);
    v8::HandleScope scope(rt->isolate);
    v8::Local<v8::Context> context = v8::Context::New(rt->isolate);
    if (context.IsEmpty()) return false;
    rt->context.Reset(rt->isolate, context);
    return true;
}

// See jsb.h: jsb_set_flags. V8 freezes its flags in V8::Initialize, so
// they are refused once the platform exists.
bool jsb_set_flags(const char *flags) {
    if (g_platform) {
        std::fprintf(stderr, "jsb: engine flags must be set before the first "
                             "runtime is created\n");
        return false;
    }
    v8::V8::SetFlagsFromString(flags);
    return true;
}

// See jsb.h. ArrayBuffer memory comes from V8's default allocator, which
// `rt` owns and deletes after the isolate.
jsb_runtime *jsb_runtime_create(const char *argv0, const char *icu_data_path) {
    if (!init_v8_once(argv0, icu_data_path)) return nullptr;
    jsb_runtime *rt = new jsb_runtime();
    rt->allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate::CreateParams params;
    params.array_buffer_allocator = rt->allocator;
    rt->isolate = v8::Isolate::New(params);
    rt->isolate->SetData(0, rt);
    if (!make_context(rt)) {
        std::fprintf(stderr, "jsb: cannot create a V8 context\n");
        jsb_runtime_destroy(rt);
        return nullptr;
    }
    return rt;
}

// See jsb.h. Resets the callback and context Globals before disposing of
// the isolate; disposal releases every backing store, so the free functions
// of live external buffers run here.
void jsb_runtime_destroy(jsb_runtime *rt) {
    if (!rt) return;
    rt->cbs.clear();
    rt->context.Reset();
    rt->isolate->Dispose();
    delete rt->allocator;
    delete rt;
}

// See jsb.h: jsb_runtime_setup.
bool jsb_runtime_setup(jsb_runtime *rt, jsb_setup_fn fn, void *user) {
    v8::Isolate::Scope isolate_scope(rt->isolate);
    v8::HandleScope scope(rt->isolate);
    v8::Local<v8::Context> context = rt->context.Get(rt->isolate);
    v8::Context::Scope context_scope(context);
    v8::TryCatch tc(rt->isolate);
    jsb_ctx ctx{rt->isolate, context, nullptr, {}, false, &tc};
    bool ok = fn(&ctx, user);
    if (tc.HasCaught()) {
        char message[512];
        to_buffer(rt->isolate, tc.Exception(), message, sizeof message);
        std::fprintf(stderr, "jsb: setup threw: %s\n", message);
        return false;
    }
    return ok;
}

// See jsb.h: jsb_runtime_eval.
bool jsb_runtime_eval(jsb_runtime *rt, const char *source,
                      const char *file_name, char *out, size_t out_cap) {
    v8::Isolate::Scope isolate_scope(rt->isolate);
    v8::HandleScope scope(rt->isolate);
    v8::Local<v8::Context> context = rt->context.Get(rt->isolate);
    v8::Context::Scope context_scope(context);
    v8::TryCatch tc(rt->isolate);
    v8::Local<v8::Value> result;
    if (run_script(rt->isolate, context, source, file_name, &result)) {
        to_buffer(rt->isolate, result, out, out_cap);
        return true;
    }
    to_buffer(rt->isolate, tc.Exception(), out, out_cap);
    return false;
}

// See jsb.h: jsb_arg_count.
int jsb_arg_count(jsb_ctx *ctx) { return ctx->info ? ctx->info->Length() : 0; }

// See jsb.h: jsb_arg_i32.
bool jsb_arg_i32(jsb_ctx *ctx, int index, int32_t *out) {
    v8::Local<v8::Value> v = arg(ctx, index);
    if (!v->IsNumber()) {
        throw_arg_type(ctx, index, "a number");
        return false;
    }
    // ToInt32 of a Number cannot throw, so FromJust is safe.
    *out = v->Int32Value(ctx->context).FromJust();
    return true;
}

// See jsb.h: jsb_arg_f64.
bool jsb_arg_f64(jsb_ctx *ctx, int index, double *out) {
    v8::Local<v8::Value> v = arg(ctx, index);
    if (!v->IsNumber()) {
        throw_arg_type(ctx, index, "a number");
        return false;
    }
    *out = v.As<v8::Number>()->Value();
    return true;
}

// See jsb.h: jsb_arg_bool.
bool jsb_arg_bool(jsb_ctx *ctx, int index, bool *out) {
    v8::Local<v8::Value> v = arg(ctx, index);
    if (!v->IsBoolean()) {
        throw_arg_type(ctx, index, "a boolean");
        return false;
    }
    *out = v->IsTrue();
    return true;
}

// See jsb.h: jsb_arg_str.
bool jsb_arg_str(jsb_ctx *ctx, int index, char *buf, size_t cap,
                 size_t *len) {
    v8::Local<v8::Value> v = arg(ctx, index);
    if (!v->IsString()) {
        throw_arg_type(ctx, index, "a string");
        return false;
    }
    v8::String::Utf8Value text(ctx->isolate, v);
    size_t n = static_cast<size_t>(text.length());
    if (cap == 0 || n > cap - 1) {
        jsb_throw_range(ctx, "argument %d is %zu bytes, the limit is %zu",
                        index + 1, n, cap ? cap - 1 : 0);
        return false;
    }
    std::memcpy(buf, *text, n);
    buf[n] = '\0';
    if (len) *len = n;
    return true;
}

// See jsb.h: jsb_arg_obj.
jsb_val *jsb_arg_obj(jsb_ctx *ctx, int index) {
    v8::Local<v8::Value> v = arg(ctx, index);
    if (!v->IsObject()) {
        throw_arg_type(ctx, index, "an object");
        return nullptr;
    }
    return wrap(ctx, v);
}

// See jsb.h: jsb_arg_val.
jsb_val *jsb_arg_val(jsb_ctx *ctx, int index) { return wrap(ctx, arg(ctx, index)); }

// See jsb.h: jsb_arg_bytes. A view's Buffer() may move an on-heap typed
// array's elements off the heap once; after that the pointer is stable
// until the buffer is detached or collected, which cannot happen while the
// call holds the argument and runs no JS.
bool jsb_arg_bytes(jsb_ctx *ctx, int index, uint8_t **data, size_t *len) {
    v8::Local<v8::Value> v = arg(ctx, index);
    v8::Local<v8::ArrayBuffer> buffer;
    size_t offset = 0, length = 0;
    if (v->IsArrayBuffer()) {
        buffer = v.As<v8::ArrayBuffer>();
        length = buffer->ByteLength();
    } else if (v->IsArrayBufferView()) {
        v8::Local<v8::ArrayBufferView> view = v.As<v8::ArrayBufferView>();
        buffer = view->Buffer();
        offset = view->ByteOffset();
        length = view->ByteLength();
    } else {
        throw_arg_type(ctx, index, "an ArrayBuffer, TypedArray or DataView");
        return false;
    }
    if (buffer->WasDetached()) {
        jsb_throw_type(ctx, "argument %d is a detached ArrayBuffer or a view "
                            "on one", index + 1);
        return false;
    }
    uint8_t *base = static_cast<uint8_t *>(buffer->Data());
    *data = base ? base + offset : nullptr;
    *len = length;
    return true;
}

// See jsb.h: jsb_arg_is_array_buffer. V8's intrinsic brand test never runs a
// property getter, so a shadowed `byteOffset` cannot change the answer or
// throw; a Proxy of an ArrayBuffer is not itself an ArrayBuffer.
bool jsb_arg_is_array_buffer(jsb_ctx *ctx, int index) {
    return arg(ctx, index)->IsArrayBuffer();
}

// See jsb.h: jsb_ret_undefined.
void jsb_ret_undefined(jsb_ctx *ctx) {
    if (can_return(ctx)) ctx->info->GetReturnValue().SetUndefined();
}

// See jsb.h: jsb_ret_bool.
void jsb_ret_bool(jsb_ctx *ctx, bool value) {
    if (can_return(ctx)) ctx->info->GetReturnValue().Set(value);
}

// See jsb.h: jsb_ret_i32.
void jsb_ret_i32(jsb_ctx *ctx, int32_t value) {
    if (can_return(ctx)) ctx->info->GetReturnValue().Set(value);
}

// See jsb.h: jsb_ret_f64.
void jsb_ret_f64(jsb_ctx *ctx, double value) {
    if (can_return(ctx)) ctx->info->GetReturnValue().Set(value);
}

// See jsb.h: jsb_ret_str.
void jsb_ret_str(jsb_ctx *ctx, const char *utf8_text) {
    if (!utf8_text) {
        jsb_throw_type(ctx, "jsb_ret_str: NULL string");
        return;
    }
    if (can_return(ctx))
        ctx->info->GetReturnValue().Set(utf8(ctx->isolate, utf8_text));
}

// See jsb.h: jsb_ret_val.
void jsb_ret_val(jsb_ctx *ctx, jsb_val *val) {
    if (!can_return(ctx)) return;
    if (val)
        ctx->info->GetReturnValue().Set(val->value);
    else
        ctx->info->GetReturnValue().SetUndefined();
}

// See jsb.h: jsb_throw_type.
void jsb_throw_type(jsb_ctx *ctx, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    throw_with(ctx, kErrType, fmt, ap);
    va_end(ap);
}

// See jsb.h: jsb_throw_range.
void jsb_throw_range(jsb_ctx *ctx, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    throw_with(ctx, kErrRange, fmt, ap);
    va_end(ap);
}

// See jsb.h: jsb_new_object.
jsb_val *jsb_new_object(jsb_ctx *ctx) {
    return wrap(ctx, v8::Object::New(ctx->isolate));
}

// See jsb.h: jsb_set_fn.
bool jsb_set_fn(jsb_ctx *ctx, jsb_val *obj, const char *name, jsb_fn fn) {
    return jsb_set_fn_n(ctx, obj, name, fn, 0);
}

// See jsb.h: jsb_set_fn_n. V8 takes the `length` when making the function.
// kThrow makes the function non-constructable, so unlike a plain V8 function
// it has no `prototype`, `arguments` or `caller` own properties; the inherited binding's
// JS_CFUNC_DEF functions have only `length` and `name` too. `name` is set
// below and `length` is the `length` argument.
bool jsb_set_fn_n(jsb_ctx *ctx, jsb_val *obj, const char *name, jsb_fn fn,
                  int length) {
    if (length < 0 || length > 255) {
        jsb_throw_range(ctx, "jsb_set_fn_n(%s): length %d is not 0..255",
                        name, length);
        return false;
    }
    v8::Local<v8::Object> target = as_object(obj);
    if (target.IsEmpty() || !fn) {
        jsb_throw_type(ctx, "jsb_set_fn(%s): bad object or function", name);
        return false;
    }
    v8::Local<v8::External> data = v8::External::New(
        ctx->isolate, reinterpret_cast<void *>(fn));
    v8::Local<v8::Function> func;
    if (!v8::Function::New(ctx->context, trampoline, data, length,
                           v8::ConstructorBehavior::kThrow)
             .ToLocal(&func))
        return false;
    v8::Local<v8::String> key = utf8(ctx->isolate, name);
    func->SetName(key);
    // Like JS_CFUNC_DEF: writable and configurable, but absent from
    // Object.keys and for-in.
    return target
        ->DefineOwnProperty(ctx->context, key, func,
                            static_cast<v8::PropertyAttribute>(v8::DontEnum))
        .FromMaybe(false);
}

// See jsb.h: jsb_set_prop.
bool jsb_set_prop(jsb_ctx *ctx, jsb_val *obj, const char *name,
                  jsb_val *val) {
    v8::Local<v8::Object> target = as_object(obj);
    if (target.IsEmpty() || !val) {
        jsb_throw_type(ctx, "jsb_set_prop(%s): bad object or value", name);
        return false;
    }
    return target->Set(ctx->context, utf8(ctx->isolate, name), val->value)
        .FromMaybe(false);
}

// See jsb.h: jsb_get_prop.
jsb_val *jsb_get_prop(jsb_ctx *ctx, jsb_val *obj, const char *name) {
    v8::Local<v8::Object> target = as_object(obj);
    if (target.IsEmpty()) {
        jsb_throw_type(ctx, "jsb_get_prop(%s): not an object", name);
        return nullptr;
    }
    v8::Local<v8::Value> v;
    if (!target->Get(ctx->context, utf8(ctx->isolate, name)).ToLocal(&v))
        return nullptr;
    return wrap(ctx, v);
}

// See jsb.h: jsb_define_module.
bool jsb_define_module(jsb_ctx *ctx, const char *name, jsb_module_init init) {
    jsb_val *module = jsb_new_object(ctx);
    if (!init(ctx, module)) return false;
    v8::Local<v8::Object> global = ctx->context->Global();
    return global->Set(ctx->context, utf8(ctx->isolate, name), module->value)
        .FromMaybe(false);
}

// See jsb.h: jsb_new_external_buffer. The backing store wraps the caller's
// allocation (not V8's allocator) and ext_release returns it. V8 counts a
// backing store's bytes as external memory itself, so nothing is adjusted.
jsb_val *jsb_new_external_buffer(jsb_ctx *ctx, void *data, size_t len,
                                 jsb_free_fn free_fn, void *user) {
    if (!data || !free_fn) {
        jsb_throw_type(ctx, "jsb_new_external_buffer: NULL data or free "
                            "function");
        return nullptr;
    }
    if (len > v8::ArrayBuffer::kMaxByteLength) {
        jsb_throw_range(ctx, "jsb_new_external_buffer: %zu bytes is too "
                             "large for an ArrayBuffer", len);
        return nullptr;
    }
    ExtHolder *holder = new ExtHolder{free_fn, user};
    std::shared_ptr<v8::BackingStore> store =
        v8::ArrayBuffer::NewBackingStore(data, len, ext_release, holder);
    return wrap(ctx, v8::ArrayBuffer::New(ctx->isolate, std::move(store)));
}

// See jsb.h: jsb_adjust_external_memory.
int64_t jsb_adjust_external_memory(jsb_ctx *ctx, int64_t change) {
    jsb_runtime *rt = static_cast<jsb_runtime *>(ctx->isolate->GetData(0));
    if (change < -rt->external_bytes) {
        std::fprintf(stderr, "jsb: external memory change %lld would make "
                             "the total of %lld bytes negative\n",
                     static_cast<long long>(change),
                     static_cast<long long>(rt->external_bytes));
        return -1;
    }
    rt->external_bytes += change;
    ctx->isolate->AdjustAmountOfExternalAllocatedMemory(change);
    return rt->external_bytes;
}

}  // extern "C"

// ---- Persistent callbacks -------------------------------------------------

namespace {

// Returns the live slot `cb` names, or NULL after printing that `what` got
// a zero, freed or stale handle.
CbSlot *cb_lookup(jsb_callback cb, const char *what) {
    jsb_runtime *rt = cb.rt;
    if (rt && cb.slot < rt->cbs.size() && rt->cbs[cb.slot].live &&
        rt->cbs[cb.slot].gen == cb.gen)
        return &rt->cbs[cb.slot];
    std::fprintf(stderr, "jsb: %s: zero, freed or stale callback handle\n",
                 what);
    return nullptr;
}

// Returns the index of a free callback slot of `rt`, adding one if none is
// free, or kNoSlot when the table is full.
uint32_t cb_take_slot(jsb_runtime *rt) {
    if (rt->cb_free != kNoSlot) {
        uint32_t i = rt->cb_free;
        rt->cb_free = rt->cbs[i].next_free;
        return i;
    }
    if (rt->cbs.size() >= kNoSlot) return kNoSlot;
    rt->cbs.emplace_back();
    return static_cast<uint32_t>(rt->cbs.size() - 1);
}

// Returns whether `argc`/`argv` are what jsb_callback_call accepts: argc
// in 0..kMaxCbArgs, argv present when needed, and only primitive types,
// strings non-NULL. Prints the first problem.
bool cb_args_ok(int argc, const jsb_cval *argv) {
    if (argc < 0 || argc > kMaxCbArgs || (argc > 0 && !argv)) {
        std::fprintf(stderr, "jsb: jsb_callback_call: bad argc %d or NULL "
                             "argv\n", argc);
        return false;
    }
    for (int i = 0; i < argc; i++) {
        if (argv[i].type > JSB_TYPE_STRING ||
            (argv[i].type == JSB_TYPE_STRING && !argv[i].str)) {
            std::fprintf(stderr, "jsb: jsb_callback_call: argument %d has a "
                                 "bad type or a NULL string\n", i + 1);
            return false;
        }
    }
    return true;
}

// Converts a C argument (checked by cb_args_ok) to a JS value in the
// current HandleScope.
v8::Local<v8::Value> cb_arg_value(v8::Isolate *isolate,
                                         const jsb_cval &v) {
    switch (v.type) {
    case JSB_TYPE_NULL: return v8::Null(isolate);
    case JSB_TYPE_BOOL: return v8::Boolean::New(isolate, v.b);
    case JSB_TYPE_NUMBER: return v8::Number::New(isolate, v.num);
    case JSB_TYPE_STRING: return utf8(isolate, v.str);
    default: return v8::Undefined(isolate);
    }
}

// Fills `ret` (may be NULL) from the callback's result. A String is copied
// into the caller's buffer (truncated, NUL-terminated) with its full length.
void cb_result(v8::Isolate *isolate, v8::Local<v8::Value> value,
                      jsb_result *ret) {
    if (!ret) return;
    ret->str_len = 0;
    if (value->IsUndefined()) {
        ret->type = JSB_TYPE_UNDEFINED;
    } else if (value->IsNull()) {
        ret->type = JSB_TYPE_NULL;
    } else if (value->IsBoolean()) {
        ret->type = JSB_TYPE_BOOL;
        ret->b = value->IsTrue();
    } else if (value->IsNumber()) {
        ret->type = JSB_TYPE_NUMBER;
        ret->num = value.As<v8::Number>()->Value();
    } else if (value->IsString()) {
        ret->type = JSB_TYPE_STRING;
        v8::String::Utf8Value text(isolate, value);
        ret->str_len = *text ? static_cast<size_t>(text.length()) : 0;
        if (ret->str && ret->str_cap > 0) {
            size_t k = std::min(ret->str_len, ret->str_cap - 1);
            if (k) std::memcpy(ret->str, *text, k);
            ret->str[k] = '\0';
        }
    } else {
        ret->type = JSB_TYPE_OBJECT;
    }
}

// Gives the exception caught by `tc` (its String() form and its `stack`,
// or "") to the runtime's error handler, or prints them on stderr.
void cb_report(jsb_runtime *rt, v8::Local<v8::Context> context,
                      v8::TryCatch &tc) {
    char message[512], stack[2048];
    to_buffer(rt->isolate, tc.Exception(), message, sizeof message);
    stack[0] = '\0';
    v8::Local<v8::Value> st;
    if (tc.StackTrace(context).ToLocal(&st) && st->IsString())
        to_buffer(rt->isolate, st, stack, sizeof stack);
    if (rt->on_error)
        rt->on_error(message, stack, rt->error_user);
    else
        std::fprintf(stderr, "jsb: callback threw: %s\n%s\n", message, stack);
}

}  // namespace

extern "C" {

// See jsb.h: jsb_set_error_handler.
void jsb_set_error_handler(jsb_runtime *rt, jsb_error_fn fn, void *user) {
    rt->on_error = fn;
    rt->error_user = fn ? user : nullptr;
}

// See jsb.h: jsb_callback_new. The slot's Global is the one owner of the
// function's persistent reference.
bool jsb_callback_new(jsb_ctx *ctx, jsb_val *fn, jsb_callback *out) {
    *out = jsb_callback{};
    if (!fn || !fn->value->IsFunction()) {
        jsb_throw_type(ctx, "jsb_callback_new: not a function");
        return false;
    }
    jsb_runtime *rt = static_cast<jsb_runtime *>(ctx->isolate->GetData(0));
    uint32_t i = cb_take_slot(rt);
    if (i == kNoSlot) {
        jsb_throw_range(ctx, "jsb_callback_new: out of memory");
        return false;
    }
    CbSlot &slot = rt->cbs[i];
    slot.fn.Reset(ctx->isolate, fn->value.As<v8::Function>());
    slot.gen = slot.gen == UINT32_MAX ? 1 : slot.gen + 1;
    slot.live = true;
    *out = jsb_callback{rt, i, slot.gen};
    return true;
}

// See jsb.h: jsb_callback_call. The function is taken into a Local before
// the call, so it survives the callback freeing its own handle; the slot is
// not touched after JS ran (JS may grow the table). The TryCatch keeps the
// exception from reaching C or an enclosing JS frame.
jsb_status jsb_callback_call(jsb_callback cb, int argc, const jsb_cval *argv,
                             jsb_result *ret) {
    if (ret) ret->type = JSB_TYPE_UNDEFINED;
    CbSlot *slot = cb_lookup(cb, "jsb_callback_call");
    if (!slot) return JSB_ERR_HANDLE;
    if (!cb_args_ok(argc, argv)) return JSB_ERR_ARG;
    v8::Isolate *isolate = cb.rt->isolate;
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = cb.rt->context.Get(isolate);
    v8::Context::Scope context_scope(context);
    v8::Local<v8::Function> fn = slot->fn.Get(isolate);
    v8::Local<v8::Value> args[kMaxCbArgs];
    for (int i = 0; i < argc; i++) args[i] = cb_arg_value(isolate, argv[i]);
    v8::TryCatch tc(isolate);
    v8::Local<v8::Value> result;
    if (!fn->Call(context, v8::Undefined(isolate), argc, args)
             .ToLocal(&result)) {
        cb_report(cb.rt, context, tc);
        return JSB_ERR_THREW;
    }
    cb_result(isolate, result, ret);
    return JSB_OK;
}

// See jsb.h: jsb_callback_free. Resetting the Global drops jsb's reference;
// the function is collected by a later GC once JS holds none either.
bool jsb_callback_free(jsb_callback *cb) {
    CbSlot *slot = cb_lookup(*cb, "jsb_callback_free");
    jsb_runtime *rt = cb->rt;
    uint32_t index = cb->slot;
    *cb = jsb_callback{};
    if (!slot) return false;
    slot->fn.Reset();
    slot->live = false;
    slot->next_free = rt->cb_free;
    rt->cb_free = index;
    return true;
}

}  // extern "C"

// ---- Values, coercion and errors (card M1-T007b) ------------------------

namespace {

// Wraps `value` when V8 produced one; otherwise records the pending
// exception as the call's throw and returns NULL.
jsb_val *wrap_maybe(jsb_ctx *ctx, v8::MaybeLocal<v8::Value> value) {
    v8::Local<v8::Value> v;
    if (value.ToLocal(&v)) return wrap(ctx, v);
    ctx->threw = true;
    return nullptr;
}

// Stores a Maybe result of a V8 conversion in `*out`, or records its
// pending exception as the call's throw. Returns whether it converted.
template <typename T, typename U>
bool take(jsb_ctx *ctx, v8::Maybe<U> result, T *out) {
    U value;
    if (!result.To(&value)) {
        ctx->threw = true;
        return false;
    }
    *out = static_cast<T>(value);
    return true;
}

// Throws the jsb_val_to_* TypeError for a NULL handle; returns false.
bool null_val(jsb_ctx *ctx, const char *who) {
    jsb_throw_type(ctx, "%s: NULL value", who);
    return false;
}

}  // namespace

extern "C" {

// See jsb.h: jsb_new_i32.
jsb_val *jsb_new_i32(jsb_ctx *ctx, int32_t value) {
    return wrap(ctx, v8::Integer::New(ctx->isolate, value));
}

// See jsb.h: jsb_new_u32.
jsb_val *jsb_new_u32(jsb_ctx *ctx, uint32_t value) {
    return wrap(ctx, v8::Integer::NewFromUnsigned(ctx->isolate, value));
}

// See jsb.h: jsb_new_f64.
jsb_val *jsb_new_f64(jsb_ctx *ctx, double value) {
    return wrap(ctx, v8::Number::New(ctx->isolate, value));
}

// See jsb.h: jsb_new_bool.
jsb_val *jsb_new_bool(jsb_ctx *ctx, bool value) {
    return wrap(ctx, v8::Boolean::New(ctx->isolate, value));
}

// See jsb.h: jsb_new_null.
jsb_val *jsb_new_null(jsb_ctx *ctx) { return wrap(ctx, v8::Null(ctx->isolate)); }

// See jsb.h: jsb_new_str.
jsb_val *jsb_new_str(jsb_ctx *ctx, const char *utf8_text) {
    if (!utf8_text) {
        jsb_throw_type(ctx, "jsb_new_str: NULL string");
        return nullptr;
    }
    return jsb_new_str_len(ctx, utf8_text, std::strlen(utf8_text));
}

// See jsb.h: jsb_new_str_len. A string over V8's length limit is a
// RangeError, like the inherited binding's.
jsb_val *jsb_new_str_len(jsb_ctx *ctx, const char *utf8_text, size_t len) {
    if (!utf8_text && len > 0) {
        jsb_throw_type(ctx, "jsb_new_str_len: NULL string");
        return nullptr;
    }
    v8::Local<v8::String> s;
    if (len > INT32_MAX ||
        !v8::String::NewFromUtf8(ctx->isolate, utf8_text ? utf8_text : "",
                                 v8::NewStringType::kNormal,
                                 static_cast<int>(len))
             .ToLocal(&s)) {
        jsb_throw_range(ctx, "jsb_new_str_len: %zu bytes is too long", len);
        return nullptr;
    }
    return wrap(ctx, s);
}

// See jsb.h: jsb_new_array.
jsb_val *jsb_new_array(jsb_ctx *ctx) {
    return wrap(ctx, v8::Array::New(ctx->isolate));
}

// See jsb.h: jsb_set_index.
bool jsb_set_index(jsb_ctx *ctx, jsb_val *obj, uint32_t index, jsb_val *val) {
    v8::Local<v8::Object> target = as_object(obj);
    if (target.IsEmpty() || !val) {
        jsb_throw_type(ctx, "jsb_set_index(%u): bad object or value", index);
        return false;
    }
    bool ok = false;
    return take(ctx, target->Set(ctx->context, index, val->value), &ok) && ok;
}

// See jsb.h: jsb_new_buffer_copy. V8's allocator owns the copy.
jsb_val *jsb_new_buffer_copy(jsb_ctx *ctx, const void *data, size_t len) {
    if (!data && len > 0) {
        jsb_throw_type(ctx, "jsb_new_buffer_copy: NULL data");
        return nullptr;
    }
    if (len > INT32_MAX) {
        jsb_throw_range(ctx, "jsb_new_buffer_copy: %zu bytes is too large "
                             "for an ArrayBuffer", len);
        return nullptr;
    }
    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(ctx->isolate, len);
    if (len > 0) std::memcpy(ab->GetBackingStore()->Data(), data, len);
    return wrap(ctx, ab);
}

// See jsb.h: jsb_arg_truthy.
bool jsb_arg_truthy(jsb_ctx *ctx, int index) {
    return arg(ctx, index)->BooleanValue(ctx->isolate);
}

// See jsb.h: jsb_arg_to_i32.
bool jsb_arg_to_i32(jsb_ctx *ctx, int index, int32_t *out) {
    return take(ctx, arg(ctx, index)->Int32Value(ctx->context), out);
}

// See jsb.h: jsb_arg_to_u32.
bool jsb_arg_to_u32(jsb_ctx *ctx, int index, uint32_t *out) {
    return take(ctx, arg(ctx, index)->Uint32Value(ctx->context), out);
}

// See jsb.h: jsb_arg_to_f64.
bool jsb_arg_to_f64(jsb_ctx *ctx, int index, double *out) {
    return take(ctx, arg(ctx, index)->NumberValue(ctx->context), out);
}

// See jsb.h: jsb_arg_to_str. ToString first (which may throw), then UTF-8.
char *jsb_arg_to_str(jsb_ctx *ctx, int index, size_t *len) {
    v8::Local<v8::String> s;
    if (!arg(ctx, index)->ToString(ctx->context).ToLocal(&s)) {
        ctx->threw = true;
        return nullptr;
    }
    v8::String::Utf8Value text(ctx->isolate, s);
    size_t n = static_cast<size_t>(text.length());
    char *copy = static_cast<char *>(std::malloc(n + 1));
    if (!copy) {
        jsb_throw_internal(ctx, "out of memory");
        return nullptr;
    }
    if (n > 0) std::memcpy(copy, *text, n);
    copy[n] = '\0';
    if (len) *len = n;
    return copy;
}

// See jsb.h: jsb_free_str.
void jsb_free_str(char *str) { std::free(str); }

// See jsb.h: jsb_is_array.
bool jsb_is_array(jsb_ctx *ctx, jsb_val *val) {
    (void)ctx;
    return val && val->value->IsArray();
}

// See jsb.h: jsb_get_index.
jsb_val *jsb_get_index(jsb_ctx *ctx, jsb_val *obj, uint32_t index) {
    v8::Local<v8::Object> target = as_object(obj);
    if (target.IsEmpty()) {
        jsb_throw_type(ctx, "jsb_get_index(%u): not an object", index);
        return nullptr;
    }
    return wrap_maybe(ctx, target->Get(ctx->context, index));
}

// See jsb.h: jsb_val_to_f64.
bool jsb_val_to_f64(jsb_ctx *ctx, jsb_val *val, double *out) {
    if (!val) return null_val(ctx, "jsb_val_to_f64");
    return take(ctx, val->value->NumberValue(ctx->context), out);
}

// See jsb.h: jsb_val_to_i32.
bool jsb_val_to_i32(jsb_ctx *ctx, jsb_val *val, int32_t *out) {
    if (!val) return null_val(ctx, "jsb_val_to_i32");
    return take(ctx, val->value->Int32Value(ctx->context), out);
}

// See jsb.h: jsb_val_to_u32.
bool jsb_val_to_u32(jsb_ctx *ctx, jsb_val *val, uint32_t *out) {
    if (!val) return null_val(ctx, "jsb_val_to_u32");
    return take(ctx, val->value->Uint32Value(ctx->context), out);
}

// See jsb.h: jsb_val_type.
jsb_type jsb_val_type(jsb_ctx *ctx, jsb_val *val) {
    (void)ctx;
    if (!val || val->value->IsUndefined()) return JSB_TYPE_UNDEFINED;
    if (val->value->IsNull()) return JSB_TYPE_NULL;
    if (val->value->IsBoolean()) return JSB_TYPE_BOOL;
    if (val->value->IsNumber()) return JSB_TYPE_NUMBER;
    if (val->value->IsString()) return JSB_TYPE_STRING;
    return JSB_TYPE_OBJECT;
}

// See jsb.h: jsb_clear_exception. Resets the call's TryCatch, except for a
// termination, which must keep unwinding.
void jsb_clear_exception(jsb_ctx *ctx) {
    if (ctx->tc && ctx->tc->HasTerminated()) return;
    if (ctx->tc) ctx->tc->Reset();
    ctx->threw = false;
}

// See jsb.h: jsb_throw_error.
void jsb_throw_error(jsb_ctx *ctx, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    throw_with(ctx, kErrPlain, fmt, ap);
    va_end(ap);
}

// See jsb.h: jsb_throw_internal.
void jsb_throw_internal(jsb_ctx *ctx, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    throw_with(ctx, kErrInternal, fmt, ap);
    va_end(ap);
}

// See jsb.h: jsb_engine_version.
const char *jsb_engine_version(void) { return v8::V8::GetVersion(); }

}  // extern "C"
