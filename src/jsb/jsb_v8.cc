// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The smzuggler contributors
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
// so no Local outlives its HandleScope. Only jsb_runtime holds a
// v8::Global (the context), and jsb_runtime_destroy resets it. Main thread
// only.

#include "jsb.h"

#include <libplatform/libplatform.h>
#include <v8.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>

// A JS value handle; lives in its jsb_ctx's deque.
struct jsb_val {
    v8::Local<v8::Value> value;
};

// Per-call state. `info` is NULL for setup callbacks, where there are no
// arguments and no return slot.
struct jsb_ctx {
    v8::Isolate *isolate;
    v8::Local<v8::Context> context;
    const v8::FunctionCallbackInfo<v8::Value> *info;
    std::deque<jsb_val> vals;
    bool threw;
};

struct jsb_runtime {
    v8::Isolate *isolate;
    v8::ArrayBuffer::Allocator *allocator;
    v8::Global<v8::Context> context;
};

namespace {

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

// Throws a new TypeError (RangeError when `range`) with a printf-style
// message, unless the call already threw: the first error is the specific
// one, later ones are usually fallout.
void throw_with(jsb_ctx *ctx, bool range, const char *fmt, va_list ap) {
    if (ctx->threw) return;
    char message[256];
    std::vsnprintf(message, sizeof message, fmt, ap);
    v8::Local<v8::String> text = utf8(ctx->isolate, message);
    ctx->threw = true;
    ctx->isolate->ThrowException(range ? v8::Exception::RangeError(text)
                                       : v8::Exception::TypeError(text));
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
void trampoline(const v8::FunctionCallbackInfo<v8::Value> &info) {
    v8::Isolate *isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);
    jsb_fn fn = reinterpret_cast<jsb_fn>(
        info.Data().As<v8::External>()->Value());
    jsb_ctx ctx{isolate, isolate->GetCurrentContext(), &info, {}, false};
    fn(&ctx);
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

// See jsb.h. ArrayBuffer memory comes from V8's default allocator, which
// `rt` owns and deletes after the isolate.
jsb_runtime *jsb_runtime_create(const char *argv0, const char *icu_data_path) {
    if (!init_v8_once(argv0, icu_data_path)) return nullptr;
    jsb_runtime *rt = new jsb_runtime();
    rt->allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate::CreateParams params;
    params.array_buffer_allocator = rt->allocator;
    rt->isolate = v8::Isolate::New(params);
    if (!make_context(rt)) {
        std::fprintf(stderr, "jsb: cannot create a V8 context\n");
        jsb_runtime_destroy(rt);
        return nullptr;
    }
    return rt;
}

// See jsb.h. Resets the context Global before disposing of the isolate.
void jsb_runtime_destroy(jsb_runtime *rt) {
    if (!rt) return;
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
    jsb_ctx ctx{rt->isolate, context, nullptr, {}, false};
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
    throw_with(ctx, false, fmt, ap);
    va_end(ap);
}

// See jsb.h: jsb_throw_range.
void jsb_throw_range(jsb_ctx *ctx, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    throw_with(ctx, true, fmt, ap);
    va_end(ap);
}

// See jsb.h: jsb_new_object.
jsb_val *jsb_new_object(jsb_ctx *ctx) {
    return wrap(ctx, v8::Object::New(ctx->isolate));
}

// See jsb.h: jsb_set_fn.
bool jsb_set_fn(jsb_ctx *ctx, jsb_val *obj, const char *name, jsb_fn fn) {
    v8::Local<v8::Object> target = as_object(obj);
    if (target.IsEmpty() || !fn) {
        jsb_throw_type(ctx, "jsb_set_fn(%s): bad object or function", name);
        return false;
    }
    v8::Local<v8::External> data = v8::External::New(
        ctx->isolate, reinterpret_cast<void *>(fn));
    v8::Local<v8::Function> func;
    if (!v8::Function::New(ctx->context, trampoline, data).ToLocal(&func))
        return false;
    v8::Local<v8::String> key = utf8(ctx->isolate, name);
    func->SetName(key);
    return target->Set(ctx->context, key, func).FromMaybe(false);
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

}  // extern "C"
