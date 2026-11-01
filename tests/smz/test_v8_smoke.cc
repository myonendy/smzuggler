// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The smzuggler contributors
//
// Smoke test for the prebuilt V8 imported by cmake/V8.cmake as smz::v8.
//
// Why: it is the first code that compiles against the V8 headers, links the
// monolith and runs it, so it catches define/ABI mismatches between the host
// build and tools/v8/build_v8.sh, and a linked V8 that is not the pin.
// See docs/build-v8.md and spec section 4.1.
//
// Handle rules (spec section 15.8, docs js-binding.md): every Local lives in
// a HandleScope opened in the function that uses it; no Local is stored
// anywhere. Only this (main) thread enters the isolate.

#include <libplatform/libplatform.h>
#include <v8.h>

#include <cstdio>
#include <cstring>
#include <memory>

namespace {

// Reports a failed check and returns false, so main() can stop at the first
// failure with context. `what` is a static description; nothing is kept.
bool fail(const char *what) {
    std::fprintf(stderr, "v8_smoke: FAIL: %s\n", what);
    return false;
}

// Compiles and runs `1+1` in a fresh context of `isolate` and checks that
// the result is the int32 2. Opens its own HandleScope (section 15.8), so
// every handle it creates dies on return. Main thread only.
bool run_one_plus_one(v8::Isolate *isolate) {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    if (context.IsEmpty()) return fail("Context::New returned empty");
    v8::Context::Scope context_scope(context);
    v8::Local<v8::String> source =
        v8::String::NewFromUtf8Literal(isolate, "1+1");
    v8::Local<v8::Script> script;
    if (!v8::Script::Compile(context, source).ToLocal(&script))
        return fail("compiling 1+1");
    v8::Local<v8::Value> result;
    if (!script->Run(context).ToLocal(&result)) return fail("running 1+1");
    if (!result->IsInt32()) return fail("1+1 is not an int32");
    int32_t value = result.As<v8::Int32>()->Value();
    if (value != 2) {
        std::fprintf(stderr, "v8_smoke: FAIL: 1+1 returned %d\n", value);
        return false;
    }
    std::printf("v8_smoke: 1+1 = %d\n", value);
    return true;
}

// Checks that the linked V8 reports the pinned version. GetVersion() returns
// a static string owned by V8.
bool check_version() {
    const char *linked = v8::V8::GetVersion();
    std::printf("v8_smoke: V8 %s (pin %s)\n", linked, SMZ_V8_EXPECTED_VERSION);
    if (std::strcmp(linked, SMZ_V8_EXPECTED_VERSION) != 0)
        return fail("linked V8 version differs from the pin");
    return true;
}

}  // namespace

// Initialises ICU and the platform, creates one isolate, runs the checks and
// tears everything down in reverse order. Exit status 0 means every check
// passed; 1 means at least one failed (the reason is on stderr).
int main(int argc, char **argv) {
    (void)argc;
    if (!v8::V8::InitializeICUDefaultLocation(argv[0], SMZ_V8_ICU_DATA_PATH)) {
        fail("loading ICU data from " SMZ_V8_ICU_DATA_PATH);
        return 1;
    }
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    if (!v8::V8::Initialize()) {
        fail("V8::Initialize");
        return 1;
    }

    v8::Isolate::CreateParams params;
    params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate *isolate = v8::Isolate::New(params);
    bool ok = check_version();
    ok = run_one_plus_one(isolate) && ok;
    isolate->Dispose();
    delete params.array_buffer_allocator;

    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    if (ok) std::printf("v8_smoke: PASS\n");
    return ok ? 0 : 1;
}
