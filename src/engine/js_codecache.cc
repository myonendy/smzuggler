// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The smzuggler contributors
//
// The on-disk store of V8 code caches: key hashing, the cache directory and
// whole-file reads and atomic writes (spec sections 4.1 item 4 and 5.6).
//
// Why: the runtime must never write next to a game's scripts, and a code
// cache is only valid for its exact source, V8 build and flags. This file
// names a cache by the sha256 of all three and keeps it under the host's
// cache directory, where Chromium (and so NW.js) keeps its own code cache.
// It has no V8 dependency, so it builds and is tested independently. See
// src/engine/js_codecache.h, docs/js-binding.md, "Code cache", and
// docs/data-dirs.md.
//
// Rules:
// - Main thread only: the event counters are not atomic.
// - Writes go through a temporary file in the target directory and a
//   rename, so a reader never sees a partial cache.
// - No V8 handles are touched here, so the §15.8 handle rules do not apply.

#include "engine/js_codecache.h"

#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define smz_getpid _getpid
#else
#include <unistd.h>
#define smz_getpid getpid
#endif

namespace {

// SHA-256 round constants (FIPS 180-4, section 4.2.2).
constexpr uint32_t kRound[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
    0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
    0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
    0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
    0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
    0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
    0xc67178f2};

// Incremental SHA-256 state. A local hash is used because the project has
// no crypto dependency and the key needs no more than this.
struct Sha256 {
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                     0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    uint8_t block[64] = {};
    size_t used = 0;     // bytes waiting in `block`
    uint64_t total = 0;  // bytes hashed so far
};

// Rotates `x` right by `n` bits (0 < n < 32).
uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

// Mixes one full 64-byte block into `s` (FIPS 180-4, section 6.2.2).
void sha_block(Sha256 *s, const uint8_t *p) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 |
               (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t v[8];
    std::memcpy(v, s->h, sizeof v);
    for (int i = 0; i < 64; i++) {
        uint32_t e = v[4], a = v[0];
        uint32_t t1 = v[7] + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) +
                      ((e & v[5]) ^ (~e & v[6])) + kRound[i] + w[i];
        uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) +
                      ((a & v[1]) ^ (a & v[2]) ^ (v[1] & v[2]));
        std::memmove(v + 1, v, 7 * sizeof v[0]);
        v[4] += t1;
        v[0] = t1 + t2;
    }
    for (int i = 0; i < 8; i++) s->h[i] += v[i];
}

// Hashes `len` bytes of `data` (may be NULL when len is 0) into `s`.
void sha_update(Sha256 *s, const void *data, size_t len) {
    const uint8_t *p = static_cast<const uint8_t *>(data);
    s->total += len;
    while (len > 0) {
        size_t take = 64 - s->used < len ? 64 - s->used : len;
        std::memcpy(s->block + s->used, p, take);
        s->used += take;
        p += take;
        len -= take;
        if (s->used == 64) {
            sha_block(s, s->block);
            s->used = 0;
        }
    }
}

// Pads and finishes `s`, writing the digest as 64 lowercase hex digits and
// a NUL to `out`. `s` must not be updated afterwards.
void sha_final_hex(Sha256 *s, char *out) {
    uint64_t bits = s->total * 8;
    uint8_t pad = 0x80, zero = 0, len_be[8];
    sha_update(s, &pad, 1);
    while (s->used != 56) sha_update(s, &zero, 1);
    for (int i = 0; i < 8; i++) len_be[i] = (uint8_t)(bits >> (56 - 8 * i));
    sha_update(s, len_be, 8);
    static const char kHex[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) {
        uint8_t byte = (uint8_t)(s->h[i / 4] >> (24 - 8 * (i % 4)));
        out[2 * i] = kHex[byte >> 4];
        out[2 * i + 1] = kHex[byte & 15];
    }
    out[JS_CODECACHE_KEY_LEN] = '\0';
}

// True for a path separator on any platform.
bool is_sep(char c) { return c == '/' || c == '\\'; }

// Creates every missing parent directory of `path`. Failures are left to
// the following open, which reports them with the file's path.
void make_parent_dirs(const std::string &path) {
    for (size_t i = 1; i < path.size(); i++) {
        if (!is_sep(path[i]) || is_sep(path[i - 1]) || path[i - 1] == ':') continue;
        std::string dir = path.substr(0, i);
        struct stat st;
        if (stat(dir.c_str(), &st) == 0) continue;
#ifdef _WIN32
        _mkdir(dir.c_str());
#else
        mkdir(dir.c_str(), 0755);
#endif
    }
}

// Process-wide lookup counters; see js_codecache_note.
js_codecache_stats g_stats = {0, 0, 0};

}  // namespace

extern "C" {

// See js_codecache.h. The two NULs keep ("ab","c") and ("a","bc") apart.
bool js_codecache_key(const char *source, size_t len, const char *engine_version,
                      uint32_t flag_hash, char out[JS_CODECACHE_KEY_LEN + 1]) {
    if ((!source && len) || !engine_version || !out) return false;
    Sha256 s;
    const uint8_t nul = 0;
    uint8_t tag[4] = {(uint8_t)flag_hash, (uint8_t)(flag_hash >> 8),
                      (uint8_t)(flag_hash >> 16), (uint8_t)(flag_hash >> 24)};
    sha_update(&s, source, len);
    sha_update(&s, &nul, 1);
    sha_update(&s, engine_version, std::strlen(engine_version));
    sha_update(&s, &nul, 1);
    sha_update(&s, tag, sizeof tag);
    sha_final_hex(&s, out);
    return true;
}

// See js_codecache.h.
bool js_codecache_default_dir(char *out, size_t out_size) {
    if (!out || out_size == 0) return false;
    out[0] = '\0';
#if defined(__linux__) && !defined(__ANDROID__)
    const char *base = std::getenv("XDG_CACHE_HOME");
    const char *suffix = "";
    if (!base || base[0] != '/') {
        base = std::getenv("HOME");
        suffix = "/.cache";
        if (!base || base[0] != '/') return false;
    }
    size_t len = std::strlen(base);
    while (len > 1 && base[len - 1] == '/') len--;
    int n = std::snprintf(out, out_size, "%.*s%s/smzuggler-rt/v8cache",
                          static_cast<int>(len), base, suffix);
    if (n <= 0 || static_cast<size_t>(n) >= out_size) {
        out[0] = '\0';
        return false;
    }
    return true;
#else
    return false;
#endif
}

// See js_codecache.h.
bool js_codecache_path(const char *dir, const char *key, char *out, size_t out_size) {
    if (!dir || !dir[0] || !key || !key[0] || !out || out_size == 0) return false;
    size_t len = std::strlen(dir);
    while (len > 1 && is_sep(dir[len - 1])) len--;
    int n = std::snprintf(out, out_size, "%.*s/%s.bin", static_cast<int>(len), dir, key);
    if (n <= 0 || static_cast<size_t>(n) >= out_size) {
        out[0] = '\0';
        return false;
    }
    return true;
}

// See js_codecache.h.
bool js_codecache_read(const char *path, uint8_t **data, size_t *len) {
    if (!data || !len) return false;
    *data = nullptr;
    *len = 0;
    FILE *f = path ? std::fopen(path, "rb") : nullptr;
    if (!f) return false;
    std::string buf;
    char chunk[16384];
    size_t n;
    while ((n = std::fread(chunk, 1, sizeof chunk, f)) > 0) buf.append(chunk, n);
    bool ok = !std::ferror(f) && !buf.empty();
    std::fclose(f);
    if (!ok) return false;
    *data = static_cast<uint8_t *>(std::malloc(buf.size()));
    if (!*data) return false;
    std::memcpy(*data, buf.data(), buf.size());
    *len = buf.size();
    return true;
}

// See js_codecache.h.
void js_codecache_free(uint8_t *data) { std::free(data); }

// See js_codecache.h. The temporary name carries the pid, so two runtimes
// writing the same key never share a temporary file.
bool js_codecache_write(const char *path, const void *data, size_t len) {
    if (!path || !path[0] || (!data && len)) return false;
    make_parent_dirs(path);
    std::string tmp = std::string(path) + ".tmp" + std::to_string(smz_getpid());
    FILE *f = std::fopen(tmp.c_str(), "wb");
    if (!f) {
        std::fprintf(stderr, "js_codecache: cannot write %s\n", tmp.c_str());
        return false;
    }
    bool ok = std::fwrite(data, 1, len, f) == len;
    ok = std::fclose(f) == 0 && ok;
#ifdef _WIN32
    if (ok) std::remove(path);  // rename does not replace a file on Windows
#endif
    if (ok) ok = std::rename(tmp.c_str(), path) == 0;
    if (!ok) {
        std::remove(tmp.c_str());
        std::fprintf(stderr, "js_codecache: cannot write %s\n", path);
    }
    return ok;
}

// See js_codecache.h.
bool js_codecache_discard(const char *path) {
    if (!path || !path[0]) return false;
    struct stat st;
    return std::remove(path) == 0 || stat(path, &st) != 0;
}

// See js_codecache.h.
void js_codecache_note(js_codecache_event event) {
    switch (event) {
        case JS_CODECACHE_PRODUCED: g_stats.produced++; break;
        case JS_CODECACHE_ACCEPTED: g_stats.accepted++; break;
        case JS_CODECACHE_REJECTED: g_stats.rejected++; break;
    }
}

// See js_codecache.h.
void js_codecache_get_stats(js_codecache_stats *out) {
    if (out) *out = g_stats;
}

// See js_codecache.h.
void js_codecache_reset_stats(void) { g_stats = js_codecache_stats{0, 0, 0}; }

}  // extern "C"
