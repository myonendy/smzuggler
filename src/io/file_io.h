/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 */

#ifndef RMMZ_FILE_IO_H
#define RMMZ_FILE_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Set the directory that relative game paths resolve against.
   NULL or "" means the current working directory. */
void file_io_set_game_root(const char *root);

/* Get the current game root directory ("" if not set). */
const char *file_io_get_game_root(void);

/* Resolve a game path to a filesystem path inside the game root. Returns
   false on overflow or if the path escapes the root (traversal, outside prefix). */
bool file_io_resolve_path(const char *relative_path, char *out, size_t out_size);

/* Save-path guard. A game that computes its save directory from an unset
   environment variable ends up with a relative path, and writing that
   relative to the game root silently mixes saves into the game folder (or
   fails on a read-only install). With the guard on, bind_io accepts writes
   (write, mkdir, unlink, rename) only through file_io_check_write_path(),
   and uses the path as given instead of resolving it against the root.
   Off by default, because stock RMMZ saves to "<root>/save/". Reads use
   file_io_resolve_read_path(), so they can reach what was written. */
void file_io_set_write_guard(bool enabled);

/* True if the save-path guard is on. */
bool file_io_write_guard_enabled(void);

/* Decide whether `path` may be written: it must be absolute, and its real
   location (after "..", and symlinks on POSIX, are resolved for the longest
   existing prefix) must not be the game root or lie under it. Logs exactly
   one error on refusal, nothing on success. Does not consult the guard. */
bool file_io_check_write_path(const char *path);

/* Resolve a path for a read call (exists, isDir, readdir, readFile). Tries
   file_io_resolve_path() first, so game paths resolve exactly as before.
   Only if that refuses, the guard is on, and the path is absolute and would
   pass file_io_check_write_path(), the path is accepted as given: reads may
   go where save writes may go, and nowhere else. Logs nothing, so probing a
   missing save is silent. */
bool file_io_resolve_read_path(const char *path, char *out, size_t out_size);

/* Receives one file_io error message (one line, no trailing newline). The
   string is only valid for the duration of the call. */
typedef void (*FileIoLogFn)(const char *message);

/* Redirect file_io's error messages, so the host can route them to its log
   and tests can count them. NULL restores the default, which prints to
   stderr. Main thread only. */
void file_io_set_log_fn(FileIoLogFn fn);

/* Read a whole file as a NUL-terminated string (free() it). Returns NULL on
   error; out_size, if non-NULL, receives the byte count. */
char *file_io_read_text(const char *path, size_t *out_size);

/* Read a whole file as bytes (free() it). Returns NULL on error. */
uint8_t *file_io_read_binary(const char *path, size_t *out_size);

/* Write a NUL-terminated string to a file. Returns true on success. */
bool file_io_write_text(const char *path, const char *data);

/* Write binary data to a file. Returns true on success. */
bool file_io_write_binary(const char *path, const uint8_t *data, size_t size);

/* Check if a path exists (file or directory). */
bool file_io_exists(const char *path);

/* Check if a path is a directory. */
bool file_io_is_directory(const char *path);

/* Create a directory (and parents if needed). Returns true on success. */
bool file_io_mkdir(const char *path);

/* List directory entries (excluding . and ..) as a NULL-terminated array.
   Returns NULL on error; free with file_io_free_dirlist(). */
char **file_io_readdir(const char *path, size_t *out_count);

/* Free a directory listing returned by file_io_readdir(). */
void file_io_free_dirlist(char **list);

/* Delete a file. Returns true on success. */
bool file_io_unlink(const char *path);

/* Rename/move a file. Returns true on success. */
bool file_io_rename(const char *old_path, const char *new_path);

#endif /* RMMZ_FILE_IO_H */
