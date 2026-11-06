// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The smzuggler contributors
//
// The runtime's optional V8 CPU profile (spec section 9, milestone M1).
//
// Why: `--cpu-prof` is the V8 counterpart of Chromium's DevTools CPU
// profiler. It records V8's own samples, so a frame can be split into JS
// functions without an external `perf` run or JIT symbol maps, and writes a
// Chrome DevTools `.cpuprofile` that opens in the Performance panel. It
// stands in for the profile NW.js's Chromium writes with `--cpu-prof`, but
// script URLs are made relative to the game root first, as NW.js shows them
// in DevTools, so a shared profile never carries a host path. See
// docs/perf.md, "CPU profiles from the runtime".
//
// Rules:
// - Main thread only. The isolate is found through the jsb runtime inside
//   jsb_runtime_setup, which also gives the one live isolate scope; nothing
//   here stores a Local, jsb_val or jsb_ctx (spec section 15.8).
// - jsb_runtime and every string argument are borrowed; the handle copies
//   the path and resolves the game root it needs, so a relative --game still
//   yields game-relative URLs.
// - The profile is written before the engine is shut down, through
//   js_codecache_write, which creates parent directories and replaces the
//   file atomically so a crash never leaves a half-written profile.

#include "engine/js_profiler.h"

#include "engine/js_codecache.h"
#include "jsb.h"

#include <v8-profiler.h>
#include <v8.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

#ifdef _WIN32
#include <process.h>
#define smz_profiler_getpid _getpid
#else
#include <unistd.h>
#define smz_profiler_getpid getpid
#endif

// One running recording. `cpu` is created and disposed on the main thread;
// `isolate` is borrowed from the runtime and must outlive this handle.
struct js_profiler {
    v8::Isolate *isolate = nullptr;
    v8::CpuProfiler *cpu = nullptr;
    std::string path;
    std::string game_root;
};

namespace {

// The profile title V8 uses to identify the recording and to stop it again.
constexpr const char *kTitle = "smzuggler";

// Collects CpuProfile::Serialize output in memory, because URLs have to be
// made game-relative before the file is written.
class StringStream : public v8::OutputStream {
    // Output is buffered in memory so URLs can be rewritten before writing.
   public:
    // Nothing left to flush: the bytes are already in `data`.
    void EndOfStream() override {}

    // Appends one chunk; memory exhaustion is reported by a failed write
    // later, so the stream itself always continues.
    WriteResult WriteAsciiChunk(char *data, int size) override {
        bytes.append(data, static_cast<size_t>(size));
        return kContinue;
    }

    std::string bytes;
};

// Returns `text` escaped as JSON requires inside a string; paths never carry
// control characters, but quotes and backslashes do occur on Windows.
std::string json_escape(const std::string &text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        unsigned char u = static_cast<unsigned char>(c);
        if (c == '"' || c == '\\') {
            out.push_back('\\');
            out.push_back(c);
        } else if (u < 0x20) {
            char buf[8];
            std::snprintf(buf, sizeof buf, "\\u%04x", u);
            out.append(buf);
        } else {
            out.push_back(c);
        }
    }
    return out;
}

// Returns `path` without trailing separators (a lone "/" is kept), so the
// game root compares the same whether or not the caller passed one.
std::string trim_root(const std::string &path) {
    size_t len = path.size();
    while (len > 1 && (path[len - 1] == '/' || path[len - 1] == '\\')) len--;
    return path.substr(0, len);
}

// Returns `path` as an absolute, separator-normalised root for URL
// comparison. Why: --game may be relative or carry "..", while V8 records
// absolute file URLs, so without resolution the prefix check silently fails
// and an absolute host path would survive in a shared profile. realpath also
// resolves symlinks, matching the path V8 actually loaded. Falls back to the
// trimmed input when the directory cannot be resolved (for example it does
// not exist yet), so callers should pass an absolute path in that case.
std::string canonical_root(const std::string &path) {
    std::string trimmed = trim_root(path);
    if (trimmed.empty()) return trimmed;
#ifdef _WIN32
    char *resolved = _fullpath(NULL, trimmed.c_str(), 0);
#else
    char *resolved = realpath(trimmed.c_str(), NULL);
#endif
    std::string out = resolved ? std::string(resolved) : trimmed;
    std::free(resolved);
    for (char &c : out)
        if (c == '\\') c = '/';
    return trim_root(out);
}

// Returns the already JSON-escaped `url` without the escaped game root when
// it lies inside it; `file://` URLs over the same root are handled too. Why:
// NW.js names game scripts relatively, and an absolute host path in a shared
// profile would leak the user's directory layout (spec section 16).
std::string game_relative(const std::string &url, const std::string &root) {
    if (root.empty()) return url;
    const std::string file_prefix = "file://";
    if (url.compare(0, file_prefix.size(), file_prefix) == 0) {
        std::string rest = url.substr(file_prefix.size());
        if (rest.compare(0, root.size(), root) == 0 &&
            (rest.size() == root.size() || rest[root.size()] == '/'))
            return rest.substr(rest.size() == root.size() ? root.size() : root.size() + 1);
    }
    if (url.compare(0, root.size(), root) != 0) return url;
    if (url.size() == root.size()) return std::string();
    if (url[root.size()] != '/' && url[root.size()] != '\\') return url;
    return url.substr(root.size() + 1);
}

// Rewrites every `"url":"..."` value of one serialized CpuProfile through
// game_relative. Only the `url` key is touched, so a function or file name
// that happens to contain the game root is never altered.
std::string relativize_urls(const std::string &json, const std::string &root) {
    static const std::string key = "\"url\":\"";
    const std::string escaped_root = json_escape(root);
    if (escaped_root.empty()) return json;
    std::string out;
    size_t pos = 0;
    while (pos < json.size()) {
        size_t at = json.find(key, pos);
        if (at == std::string::npos) {
            out.append(json, pos, std::string::npos);
            break;
        }
        size_t start = at + key.size();
        size_t end = start;
        while (end < json.size() && json[end] != '"') {
            if (json[end] == '\\') end++;
            end++;
        }
        if (end >= json.size()) {  // truncated: keep the rest verbatim
            out.append(json, pos, std::string::npos);
            break;
        }
        out.append(json, pos, start - pos);
        out += game_relative(json.substr(start, end - start), escaped_root);
        pos = end;
    }
    return out;
}

// jsb_runtime_setup callback that creates and starts the profiler while an
// isolate scope is live; the V8 profiler itself outlives the scope.
bool start_recording(jsb_ctx *ctx, void *user) {
    (void)ctx;
    js_profiler *profiler = static_cast<js_profiler *>(user);
    profiler->isolate = v8::Isolate::GetCurrent();
    if (!profiler->isolate) return false;
    profiler->cpu = v8::CpuProfiler::New(profiler->isolate);
    if (!profiler->cpu) return false;
    // 100 us keeps a short run useful; Chromium's default of 1 ms misses
    // most of a single frame.
    profiler->cpu->SetSamplingInterval(100);
    v8::Local<v8::String> title;
    if (!v8::String::NewFromUtf8(profiler->isolate, kTitle).ToLocal(&title)) return false;
    v8::CpuProfilingResult result =
        profiler->cpu->Start(title, v8::kLeafNodeLineNumbers, true);
    return result.status == v8::CpuProfilingStatus::kStarted;
}

// Stops the recording, serializes it with game-relative URLs and writes it.
// Requires an isolate scope; returns false after a message on stderr.
bool write_profile(js_profiler *profiler) {
    v8::Local<v8::String> title;
    if (!v8::String::NewFromUtf8(profiler->isolate, kTitle).ToLocal(&title)) return false;
    v8::CpuProfile *profile = profiler->cpu->StopProfiling(title);
    if (!profile) {
        std::fprintf(stderr, "js_profiler: V8 returned no profile\n");
        return false;
    }
    StringStream stream;
    profile->Serialize(&stream, v8::CpuProfile::kJSON);
    profile->Delete();
    std::string json = relativize_urls(stream.bytes, profiler->game_root);
    if (!js_codecache_write(profiler->path.c_str(), json.data(), json.size())) return false;
    std::fprintf(stderr, "js_profiler: wrote %s\n", profiler->path.c_str());
    return true;
}

}  // namespace

extern "C" {

// See js_profiler.h. Owns nothing but its own copies of the two strings.
struct js_profiler *js_profiler_start(struct jsb_runtime *rt, const char *path,
                                      const char *game_root) {
    if (!rt || !path || !path[0]) {
        std::fprintf(stderr, "js_profiler: no runtime or no profile path\n");
        return nullptr;
    }
    js_profiler *profiler = new js_profiler();
    profiler->path = path;
    if (game_root) profiler->game_root = canonical_root(game_root);
    if (!jsb_runtime_setup(rt, start_recording, profiler) || !profiler->cpu) {
        std::fprintf(stderr, "js_profiler: cannot start V8's CPU profiler\n");
        if (profiler->cpu) profiler->cpu->Dispose();
        delete profiler;
        return nullptr;
    }
    return profiler;
}

// See js_profiler.h. The isolate scope is entered only while V8 is alive;
// the profile is written before the handle is freed.
bool js_profiler_stop(struct js_profiler *profiler) {
    if (!profiler) return true;
    bool ok = false;
    if (profiler->isolate && profiler->cpu) {
        v8::Isolate::Scope isolate_scope(profiler->isolate);
        v8::HandleScope handle_scope(profiler->isolate);
        ok = write_profile(profiler);
        profiler->cpu->Dispose();
    }
    delete profiler;
    return ok;
}

// See js_profiler.h. <state_dir>/profiles/cpu-<pid>-<unix time>.cpuprofile.
bool js_profiler_default_path(const char *state_dir, char *out, size_t out_size) {
    if (!state_dir || !state_dir[0] || !out || out_size == 0) return false;
    out[0] = '\0';
    int n = std::snprintf(out, out_size, "%s/profiles/cpu-%ld-%lld.cpuprofile", state_dir,
                          static_cast<long>(smz_profiler_getpid()),
                          static_cast<long long>(std::time(nullptr)));
    if (n <= 0 || static_cast<size_t>(n) >= out_size) {
        out[0] = '\0';
        return false;
    }
    return true;
}

}  // extern "C"
