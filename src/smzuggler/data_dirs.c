/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Resolve host-owned data directories from XDG or SDL so the game folder is
 * never written. See docs/data-dirs.md. Pure path construction; callers mkdir.
 */
#include "smzuggler/data_dirs.h"
#include "smzuggler/names.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__linux__) || defined(__ANDROID__)
#include <SDL3/SDL.h>
#endif

/** Drop trailing slashes so joins do not produce empty path segments. */
static void trim_slash(char *path)
{
    size_t n;
    if (!path) return;
    n = strlen(path);
    while (n > 1 && (path[n - 1] == '/' || path[n - 1] == '\\'))
        path[--n] = '\0';
}

/** True for a filesystem separator on either slash style. */
static int is_sep(char c)
{
    return c == '/' || c == '\\';
}

/** True when p is an absolute filesystem path (POSIX, or a Windows drive). */
static int is_absolute(const char *p)
{
    if (!p || !p[0]) return 0;
    if (is_sep(p[0])) return 1;
#ifdef _WIN32
    if (((p[0] >= 'A' && p[0] <= 'Z') || (p[0] >= 'a' && p[0] <= 'z')) &&
        p[1] == ':' && is_sep(p[2]))
        return 1;
#endif
    return 0;
}

/** True when path equals root or is a child of it. Trailing slashes on root
 * are ignored. Both strings are borrowed; NULL/empty root is never a match.
 */
static int inside_root(const char *path, const char *root)
{
    size_t n;
    if (!path || !root || !root[0]) return 0;
    n = strlen(root);
    while (n > 1 && is_sep(root[n - 1])) n--;
    if (strncmp(path, root, n) != 0) return 0;
    return path[n] == '\0' || is_sep(path[n]);
}

/* See the public header for the absolute/outside-game-root contract. */
bool smz_data_dir_usable(const char *path, const char *game_root)
{
    return path && path[0] && is_absolute(path) && !inside_root(path, game_root);
}

/** Copy src into dst, trimming trailing slashes. Returns 0 if it does not fit. */
static int copy_trimmed(char *dst, size_t n, const char *src)
{
    int k;
    if (!dst || n == 0 || !src) return 0;
    k = snprintf(dst, n, "%s", src);
    if (k <= 0 || (size_t)k >= n) return 0;
    trim_slash(dst);
    return dst[0] != '\0';
}

/** XDG variable for kind, or NULL when kind is unknown. */
static const char *xdg_var(SmzDataDirKind kind)
{
    switch (kind) {
    case SMZ_DATA_DIR_STATE:  return "XDG_STATE_HOME";
    case SMZ_DATA_DIR_CACHE:  return "XDG_CACHE_HOME";
    case SMZ_DATA_DIR_CONFIG: return "XDG_CONFIG_HOME";
    }
    return NULL;
}

/** $HOME-relative suffix for kind when the matching XDG variable is unset. */
static const char *home_suffix(SmzDataDirKind kind)
{
    switch (kind) {
    case SMZ_DATA_DIR_STATE:  return ".local/state";
    case SMZ_DATA_DIR_CACHE:  return ".cache";
    case SMZ_DATA_DIR_CONFIG: return ".config";
    }
    return NULL;
}

/** Write base[/mid]/SMZUGGLER_BIN into out. mid may be NULL. */
static int join_app(char *out, size_t n, const char *base, const char *mid)
{
    char prefix[1024];
    int k;
    if (!copy_trimmed(prefix, sizeof(prefix), base)) return 0;
    if (mid && mid[0])
        k = snprintf(out, n, "%s/%s/%s", prefix, mid, SMZUGGLER_BIN);
    else
        k = snprintf(out, n, "%s/%s", prefix, SMZUGGLER_BIN);
    return k > 0 && (size_t)k < n;
}

#if defined(__linux__) && !defined(__ANDROID__)
/** Linux XDG path for kind. Relative or empty XDG values fall back to HOME. */
static int resolve_linux(SmzDataDirKind kind, char *out, size_t n)
{
    const char *var = xdg_var(kind);
    const char *suffix = home_suffix(kind);
    const char *base;
    if (!var || !suffix) return 0;
    base = getenv(var);
    if (base && base[0] && is_absolute(base))
        return join_app(out, n, base, NULL);
    base = getenv("HOME");
    if (base && base[0] && is_absolute(base))
        return join_app(out, n, base, suffix);
    return 0;
}
#else
/** SDL pref path used on Windows, macOS and Android for every kind. */
static int resolve_pref(char *out, size_t n)
{
    char *pref = SDL_GetPrefPath("", SMZUGGLER_BIN);
    int ok;
    if (!pref) return 0;
    ok = copy_trimmed(out, n, pref);
    SDL_free(pref);
    return ok;
}
#endif

/* See the public header for XDG/SDL fallbacks, ownership and error cases. */
bool smz_data_dir(SmzDataDirKind kind, const char *game_root,
                  char *out, size_t out_size)
{
    int ok;
    if (!out || out_size == 0) return false;
    out[0] = '\0';
    if (kind < SMZ_DATA_DIR_STATE || kind > SMZ_DATA_DIR_CONFIG) return false;
#if defined(__linux__) && !defined(__ANDROID__)
    ok = resolve_linux(kind, out, out_size);
#else
    (void)kind;
    ok = resolve_pref(out, out_size);
#endif
    if (!ok || !smz_data_dir_usable(out, game_root)) {
        out[0] = '\0';
        return false;
    }
    return true;
}
