// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The smzuggler contributors
// Modified 2026-11-04: explicit microtask checkpoints and exception-safe nextTick priority.
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
// checkpoint the game loop calls after each macrotask, plus a watchdog that
// stops runaway scripts as Chromium's hung-page check would. The isolate and
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
// - The watchdog thread (js_watchdog.h) never enters the isolate: it only
//   calls the thread-safe Isolate::RequestInterrupt. Everything else, the
//   location report and TerminateExecution, runs on the main thread inside
//   the interrupt (docs/js-binding.md, "Watchdog").

#include "engine/js_engine.h"
#include "engine/js_watchdog.h"

#include "jsb.h"

#include <v8.h>

#include <sys/stat.h>

#include <atomic>
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
    // Nested dispatch from a promise must leave ticks to the outer checkpoint.
    bool checkpoint_running = false;
    // Watchdog limit (0 = off) and its timer thread, which exists only
    // while the limit is on; see js_engine_set_watchdog.
    unsigned watchdog_ms = 0;
    js_watchdog *watchdog = nullptr;
    // Generation the watchdog thread asked to interrupt (0 = none); written
    // by that thread, taken by the interrupt on the main thread.
    std::atomic<uint64_t> fired_gen{0};
    // Set when the watchdog terminated a script; cleared, with
    // CancelTerminateExecution, once no engine entry point is on the stack.
    bool terminated = false;
    // Nesting of engine entry points (an eval from a native called by JS
    // nests); only the outermost arms the watchdog and cancels termination.
    int depth = 0;
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

// Exhausts the engine-owned nextTick queue on the main thread, retrying after
// reported callback exceptions so remaining ticks still precede promises.
// Each attempt owns its handles; returns whether any ticks ran. Termination
// stops retries so the watchdog can return control to the host.
bool drain_ticks(v8::Isolate *isolate, v8::Local<v8::Context> context) {
    bool ran = false;
    for (;;) {
        v8::HandleScope scope(isolate);
        v8::TryCatch caught(isolate);
        v8::Local<v8::Value> drain, result;
        if (!context->Global()->Get(context, v8_str(isolate, "__smz_drainTicks")).ToLocal(&drain) ||
            !drain->IsFunction()) return ran;
        if (drain.As<v8::Function>()->Call(context, context->Global(), 0, nullptr)
                .ToLocal(&result)) return ran || result->BooleanValue(isolate);
        engine_of(isolate)->job_failed = true;
        if (caught.HasTerminated()) return ran;
        report_exception(engine_of(isolate), context, caught);
        ran = true;
    }
}

// Implements Node's checkpoint ordering: exhaust ticks, drain promises,
// then repeat if promises queued ticks. Main thread; no handles escape.
void run_checkpoint(v8::Isolate *isolate, v8::Local<v8::Context> context) {
    JSEngine *engine = engine_of(isolate);
    if (engine->checkpoint_running) return;
    engine->checkpoint_running = true;
    drain_ticks(isolate, context);
    do {
        isolate->PerformMicrotaskCheckpoint();
    } while (drain_ticks(isolate, context));
    engine->checkpoint_running = false;
}

// Installs the dedicated tick queue before any application script executes.
// The process setter preserves the storage shim's remaining Node properties.
// Functions implement https://nodejs.org/api/process.html#processnexttickcallback-args
// on the main thread; the closure owns callbacks until drained.
const char kTickBootstrap[] = R"JS(
/** Own Node ticks for the lifetime of this main-thread context. */
(function (g) {
    var ticks = [], processValue;
    /** Queue a Node callback and owned argument array; reject non-functions. */
    function nextTick(fn) {
        if (typeof fn !== 'function') throw new TypeError('nextTick callback must be a function');
        ticks.push([fn, Array.prototype.slice.call(arguments, 1)]);
    }
    /** Exhaust ticks before promises; errors propagate to the host checkpoint. */
    g.__smz_drainTicks = function () {
        var ran = ticks.length > 0;
        while (ticks.length) {
            var t = ticks.shift();
            t[0].apply(undefined, t[1]);
        }
        return ran;
    };
    Object.defineProperty(g, 'process', {
        configurable: true,
        /** Return the shim's process object without transferring ownership. */
        get: function () { return processValue; },
        /** Adopt the shim object while supplying the engine's tick priority. */
        set: function (value) {
            processValue = value;
            if (value) value.nextTick = nextTick;
        }
    });
 })(globalThis);
/** Install callback boundaries before DOM shims and application registration. */
(function (g) {
    /** Register host schedulers before shims load so startup callbacks are covered. */
    function scheduler(name) {
        var schedule;
        Object.defineProperty(g, name, {
            configurable: true,
            /** Return the scheduler wrapper; the closure retains the shim function. */
            get: function () { return schedule; },
            /** Wrap callback invocation on the main thread, preserving arguments. */
            set: function (fn) {
                if (typeof fn !== 'function') { schedule = fn; return; }
                /** Schedule an owned callback with a finally checkpoint. */
                schedule = function (cb) {
                    var args = Array.prototype.slice.call(arguments);
                    if (typeof cb === 'function') {
                        /** Finish this macrotask even when the callback throws. */
                        args[0] = function () {
                            try { return cb.apply(this, arguments); }
                            finally { g.__smz_microtaskCheckpoint(); }
                        };
                    }
                    return fn.apply(this, args);
                };
            }
        });
    }
    scheduler('setTimeout');
    scheduler('setInterval');
    scheduler('requestAnimationFrame');
 })(globalThis);
/** Preserve synchronous listeners while separating native input, resize, gamepad and video events. */
(function (g) {
    /** Deliver each polled event completely, including overlay default actions.
     * Main-thread-only; restore the borrowed poll function even on exceptions.
     * The dispatcher owns translation; singleton batches define task boundaries.
     */
    function inputTasks(fn, receiver, args) {
        if (typeof g.__native_input === 'undefined') return fn.apply(receiver, args);
        var input = g.__native_input, poll = input.pollEvents;
        var events = poll.call(input), result;
        if (!events || !events.length) return;
        try {
            for (var i = 0; i < events.length; i++) {
                var batch = [events[i]];
                /** Lend the current raw event to the existing DOM dispatcher. */
                input.pollEvents = function () { return batch; };
                try { result = fn.apply(receiver, args); }
                finally {
                    input.pollEvents = poll;
                    g.__smz_microtaskCheckpoint();
                }
            }
        } finally { input.pollEvents = poll; }
        return result;
    }
    /** Adopt a native phase, retaining its original dispatcher in this context. */
    function nativePhase(name) {
        var phase;
        Object.defineProperty(g, name, {
            configurable: true,
            /** Return the phase wrapper without transferring its ownership. */
            get: function () { return phase; },
            /** Preserve the phase's arguments and return value, including errors. */
            set: function (fn) {
                if (typeof fn !== 'function') { phase = fn; return; }
                /** Intercept event invocation only while native events are delivered. */
                phase = function () {
                    if (name === '__dom_flushInputEvents')
                        return inputTasks(fn, this, arguments);
                    var depth = 0, restores = [];
                    /** Wrap one target; nested synchronous dispatch shares the depth. */
                    function intercept(target) {
                        if (!target || typeof target.dispatchEvent !== 'function') return;
                        var dispatch = target.dispatchEvent;
                        restores.push([target, dispatch, Object.prototype.hasOwnProperty.call(target, 'dispatchEvent')]);
                        /** Checkpoint after all listeners and nested dispatch complete. */
                        target.dispatchEvent = function () {
                            depth++;
                            try { return dispatch.apply(this, arguments); }
                            finally { if (--depth === 0) g.__smz_microtaskCheckpoint(); }
                        };
                    }
                    intercept(g);
                    intercept(g.document);
                    if (name === '__dom_updateVideos' && g.HTMLVideoElement)
                        intercept(g.HTMLVideoElement.prototype);
                    try { return fn.apply(this, arguments); }
                    finally {
                        for (var i = restores.length - 1; i >= 0; i--) {
                            if (restores[i][2]) restores[i][0].dispatchEvent = restores[i][1];
                            else delete restores[i][0].dispatchEvent;
                        }
                    }
                };
            }
        });
    }
    nativePhase('__dom_flushInputEvents');
    nativePhase('__dom_updateGamepads');
    nativePhase('__dom_updateVideos');
    nativePhase('__dom_setWindowSize');
})(globalThis);
)JS";

// __smz_microtaskCheckpoint(): runs a V8 microtask checkpoint from inside
// JS. Why: dom_shim.js runs a whole frame's timers (or rAF callbacks) in
// one call, while Chrome runs a checkpoint after each of them (HTML "clean
// up after running script"); the bootstrap calls this after each
// callback (card M1-T010, DEC-M1-T010). It is the same nested checkpoint
// Node's runMicrotasks() makes under kExplicit. V8 ignores the call while
// microtasks are already running, so a job cannot re-enter the queue.
// Unhandled rejections are left for the checkpoint of
// js_engine_execute_pending_jobs, which reports them. Returns undefined.
void microtask_checkpoint(const v8::FunctionCallbackInfo<v8::Value> &info) {
    v8::Isolate *isolate = info.GetIsolate();
    v8::HandleScope scope(isolate);  // spec section 15.8: one per native call
    run_checkpoint(isolate, isolate->GetCurrentContext());
}

// Defines __smz_microtaskCheckpoint on the global object, not enumerable
// so that scripts listing window properties do not see it. Returns false
// if V8 could not create or define it.
bool install_checkpoint(v8::Isolate *isolate, v8::Local<v8::Context> context) {
    v8::Local<v8::Function> fn;
    if (!v8::Function::New(context, microtask_checkpoint).ToLocal(&fn)) return false;
    return context->Global()
        ->DefineOwnProperty(context, v8_str(isolate, "__smz_microtaskCheckpoint"), fn,
                            v8::DontEnum)
        .FromMaybe(false);
}

// jsb_runtime_setup callback of js_engine_init: the runtime's isolate and
// context are entered here, which is the only way jsb exposes them. Keeps
// the context, installs console, __smz_microtaskCheckpoint and the isolate
// callbacks. Microtasks run only at explicit checkpoints (kExplicit), never
// when a script returns, so the game loop decides when they run.
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
    if (!install_console(engine, context) || !install_checkpoint(engine->isolate, context))
        return false;
    v8::Local<v8::Script> bootstrap;
    v8::Local<v8::Value> result;
    return v8::Script::Compile(context, v8_str(engine->isolate, kTickBootstrap))
               .ToLocal(&bootstrap) && bootstrap->Run(context).ToLocal(&result);
}

// Returns `file:line:col` of the innermost JS frame, or "<unknown>" when
// no JS is on the stack. Main thread, isolate entered.
std::string current_location(v8::Isolate *isolate) {
    v8::HandleScope scope(isolate);
    v8::Local<v8::StackTrace> trace = v8::StackTrace::CurrentStackTrace(isolate, 1);
    if (trace->GetFrameCount() < 1) return "<unknown>";
    v8::Local<v8::StackFrame> frame = trace->GetFrame(isolate, 0);
    return to_utf8(isolate, frame->GetScriptName()) + ":" +
           std::to_string(frame->GetLineNumber()) + ":" +
           std::to_string(frame->GetColumn());
}

// Isolate::RequestInterrupt callback: runs on the main thread inside the
// running script, the only place where V8 can say where that script is.
// Reports the location at level `error` and terminates the script with
// Isolate::TerminateExecution, unless a later arm made this request stale
// (the script it was meant for already finished).
void on_watchdog_interrupt(v8::Isolate *isolate, void *data) {
    JSEngine *engine = static_cast<JSEngine *>(data);
    uint64_t gen = engine->fired_gen.exchange(0);
    if (gen == 0 || gen != js_watchdog_generation(engine->watchdog)) return;
    emit(engine, "error", "js_engine: watchdog: script ran longer than " +
                              std::to_string(engine->watchdog_ms) + " ms, terminated at " +
                              current_location(isolate));
    engine->terminated = true;
    isolate->TerminateExecution();
}

// js_watchdog fire callback, on the watchdog thread: records the overrun
// generation and asks V8 to run on_watchdog_interrupt at its next interrupt
// check (loop back-edges and calls). Touches nothing else of the isolate.
void on_watchdog_fire(uint64_t generation, void *user) {
    JSEngine *engine = static_cast<JSEngine *>(user);
    engine->fired_gen.store(generation);
    engine->isolate->RequestInterrupt(on_watchdog_interrupt, engine);
}

// Called when no engine entry point is on the stack: lets JS run again
// after a watchdog termination (V8 refuses all JS until it is cancelled).
void clear_termination(JSEngine *engine) {
    if (!engine->terminated) return;
    engine->terminated = false;
    engine->isolate->CancelTerminateExecution();
}

// Marks the start of an engine entry point that runs JS; the outermost one
// restarts the watchdog clock (js_engine.h: eval entry points arm it).
void begin_entry(JSEngine *engine) {
    if (engine->depth++ == 0) js_watchdog_arm(engine->watchdog);
}

// Marks the end of an entry point begun with begin_entry; the outermost one
// cancels a watchdog termination so the next script runs.
void end_entry(JSEngine *engine) {
    if (--engine->depth == 0) clear_termination(engine);
}

// Compiles and runs `source` (borrowed, `len` bytes of UTF-8) as a classic
// script named `name`, line and column offsets 0, so V8's 1-based stack
// positions match the file. `cache` (ownership taken, may be NULL) is a V8
// code cache to consume; when `cache_out` is not NULL a fresh code cache of
// the compiled script is stored there (caller deletes). The result goes to
// `result_out` when not NULL. Errors are reported, except a watchdog
// termination, which on_watchdog_interrupt already reported; returns false
// on them. Call through run_source.
bool compile_and_run(JSEngine *engine, const char *source, size_t len, const char *name,
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
        if (!tc.HasTerminated()) report_exception(engine, context, tc);
        return false;
    }
    if (cache_out)
        *cache_out = v8::ScriptCompiler::CreateCodeCache(script->GetUnboundScript());
    if (!script->Run(context).ToLocal(&result)) {
        if (!tc.HasTerminated()) report_exception(engine, context, tc);
        return false;
    }
    if (result_out) *result_out = to_utf8(isolate, result);
    return true;
}

// compile_and_run (same arguments and result) inside an engine entry, so
// the watchdog covers it and a termination is cancelled afterwards.
bool run_source(JSEngine *engine, const char *source, size_t len, const char *name,
                v8::ScriptCompiler::CachedData *cache,
                v8::ScriptCompiler::CachedData **cache_out, std::string *result_out) {
    begin_entry(engine);
    bool ok = compile_and_run(engine, source, len, name, cache, cache_out, result_out);
    end_entry(engine);
    return ok;
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

// See js_engine.h. The watchdog thread is joined, then Globals are Reset,
// before jsb disposes of the isolate.
void js_engine_shutdown(JSEngine *engine) {
    if (!engine) return;
    js_watchdog_stop(engine->watchdog);
    engine->watchdog = nullptr;
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
// spec section 4.1), then the unhandled-rejection report. The checkpoint is
// an entry point for the watchdog; after a termination the report waits for
// the next checkpoint and -1 is returned.
int js_engine_execute_pending_jobs(JSEngine *engine) {
    if (!engine) return -1;
    v8::Isolate *isolate = engine->isolate;
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = engine->context.Get(isolate);
    v8::Context::Scope context_scope(context);
    engine->jobs_run = 0;
    engine->job_failed = false;
    begin_entry(engine);
    run_checkpoint(isolate, context);
    bool stopped = engine->terminated;
    if (!stopped) flush_rejections(engine, context);
    end_entry(engine);
    return (stopped || engine->job_failed) ? -1 : engine->jobs_run;
}

// See js_engine.h.
void js_engine_set_console_callback(JSEngine *engine, JSConsoleCallback cb, void *userdata) {
    if (!engine) return;
    engine->console_cb = cb;
    engine->console_userdata = userdata;
}

// See js_engine.h.
jsb_runtime *js_engine_get_jsb(JSEngine *engine) { return engine ? engine->rt : nullptr; }

// See js_engine.h. The timer thread exists only while the limit is on: a
// new limit stops the old thread and starts one for the new limit, and 0
// leaves none. If the thread cannot start, scripts run unwatched (stderr
// says so). Main thread, outside JS.
void js_engine_set_watchdog(JSEngine *engine, unsigned limit_ms) {
    if (!engine) return;
    js_watchdog_stop(engine->watchdog);
    engine->watchdog = nullptr;
    engine->fired_gen.store(0);  // a request of the old thread is stale
    engine->watchdog_ms = limit_ms;
    if (limit_ms) {
        engine->watchdog = js_watchdog_start(limit_ms, on_watchdog_fire, engine);
        js_watchdog_arm(engine->watchdog);
    }
}

// See js_engine.h. Called by the game loop at the start of each frame, when
// no script runs, so a termination left by JS the loop called directly is
// cancelled here too.
void js_engine_watchdog_arm(JSEngine *engine) {
    if (!engine) return;
    if (engine->depth == 0) clear_termination(engine);
    js_watchdog_arm(engine->watchdog);
}

}  // extern "C"
