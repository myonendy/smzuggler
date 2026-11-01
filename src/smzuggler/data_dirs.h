/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Host-owned state, cache and config paths so the game folder stays read-only.
 * See docs/data-dirs.md. Callers own output buffers; this module never mkdir's.
 */
#ifndef SMZUGGLER_DATA_DIRS_H
#define SMZUGGLER_DATA_DIRS_H

#include <stdbool.h>
#include <stddef.h>

/** Which host directory to resolve. Values are the three XDG bases in §13. */
typedef enum {
    SMZ_DATA_DIR_STATE = 0,  /* logs: debug.log, console.log */
    SMZ_DATA_DIR_CACHE = 1,  /* disposable .jsc parent */
    SMZ_DATA_DIR_CONFIG = 2  /* runtime config directory */
} SmzDataDirKind;

/** True when path is absolute and is not the game root or a child of it.
 * game_root may be NULL or empty (then only the absolute check applies).
 * Both strings are borrowed; no allocation. Safe on any thread.
 */
bool smz_data_dir_usable(const char *path, const char *game_root);

/** Fill out with the absolute host directory for kind, named SMZUGGLER_BIN.
 * Linux uses $XDG_* / $HOME fallbacks; elsewhere SDL_GetPrefPath.
 * Empty or relative XDG values are treated as unset so cwd is never used.
 * game_root is borrowed and may be NULL; a result under it is refused.
 * out is caller-owned. Does not create directories or retain pointers.
 * Main thread (getenv / SDL). Returns false on missing env, overflow, a
 * relative result, an unknown kind, or a game-root collision; out is then "".
 */
bool smz_data_dir(SmzDataDirKind kind, const char *game_root,
                  char *out, size_t out_size);

#endif
