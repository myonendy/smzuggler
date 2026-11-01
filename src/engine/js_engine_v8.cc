// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The smzuggler contributors
//
// The engine API of src/engine/js_engine.h implemented on V8 (spec section
// 4.1, item 3). Built instead of js_engine.c when SMZ_JS_ENGINE=v8.
//
// Why: script_loader.c and main.c talk to the JS engine only through
// js_engine.h, so the move from QuickJS to V8 swaps this file in and leaves
// them alone. It stands in for the parts of NW.js's Chromium that run page
// scripts: one isolate and one context (the page's window), classic-script
// evaluation with a ScriptOrigin naming the file so stack traces read
// `file.js:line:col` as in NW.js DevTools, `console`, and the microtask
// checkpoint the game loop calls after each macrotask. The isolate and
// context come from the jsb seam (src/jsb/jsb.h), so bindings defined
// through js_engine_get_jsb() share the game's global object. See
// docs/js-binding.md, "js_engine on V8".
//
// Rules:
// - Main thread only. Only the main thread enters the isolate.
// - Handle lifetimes follow spec section 15.8 and docs/js-binding.md,
//   "Handle rules": every entry point opens its own HandleScope, no Local is
//   stored, and the JSEngine owns its Globals (the context and pending
//   rejections) and Resets them in js_engine_shutdown.
// - Isolate data slot 0 belongs to jsb; this file uses slot 1 for its
//   JSEngine, so isolate-wide callbacks can find it.

#include "engine/js_engine.h"

#include "jsb.h"

#include <v8.h>

#include <sys/stat.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#endif

#ifndef SMZ_V8_ICU_DATA_PATH
#error "SMZ_V8_ICU_DATA_PATH must name icudtl.dat (set by CMakeLists.txt)"
#endif

namespace {

// Isolate data slot holding the JSEngine (slot 0 is jsb's runtime).
constexpr uint32_t kEngineSlot = 1;

// First bytes of a V8 .jsc file, after the source mtime. A QuickJS .jsc left
// by a quickjs-mode build lacks it and is simply recompiled and replaced.
constexpr char kJscMagic[8] = {'S', 'M', 'Z', 'V', '8', 'J', 'C', '1'};

// A rejected promise with no handler yet, held until the next checkpoint.
struct Rejection {
    v8::Global<v8::Promise> promise;
    v8::Global<v8::Value> reason;
};

}  // namespace

// The engine instance. `rt` owns the isolate and its one context; `context`
// is this file's own Global to it, Reset before `rt` is destroyed.
struct JSEngine {
    jsb_runtime *rt = nullptr;
    v8::Isolate *isolate = nullptr;
    v8::Global<v8::Context> context;
    JSConsoleCallback console_cb = nullptr;
    void *console_userdata = nullptr;
    // Unhandled rejections, reported after each checkpoint as QuickJS mode
    // does (when a browser fires `unhandledrejection`).
    std::vector<Rejection> rejections;
    // Promise reaction jobs run so far, counted by the promise hook.
    int jobs_run = 0;
    // Set by the message listener when a microtask threw uncaught.
    bool job_failed = false;
    // Watchdog settings; see js_engine_set_watchdog.
    unsigned watchdog_ms = 0;
    // .jsc location (empty = beside each script) and the game root that
    // keys it, as in js_engine.c.
    std::string cache_dir;
    std::string cache_game_root;
};

namespace {

// Returns the JSEngine stored in `isolate`'s slot. Isolate callbacks only.
JSEngine *engine_of(v8::Isolate *isolate) {
    return static_cast<JSEngine *>(isolate->GetData(kEngineSlot));
}

// Makes a V8 string from UTF-8 `text` (borrowed); the empty string if V8
// refuses it (longer than String::kMaxLength).
v8::Local<v8::String> v8_str(v8::Isolate *isolate, const char *text,
                             int len = -1) {
    v8::Local<v8::String> out;
    if (!v8::String::NewFromUtf8(isolate, text, v8::NewStringType::kNormal, len)
             .ToLocal(&out))
        return v8::String::Empty(isolate);
    return out;
}

// Returns String(value) as UTF-8. A toString that throws yields "" and its
// exception is swallowed, so reporting an error never raises another.
std::string to_utf8(v8::Isolate *isolate, v8::Local<v8::Value> value) {
    v8::TryCatch tc(isolate);
    v8::String::Utf8Value text(isolate, value);
    return *text ? std::string(*text, text.length()) : std::string();
}

// Sends one line of output to the console callback, or to stdout/stderr
// with QuickJS mode's prefixes. `level` is "log", "warn" or "error".
void emit(JSEngine *engine, const char *level, const std::string &text) {
    if (engine->console_cb) {
        engine->console_cb(level, text.c_str(), engine->console_userdata);
    } else if (std::strcmp(level, "warn") == 0) {
        std::fprintf(stdout, "[WARN] %s\n", text.c_str());
    } else if (std::strcmp(level, "error") == 0) {
        std::fprintf(stderr, "[ERROR] %s\n", text.c_str());
    } else {
        std::fprintf(stdout, "%s\n", text.c_str());
    }
}

// Joins console arguments with spaces, as Chromium's console does for
// non-format arguments. Implements console.log/warn/error (compat
// "console", https://console.spec.whatwg.org/#logging); `%` format
// specifiers are not expanded, as in QuickJS mode.
void console_emit(const v8::FunctionCallbackInfo<v8::Value> &info,
                  const char *level) {
    v8::HandleScope scope(info.GetIsolate());
    JSEngine *engine = static_cast<JSEngine *>(info.Data().As<v8::External>()->Value());
    std::string message;
    for (int i = 0; i < info.Length(); i++) {
        if (i > 0) message += ' ';
        message += to_utf8(info.GetIsolate(), info[i]);
    }
    emit(engine, level, message);
}

// console.log: see console_emit.
void console_log(const v8::FunctionCallbackInfo<v8::Value> &info) {
    console_emit(info, "log");
}

// console.warn: see console_emit.
void console_warn(const v8::FunctionCallbackInfo<v8::Value> &info) {
    console_emit(info, "warn");
}

// console.error: see console_emit.
void console_error(const v8::FunctionCallbackInfo<v8::Value> &info) {
    console_emit(info, "error");
}

// Defines global `console` with log, warn and error bound to `engine`.
// Returns false if V8 refused a property (out of memory).
bool install_console(JSEngine *engine, v8::Local<v8::Context> context) {
    v8::Isolate *isolate = engine->isolate;
    v8::Local<v8::External> data = v8::External::New(isolate, engine);
    v8::Local<v8::Object> console = v8::Object::New(isolate);
    struct { const char *name; v8::FunctionCallback fn; } methods[] = {
        {"log", console_log}, {"warn", console_warn}, {"error", console_error}};
    for (const auto &m : methods) {
        v8::Local<v8::Function> fn;
        if (!v8::Function::New(context, m.fn, data).ToLocal(&fn) ||
            !console->Set(context, v8_str(isolate, m.name), fn).FromMaybe(false))
            return false;
    }
    return context->Global()
        ->Set(context, v8_str(isolate, "console"), console)
        .FromMaybe(false);
}

// Reports a caught exception the way js_engine.c does: its String() form,
// then its `stack`. V8's SyntaxError stack has no frame, so the location
// from the message is added in stack-frame form (`    at file:line:col`).
void report_exception(JSEngine *engine, v8::Local<v8::Context> context,
                      const v8::TryCatch &tc) {
    v8::Isolate *isolate = engine->isolate;
    v8::Local<v8::Value> exc = tc.Exception();
    emit(engine, "error", to_utf8(isolate, exc));
    if (!exc->IsNativeError()) return;
    v8::Local<v8::Value> stack;
    if (!exc.As<v8::Object>()->Get(context, v8_str(isolate, "stack")).ToLocal(&stack) ||
        !stack->IsString())
        return;
    std::string text = to_utf8(isolate, stack);
    v8::Local<v8::Message> msg = tc.Message();
    if (text.find('\n') == std::string::npos && !msg.IsEmpty()) {
        text += "\n    at " + to_utf8(isolate, msg->GetScriptResourceName()) + ":" +
                std::to_string(msg->GetLineNumber(context).FromMaybe(0)) + ":" +
                std::to_string(msg->GetStartColumn(context).FromMaybe(0) + 1);
    }
    emit(engine, "error", text);
}

// Counts promise reaction jobs for js_engine_execute_pending_jobs: V8 runs
// microtasks without telling the embedder how many ran. queueMicrotask
// callbacks are not counted (they are not promise jobs).
void promise_hook(v8::PromiseHookType type, v8::Local<v8::Promise>,
                  v8::Local<v8::Value>) {
    if (type == v8::PromiseHookType::kBefore)
        engine_of(v8::Isolate::GetCurrent())->jobs_run++;
}

// Tracks rejections without a handler, and retracts them when a handler is
// attached later, so only still-unhandled ones are reported after the
// checkpoint (HTML "unhandled promise rejections").
void on_promise_reject(v8::PromiseRejectMessage message) {
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    JSEngine *engine = engine_of(isolate);
    v8::Local<v8::Promise> promise = message.GetPromise();
    if (message.GetEvent() == v8::kPromiseRejectWithNoHandler) {
        Rejection r;
        r.promise.Reset(isolate, promise);
        r.reason.Reset(isolate, message.GetValue());
        engine->rejections.push_back(std::move(r));
    } else if (message.GetEvent() == v8::kPromiseHandlerAddedAfterReject) {
        auto &list = engine->rejections;
        for (size_t i = 0; i < list.size(); i++) {
            if (list[i].promise == promise) {
                list.erase(list.begin() + static_cast<std::ptrdiff_t>(i));
                return;
            }
        }
    }
}

// Reports an exception no TryCatch caught: during a microtask checkpoint
// that is a queueMicrotask callback that threw. Marks the drain as failed.
void on_uncaught(v8::Local<v8::Message> message, v8::Local<v8::Value> error) {
    v8::Isolate *isolate = v8::Isolate::GetCurrent();
    JSEngine *engine = engine_of(isolate);
    engine->job_failed = true;
    emit(engine, "error", to_utf8(isolate, error));
    emit(engine, "error", "    at " + to_utf8(isolate, message->GetScriptResourceName()) +
                              ":" + std::to_string(message->GetLineNumber(
                                        isolate->GetCurrentContext()).FromMaybe(0)));
}

// jsb_runtime_setup callback of js_engine_init: the runtime's isolate and
// context are entered here, which is the only way jsb exposes them. Keeps
// the context, installs console and the isolate callbacks.
bool adopt_runtime(jsb_ctx *, void *user) {
    JSEngine *engine = static_cast<JSEngine *>(user);
    engine->isolate = v8::Isolate::GetCurrent();
    v8::Local<v8::Context> context = engine->isolate->GetCurrentContext();
    engine->context.Reset(engine->isolate, context);
    engine->isolate->SetData(kEngineSlot, engine);
    engine->isolate->SetMicrotasksPolicy(v8::MicrotasksPolicy::kExplicit);
    engine->isolate->SetPromiseHook(promise_hook);
    engine->isolate->SetPromiseRejectCallback(on_promise_reject);
    engine->isolate->AddMessageListener(on_uncaught);
    return install_console(engine, context);
}

// Compiles and runs `source` (borrowed, `len` bytes of UTF-8) as a classic
// script named `name`, line and column offsets 0, so V8's 1-based stack
// positions match the file. `cache` (ownership taken, may be NULL) is a V8
// code cache to consume; when `cache_out` is not NULL a fresh code cache of
// the compiled script is stored there (caller deletes). The result goes to
// `result_out` when not NULL. Errors are reported; returns false on them.
bool run_source(JSEngine *engine, const char *source, size_t len, const char *name,
                v8::ScriptCompiler::CachedData *cache,
                v8::ScriptCompiler::CachedData **cache_out, std::string *result_out) {
    v8::Isolate *isolate = engine->isolate;
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = engine->context.Get(isolate);
    v8::Context::Scope context_scope(context);
    v8::TryCatch tc(isolate);
    v8::ScriptOrigin origin(v8_str(isolate, name), 0, 0);
    v8::ScriptCompiler::Source src(v8_str(isolate, source, static_cast<int>(len)),
                                   origin, cache);
    v8::Local<v8::Script> script;
    v8::Local<v8::Value> result;
    if (!v8::ScriptCompiler::Compile(context, &src,
                                     cache ? v8::ScriptCompiler::kConsumeCodeCache
                                           : v8::ScriptCompiler::kNoCompileOptions)
             .ToLocal(&script)) {
        report_exception(engine, context, tc);
        return false;
    }
    if (cache_out)
        *cache_out = v8::ScriptCompiler::CreateCodeCache(script->GetUnboundScript());
    if (!script->Run(context).ToLocal(&result)) {
        report_exception(engine, context, tc);
        return false;
    }
    if (result_out) *result_out = to_utf8(isolate, result);
    return true;
}

// Reads a whole file. Returns false if it cannot be opened or read.
bool read_file(const char *path, std::string *out) {
    FILE *f = std::fopen(path, "rb");
    if (!f) return false;
    out->clear();
    char buf[16384];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) out->append(buf, n);
    bool ok = !std::ferror(f);
    std::fclose(f);
    return ok;
}

// Writes `data` to `path`, replacing it. Returns false on any I/O error.
bool write_file(const std::string &path, const std::string &data) {
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
    return std::fclose(f) == 0 && ok;
}

// Returns the name a script gets in stack traces: its path relative to the
// game root when one is known and the path lies inside it (NW.js shows
// game-relative URLs), else the path as given.
std::string origin_name(const JSEngine *engine, const char *filepath) {
    const std::string &root = engine->cache_game_root;
    size_t n = root.size();
    if (n && std::strncmp(filepath, root.c_str(), n) == 0 &&
        (filepath[n] == '/' || filepath[n] == '\\'))
        return std::string(filepath + n + 1);
    return std::string(filepath);
}

// True for a path separator on any platform.
bool is_sep(char c) { return c == '/' || c == '\\'; }

// Returns `s` without trailing separators (a lone "/" is kept).
std::string trimmed(const char *s) {
    size_t len = std::strlen(s);
    while (len > 1 && is_sep(s[len - 1])) len--;
    return std::string(s, len);
}

// The cache key of `filepath`: relative to `root` when inside it, else the
// whole path, with drive colons dropped and ".." segments turned into "__"
// so the key cannot leave the cache dir. Same rule as js_engine.c, so both
// modes put a script's .jsc at the same path.
std::string cache_key(const std::string &root, const char *filepath) {
    const char *rel = filepath;
    size_t rlen = root.size();
    if (rlen && std::strncmp(filepath, root.c_str(), rlen) == 0 && is_sep(filepath[rlen]))
        rel = filepath + rlen;
    while (is_sep(*rel)) rel++;
    std::string key;
    bool seg_start = true;
    for (const char *p = rel; *p; p++) {
        if (seg_start && p[0] == '.' && p[1] == '.' && (!p[2] || is_sep(p[2]))) {
            key += "__";
            p++;
            seg_start = false;
        } else if (is_sep(*p)) {
            if (!seg_start) key += '/';
            seg_start = true;
        } else {
            key += (*p == ':') ? '_' : *p;
            seg_start = false;
        }
    }
    return key;
}

// Creates every missing parent directory of `path`, reporting failures on
// stderr (the later write then fails and the script runs uncached).
void make_parent_dirs(const std::string &path) {
    for (size_t i = 1; i < path.size(); i++) {
        if (!is_sep(path[i]) || is_sep(path[i - 1]) || path[i - 1] == ':') continue;
        std::string dir = path.substr(0, i);
        struct stat st;
        if (stat(dir.c_str(), &st) == 0) continue;
#ifdef _WIN32
        int rc = _mkdir(dir.c_str());
#else
        int rc = mkdir(dir.c_str(), 0755);
#endif
        if (rc != 0) std::fprintf(stderr, "js_engine: cannot create cache dir %s\n", dir.c_str());
    }
}

// The .jsc path of `filepath`: beside it by default, or under the cache dir
// keyed by cache_key (parent directories are created).
std::string cache_path(const JSEngine *engine, const char *filepath) {
    if (engine->cache_dir.empty()) return std::string(filepath) + "c";
    std::string path = engine->cache_dir + "/" +
                       cache_key(engine->cache_game_root, filepath) + "c";
    make_parent_dirs(path);
    return path;
}

// Byte layout of a V8 .jsc: int64 source mtime, kJscMagic, uint64 source
// length, the source, then V8's code cache. The source is kept because a V8
// code cache is only usable with its own source text, and js_engine.h
// promises that a cache hit (same mtime) runs what was cached.
constexpr size_t kJscHeader = sizeof(int64_t) + sizeof kJscMagic + sizeof(uint64_t);

// Splits `jsc` into its source and code cache when its header is valid and
// records `mtime`. Returns false for a stale or foreign file.
bool parse_jsc(const std::string &jsc, int64_t mtime, std::string *source,
               std::string *code) {
    if (jsc.size() < kJscHeader) return false;
    int64_t stored;
    uint64_t src_len;
    std::memcpy(&stored, jsc.data(), sizeof stored);
    std::memcpy(&src_len, jsc.data() + sizeof stored + sizeof kJscMagic, sizeof src_len);
    if (stored != mtime || std::memcmp(jsc.data() + sizeof stored, kJscMagic, sizeof kJscMagic) != 0 ||
        src_len > jsc.size() - kJscHeader)
        return false;
    source->assign(jsc, kJscHeader, src_len);
    code->assign(jsc, kJscHeader + src_len, std::string::npos);
    return true;
}

// Builds a .jsc file image; see kJscHeader for the layout.
std::string build_jsc(int64_t mtime, const std::string &source,
                      const v8::ScriptCompiler::CachedData *code) {
    uint64_t src_len = source.size();
    std::string out(reinterpret_cast<const char *>(&mtime), sizeof mtime);
    out.append(kJscMagic, sizeof kJscMagic);
    out.append(reinterpret_cast<const char *>(&src_len), sizeof src_len);
    out += source;
    out.append(reinterpret_cast<const char *>(code->data), static_cast<size_t>(code->length));
    return out;
}

// Reports every rejection still unhandled after a checkpoint through the
// shim's __error_handler_dispatchRejection (so the game's
// `unhandledrejection` listener fires), or as a console error when the shim
// is not loaded. Same behaviour as js_engine.c.
void flush_rejections(JSEngine *engine, v8::Local<v8::Context> context) {
    std::vector<Rejection> list;
    list.swap(engine->rejections);  // dispatching may reject further promises
    v8::Isolate *isolate = engine->isolate;
    v8::Local<v8::Value> fn;
    if (!context->Global()
             ->Get(context, v8_str(isolate, "__error_handler_dispatchRejection"))
             .ToLocal(&fn))
        fn = v8::Undefined(isolate);
    for (Rejection &r : list) {
        v8::HandleScope scope(isolate);
        v8::Local<v8::Value> reason = r.reason.Get(isolate);
        if (fn->IsFunction()) {
            v8::TryCatch tc(isolate);
            if (fn.As<v8::Function>()->Call(context, context->Global(), 1, &reason).IsEmpty())
                report_exception(engine, context, tc);
        } else {
            emit(engine, "error", "Unhandled promise rejection: " + to_utf8(isolate, reason));
        }
        r.promise.Reset();
        r.reason.Reset();
    }
}

}  // namespace

extern "C" {

// See js_engine.h. The V8 platform is started by jsb on first use and lives
// until exit; ICU data comes from SMZ_V8_ICU_DATA_PATH (CMakeLists.txt).
JSEngine *js_engine_init(void) {
    JSEngine *engine = new JSEngine();
    engine->rt = jsb_runtime_create("smzuggler", SMZ_V8_ICU_DATA_PATH);
    if (!engine->rt || !jsb_runtime_setup(engine->rt, adopt_runtime, engine)) {
        std::fprintf(stderr, "js_engine: cannot start V8\n");
        js_engine_shutdown(engine);
        return nullptr;
    }
    return engine;
}

// See js_engine.h. Globals are Reset before jsb disposes of the isolate.
void js_engine_shutdown(JSEngine *engine) {
    if (!engine) return;
    for (Rejection &r : engine->rejections) {
        r.promise.Reset();
        r.reason.Reset();
    }
    engine->context.Reset();
    if (engine->isolate) {
        engine->isolate->SetPromiseHook(nullptr);
        engine->isolate->RemoveMessageListeners(on_uncaught);
        engine->isolate->SetData(kEngineSlot, nullptr);
    }
    jsb_runtime_destroy(engine->rt);
    delete engine;
}

// See js_engine.h.
bool js_engine_eval(JSEngine *engine, const char *script, const char *filename) {
    if (!engine || !script) return false;
    return run_source(engine, script, std::strlen(script), filename ? filename : "<eval>",
                      nullptr, nullptr, nullptr);
}

// See js_engine.h. The copy is malloc'd so js_engine_free_string can free it
// the same way in both engine modes.
char *js_engine_eval_string(JSEngine *engine, const char *script, const char *filename) {
    if (!engine || !script) return nullptr;
    std::string result;
    if (!run_source(engine, script, std::strlen(script), filename ? filename : "<eval>",
                    nullptr, nullptr, &result))
        return nullptr;
    char *copy = static_cast<char *>(std::malloc(result.size() + 1));
    if (copy) std::memcpy(copy, result.c_str(), result.size() + 1);
    return copy;
}

// See js_engine.h.
void js_engine_free_string(char *str) { std::free(str); }

// See js_engine.h. Uses the .jsc cache when a cache dir is set, so the game
// folder is read-only, as in js_engine.c.
bool js_engine_eval_file(JSEngine *engine, const char *filepath) {
    if (!engine || !filepath) return false;
    if (!engine->cache_dir.empty()) return js_engine_eval_file_cached(engine, filepath);
    std::string source;
    if (!read_file(filepath, &source)) {
        std::fprintf(stderr, "Failed to open file: %s\n", filepath);
        return false;
    }
    return run_source(engine, source.data(), source.size(),
                      origin_name(engine, filepath).c_str(), nullptr, nullptr, nullptr);
}

// See js_engine.h. The cache is a V8 code cache stored with its source
// (see kJscHeader); V8 checks the code cache itself and recompiles silently
// when it rejects it (other V8 version or flags).
bool js_engine_eval_file_cached(JSEngine *engine, const char *filepath) {
    if (!engine || !filepath) return false;
    struct stat st;
    std::string jsc_path = cache_path(engine, filepath);
    int64_t mtime = stat(filepath, &st) == 0 ? static_cast<int64_t>(st.st_mtime) : -1;
    std::string jsc, source, code;
    std::string name = origin_name(engine, filepath);
    if (mtime >= 0 && read_file(jsc_path.c_str(), &jsc) && parse_jsc(jsc, mtime, &source, &code)) {
        auto *cached = new v8::ScriptCompiler::CachedData(
            reinterpret_cast<const uint8_t *>(code.data()), static_cast<int>(code.size()));
        return run_source(engine, source.data(), source.size(), name.c_str(), cached,
                          nullptr, nullptr);
    }
    if (!read_file(filepath, &source)) {
        std::fprintf(stderr, "Failed to open file: %s\n", filepath);
        return false;
    }
    v8::ScriptCompiler::CachedData *fresh = nullptr;
    bool ok = run_source(engine, source.data(), source.size(), name.c_str(), nullptr,
                         &fresh, nullptr);
    if (fresh && mtime >= 0 && !write_file(jsc_path, build_jsc(mtime, source, fresh)))
        std::fprintf(stderr, "js_engine: cannot write %s\n", jsc_path.c_str());
    delete fresh;
    return ok;
}

// See js_engine.h.
bool js_engine_set_cache_dir(JSEngine *engine, const char *cache_dir, const char *game_root) {
    if (!engine) return false;
    engine->cache_dir.clear();
    engine->cache_game_root.clear();
    if (!cache_dir || !cache_dir[0]) return true;
    engine->cache_dir = trimmed(cache_dir);
    if (game_root && game_root[0]) engine->cache_game_root = trimmed(game_root);
    return true;
}

// See js_engine.h. Same rule as js_engine.c.
bool js_engine_default_cache_dir(const char *app, char *out, size_t out_size) {
    if (!app || !app[0] || !out || out_size == 0) return false;
    out[0] = '\0';
#if defined(__linux__) && !defined(__ANDROID__)
    const char *base = std::getenv("XDG_CACHE_HOME");
    const char *suffix = "";
    if (!base || !base[0]) {
        base = std::getenv("HOME");
        suffix = "/.cache";
        if (!base || !base[0]) return false;
    }
    size_t len = std::strlen(base);
    while (len > 1 && base[len - 1] == '/') len--;
    int n = std::snprintf(out, out_size, "%.*s%s/%s/jsc", static_cast<int>(len), base,
                          suffix, app);
    if (n <= 0 || static_cast<size_t>(n) >= out_size) {
        out[0] = '\0';
        return false;
    }
    return true;
#else
    return false;
#endif
}

// See js_engine.h. One V8 microtask checkpoint (MicrotasksPolicy::kExplicit,
// spec section 4.1), then the unhandled-rejection report.
int js_engine_execute_pending_jobs(JSEngine *engine) {
    if (!engine) return -1;
    v8::Isolate *isolate = engine->isolate;
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = engine->context.Get(isolate);
    v8::Context::Scope context_scope(context);
    engine->jobs_run = 0;
    engine->job_failed = false;
    isolate->PerformMicrotaskCheckpoint();
    flush_rejections(engine, context);
    return engine->job_failed ? -1 : engine->jobs_run;
}

// See js_engine.h.
void js_engine_set_console_callback(JSEngine *engine, JSConsoleCallback cb, void *userdata) {
    if (!engine) return;
    engine->console_cb = cb;
    engine->console_userdata = userdata;
}

// See js_engine.h.
jsb_runtime *js_engine_get_jsb(JSEngine *engine) { return engine ? engine->rt : nullptr; }

// See js_engine.h. Only the limit is kept here: the V8 watchdog thread
// (Isolate::TerminateExecution) is card M1-T008, so nothing interrupts
// scripts yet in v8 mode; a non-zero limit says so once on stderr.
void js_engine_set_watchdog(JSEngine *engine, unsigned limit_ms) {
    if (!engine) return;
    engine->watchdog_ms = limit_ms;
    if (limit_ms)
        std::fprintf(stderr, "js_engine: the script watchdog is not available "
                             "on V8 yet; scripts are not interrupted\n");
}

// See js_engine.h. Nothing to restart until the V8 watchdog exists.
void js_engine_watchdog_arm(JSEngine *engine) { (void)engine; }

}  // extern "C"
