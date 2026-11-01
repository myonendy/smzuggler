/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Host-owned state, cache and config paths so the game folder stays read-only.
 * See docs/data-dirs.md. Callers own output buffers; this module never mkdir's.
 */
#ifndef SMZUGGLER_DATA_DIRS_H
#define SMZUGGLER_DATA_DIRS_H

#include <stdbool.h>
#include <stddef.h>

/** Byte capacity of each SmzHostDirs path, matching main.c's path buffers. */
#define SMZ_DATA_DIR_MAX 1024

/** Which host directory to resolve. Values are the three XDG bases in §13. */
typedef enum {
    SMZ_DATA_DIR_STATE = 0,  /* logs: debug.log, console.log */
    SMZ_DATA_DIR_CACHE = 1,  /* disposable .jsc parent */
    SMZ_DATA_DIR_CONFIG = 2  /* runtime config directory */
} SmzDataDirKind;

/** Paths main() uses at startup. Empty string means that kind failed.
 * Caller-owned; the resolver never mkdir's or retains pointers.
 */
typedef struct SmzHostDirs {
    char log_dir[SMZ_DATA_DIR_MAX];
    char debug_log[SMZ_DATA_DIR_MAX];
    char console_log[SMZ_DATA_DIR_MAX];
    char cache_dir[SMZ_DATA_DIR_MAX];
    char jsc_dir[SMZ_DATA_DIR_MAX];
    char config_dir[SMZ_DATA_DIR_MAX];
} SmzHostDirs;

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

/** Choose the log directory the way main() does: --data-dir=PATH if PATH is
 * usable, else the host state dir (default-on; a bare --data-dir is the
 * same). argv is borrowed; game_root may be NULL. out is caller-owned.
 * Never mkdir's. Main thread (getenv). Returns true when out is absolute
 * and outside the game root.
 */
bool smz_choose_log_dir(int argc, char **argv, const char *game_root,
                        char *out, size_t out_size);

/** Fill dirs the way main() does at startup: logs honor --data-dir, cache
 * and config always use smz_data_dir (CONFIG is resolved even though the
 * config file is read by a later card). argv and game_root are borrowed.
 * dirs is caller-owned and zeroed first. Never mkdir's. Main thread.
 * Returns true when at least one kind resolved.
 */
bool smz_host_dirs(int argc, char **argv, const char *game_root,
                   SmzHostDirs *dirs);

#endif
