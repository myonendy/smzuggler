/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: ported from QuickJS to the jsb seam (spec section 4.1).
 *
 * __native_io: synchronous file I/O for fs_shim.js, which stands in for
 * Node's `fs` module as NW.js games use it (docs/js-binding.md, "bind_io").
 * Paths are resolved against the game root, or checked by the save-path
 * guard (io/file_io.h).
 *
 * smzuggler: the port keeps Outsider's structure line by line so upstream
 * fixes re-apply by hand (docs/UPSTREAM.md). Every jsb_val here is a
 * call-scoped handle, never kept past the native call that made it (spec
 * section 15.8, docs/js-binding.md "Handle and lifetime rules"). Main
 * thread only.
 */

#include "bindings/bind_io.h"
#include "io/file_io.h"

#ifdef SMZ_JSB_ADOPT
#include "jsb/jsb_adopt.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATH_BUF_SIZE 4096

/* Resolve JS argument `arg` (coerced with ToString) to an absolute path in
   `out`. Returns the string (caller must jsb_free_str) or NULL with a
   pending exception. */
static char *resolve_path_arg(jsb_ctx *ctx, int arg,
                              char *out, size_t out_size)
{
    char *raw = jsb_arg_to_str(ctx, arg, NULL);
    if (!raw) return NULL;

    if (!file_io_resolve_path(raw, out, out_size)) {
        jsb_throw_type(ctx, "path resolution failed: %s", raw);
        jsb_free_str(raw);
        return NULL;
    }
    return raw;
}

/* Like resolve_path_arg, for calls that only read. With the save-path guard
   on, an absolute path the write guard would accept is also allowed, so a
   save written outside the game root can be found and read back. */
static char *resolve_read_arg(jsb_ctx *ctx, int arg,
                              char *out, size_t out_size)
{
    char *raw = jsb_arg_to_str(ctx, arg, NULL);
    if (!raw) return NULL;

    if (!file_io_resolve_read_path(raw, out, out_size)) {
        jsb_throw_type(ctx, "path resolution failed: %s", raw);
        jsb_free_str(raw);
        return NULL;
    }
    return raw;
}

/* Like resolve_path_arg, for calls that change the file system. With the
   save-path guard on, the path must pass file_io_check_write_path() and is
   used as given: save paths are absolute file-system paths outside the game
   root, not game URLs. A refusal throws the same error a failed write does. */
static char *resolve_write_arg(jsb_ctx *ctx, int arg,
                               char *out, size_t out_size)
{
    if (!file_io_write_guard_enabled()) {
        return resolve_path_arg(ctx, arg, out, out_size);
    }

    char *raw = jsb_arg_to_str(ctx, arg, NULL);
    if (!raw) return NULL;

    size_t len = strlen(raw);
    if (!file_io_check_write_path(raw) || len >= out_size) {
        jsb_throw_internal(ctx, "failed to write file: %s", raw);
        jsb_free_str(raw);
        return NULL;
    }
    memcpy(out, raw, len + 1);
    return raw;
}

/* readFileSync(path) -> string
 *
 * Why: backs fs.readFileSync(path, "utf8") in fs_shim.js (Node fs,
 * https://nodejs.org/api/fs.html). `path` is coerced with ToString and read
 * through the save-path guard. The file's bytes become the string as UTF-8,
 * embedded NULs kept. A missing path is a TypeError, an unreadable file an
 * InternalError. */
static void js_read_file_sync(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "readFileSync requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_read_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    size_t size = 0;
    char *data = file_io_read_text(resolved, &size);
    if (!data) {
        jsb_throw_internal(ctx, "failed to read file: %s", resolved);
        return;
    }

    jsb_ret_val(ctx, jsb_new_str_len(ctx, data, size));
    free(data);
}

/* readFileBinary(path) -> ArrayBuffer
 *
 * Why: backs the binary form of fs.readFileSync (Node fs). Returns a new
 * ArrayBuffer the engine owns (a copy; the C buffer is freed here). Errors
 * as readFileSync. */
static void js_read_file_binary(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "readFileBinary requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_read_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    size_t size = 0;
    uint8_t *data = file_io_read_binary(resolved, &size);
    if (!data) {
        jsb_throw_internal(ctx, "failed to read binary file: %s", resolved);
        return;
    }

    jsb_ret_val(ctx, jsb_new_buffer_copy(ctx, data, size));
    free(data);
}

/* writeFileSync(path, data) -> undefined
 *
 * Why: backs fs.writeFileSync with a string (Node fs); RPG Maker saves go
 * through it. `data` is coerced with ToString and written as UTF-8 with its
 * full length. A path refused by the write guard or a failed write is an
 * InternalError "failed to write file: ...". */
static void js_write_file_sync(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "writeFileSync requires path and data arguments");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_write_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    size_t data_len = 0;
    char *data = jsb_arg_to_str(ctx, 1, &data_len);
    if (!data) return;

    /* Write with explicit length: strings may contain null bytes (e.g. deflated save data). */
    bool ok = file_io_write_binary(resolved, (const uint8_t *)data, data_len);
    jsb_free_str(data);

    if (!ok) {
        jsb_throw_internal(ctx, "failed to write file: %s", resolved);
        return;
    }
    jsb_ret_undefined(ctx);
}

/* smzuggler: reads argument `index` the way upstream's QuickJS call did: only an
 * ArrayBuffer is accepted, not a TypedArray or DataView (those have a
 * `byteOffset`; an ArrayBuffer has none). Why: jsb_arg_bytes also takes
 * views, and writeFileBinary's API must stay as it was. On success `*data`
 * and `*size` are borrowed from JS for the rest of this call (`*data` may be
 * NULL when `*size` is 0, which file_io_write_binary accepts). On failure
 * any pending exception is dropped and false is returned, so the caller
 * throws upstream's own message. */
static bool array_buffer_arg(jsb_ctx *ctx, int index, uint8_t **data,
                             size_t *size)
{
    if (!jsb_arg_bytes(ctx, index, data, size)) {
        jsb_clear_exception(ctx);
        return false;
    }
    jsb_val *offset = jsb_get_prop(ctx, jsb_arg_val(ctx, index), "byteOffset");
    if (!offset) {
        jsb_clear_exception(ctx);
        return false;
    }
    return jsb_val_type(ctx, offset) == JSB_TYPE_UNDEFINED;
}

/* writeFileBinary(path, arrayBuffer) -> undefined
 *
 * Why: backs fs.writeFileSync with a Buffer (Node fs). The ArrayBuffer's
 * bytes are borrowed for the call only. Anything but an ArrayBuffer is a
 * TypeError; a failed write an InternalError. */
static void js_write_file_binary(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "writeFileBinary requires path and data arguments");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_write_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    size_t size = 0;
    uint8_t *buf = NULL;
    if (!array_buffer_arg(ctx, 1, &buf, &size)) {
        jsb_throw_type(ctx, "writeFileBinary: second argument must be an ArrayBuffer");
        return;
    }

    bool ok = file_io_write_binary(resolved, buf, size);
    if (!ok) {
        jsb_throw_internal(ctx, "failed to write binary file: %s", resolved);
        return;
    }
    jsb_ret_undefined(ctx);
}

/* mkdirSync(path) -> bool
 *
 * Why: backs fs.mkdirSync (Node fs) for the save folder. Returns
 * file_io_mkdir's result; a guard refusal throws. */
static void js_mkdir_sync(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "mkdirSync requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_write_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    jsb_ret_bool(ctx, file_io_mkdir(resolved));
}

/* existsSync(path) -> bool
 *
 * Why: backs fs.existsSync (Node fs); StorageManager probes saves with it.
 * Path errors throw; a missing file is false. */
static void js_exists_sync(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "existsSync requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_read_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    jsb_ret_bool(ctx, file_io_exists(resolved));
}

/* isDirSync(path) -> bool
 *
 * Why: backs fs.statSync(path).isDirectory() in fs_shim.js (Node fs). */
static void js_is_dir_sync(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "isDirSync requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_read_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    jsb_ret_bool(ctx, file_io_is_directory(resolved));
}

/* readdirSync(path) -> string[]
 *
 * Why: backs fs.readdirSync (Node fs). Returns a new Array of entry names in
 * file_io_readdir's order; an unreadable directory is an InternalError. */
static void js_readdir_sync(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "readdirSync requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_read_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    size_t count = 0;
    char **entries = file_io_readdir(resolved, &count);
    if (!entries) {
        jsb_throw_internal(ctx, "failed to read directory: %s", resolved);
        return;
    }

    jsb_val *arr = jsb_new_array(ctx);
    for (size_t i = 0; i < count; i++) {
        jsb_set_index(ctx, arr, (uint32_t)i,
                      jsb_new_str(ctx, entries[i]));
    }
    file_io_free_dirlist(entries);
    jsb_ret_val(ctx, arr);
}

/* unlinkSync(path) -> bool
 *
 * Why: backs fs.unlinkSync (Node fs) for deleting saves. Returns whether the
 * file was removed; a guard refusal throws. */
static void js_unlink_sync(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "unlinkSync requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_write_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    jsb_ret_bool(ctx, file_io_unlink(resolved));
}

/* renameSync(oldPath, newPath) -> bool
 *
 * Why: backs fs.renameSync (Node fs), which save backups use. Both paths go
 * through the write guard, oldPath first. Returns whether the rename
 * succeeded. */
static void js_rename_sync(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 2) {
        jsb_throw_type(ctx, "renameSync requires oldPath and newPath arguments");
        return;
    }

    char resolved_old[PATH_BUF_SIZE];
    char *raw_old = resolve_write_arg(ctx, 0, resolved_old, sizeof(resolved_old));
    if (!raw_old) return;
    jsb_free_str(raw_old);

    char resolved_new[PATH_BUF_SIZE];
    char *raw_new = resolve_write_arg(ctx, 1, resolved_new, sizeof(resolved_new));
    if (!raw_new) return;
    jsb_free_str(raw_new);

    jsb_ret_bool(ctx, file_io_rename(resolved_old, resolved_new));
}

/* getGameRoot() -> string
 *
 * Why: the shims build process.cwd()-style paths from it. Arguments are
 * ignored. */
static void js_get_game_root(jsb_ctx *ctx)
{
    jsb_ret_str(ctx, file_io_get_game_root());
}

/* resolvePath(path) -> string
 *
 * Why: lets the shims turn a game-relative URL into the absolute path the
 * other calls use (no save-path guard). A path that cannot be resolved is a
 * TypeError "path resolution failed: ...". */
static void js_resolve_path(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_throw_type(ctx, "resolvePath requires a path argument");
        return;
    }

    char resolved[PATH_BUF_SIZE];
    char *raw = resolve_path_arg(ctx, 0, resolved, sizeof(resolved));
    if (!raw) return;
    jsb_free_str(raw);

    jsb_ret_str(ctx, resolved);
}

/* Registration */

/* Fills __native_io in upstream's function-list order, with its arities as
 * each function's `length`. Returns false with a JS exception pending. */
static bool io_module_init(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn_n(ctx, module, "readFileSync",    js_read_file_sync,    1) &&
           jsb_set_fn_n(ctx, module, "readFileBinary",  js_read_file_binary,  1) &&
           jsb_set_fn_n(ctx, module, "writeFileSync",   js_write_file_sync,   2) &&
           jsb_set_fn_n(ctx, module, "writeFileBinary", js_write_file_binary, 2) &&
           jsb_set_fn_n(ctx, module, "mkdirSync",       js_mkdir_sync,        1) &&
           jsb_set_fn_n(ctx, module, "existsSync",      js_exists_sync,       1) &&
           jsb_set_fn_n(ctx, module, "isDirSync",       js_is_dir_sync,       1) &&
           jsb_set_fn_n(ctx, module, "readdirSync",     js_readdir_sync,      1) &&
           jsb_set_fn_n(ctx, module, "unlinkSync",      js_unlink_sync,       1) &&
           jsb_set_fn_n(ctx, module, "renameSync",      js_rename_sync,       2) &&
           jsb_set_fn_n(ctx, module, "getGameRoot",     js_get_game_root,     0) &&
           jsb_set_fn_n(ctx, module, "resolvePath",     js_resolve_path,      1);
}

/* jsb_runtime_setup callback that defines the module; `user` is unused. */
static bool io_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_io", io_module_init);
}

/* smzuggler: upstream's function list defined the functions non-enumerable
 * (writable, configurable); jsb_set_fn_n assigns them, which makes them
 * enumerable. This restores the old attributes so the JS API stays
 * byte-for-byte. Run once by bind_io_define, after the module exists. */
static const char io_hide_fns_js[] =
    "(function (m) {\n"
    "  Object.getOwnPropertyNames(m).forEach(function (k) {\n"
    "    Object.defineProperty(m, k, { enumerable: false });\n"
    "  });\n"
    "})(__native_io);\n";

bool bind_io_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_io_define: no jsb runtime\n");
        return false;
    }
    if (!jsb_runtime_setup(rt, io_define_setup, NULL))
        return false;
    char err[256];
    if (!jsb_runtime_eval(rt, io_hide_fns_js, "bind_io.c", err, sizeof err)) {
        fprintf(stderr, "bind_io_define: %s\n", err);
        return false;
    }
    return true;
}

#ifdef SMZ_JSB_ADOPT
void bind_io_register(struct JSContext *ctx)
{
    bind_io_define(jsb_runtime_of(ctx));
}
#endif
