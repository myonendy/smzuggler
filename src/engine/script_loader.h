/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: document the loader interface (M1-T030).
 * Modified 2026-11-06: add the real-PIXI / Outsider renderer mode (M2-T005).
 *
 * Declares the script load-order entry points used by main.c and tests.
 * Why: the game must see shims, libraries, core scripts, plugins and main.js
 * in NW.js order; callers use these functions instead of reimplementing it.
 * See docs/js-binding.md; main thread only.
 */

#ifndef RMMZ_SCRIPT_LOADER_H
#define RMMZ_SCRIPT_LOADER_H

#include "engine/js_engine.h"
#include <stdbool.h>
#include <stddef.h>

/* One entry parsed from plugins.js. */
typedef struct {
    char *name;     /* plugin filename without extension */
    bool  status;   /* enabled? */
} PluginEntry;

typedef struct {
    PluginEntry *entries;
    size_t       count;
} PluginList;

/* Which renderer the game sees (spec §4.2). REAL_PIXI is the default: the
   game's own js/libs/pixi.js drives WebGL2 and the PIXI/tilemap shims are
   absent. OUTSIDER keeps the map-to-quads shims for comparison and for
   tracking upstream (§8.3). The value is fixed for the life of a process. */
typedef enum {
    SCRIPT_LOADER_RENDERER_REAL_PIXI = 0,
    SCRIPT_LOADER_RENDERER_OUTSIDER = 1
} ScriptLoaderRenderer;

/* Parse plugins.js (var $plugins = [{name, status, ...}, ...]) into a list.
   Returns NULL on error; free with script_loader_free_plugins(). */
PluginList *script_loader_parse_plugins(const char *plugins_js_path);

/* Free a plugin list returned by script_loader_parse_plugins(). */
void script_loader_free_plugins(PluginList *list);

/* Map a --renderer value to a mode: "real-pixi"/"real_pixi"/"realpixi" and
   NULL select REAL_PIXI (the default); "outsider" selects OUTSIDER.
   Why: main.c and the evaluator must agree on the accepted CLI spellings.
   `name` is borrowed; `*out` is written only on success. Returns false and
   leaves `*out` untouched for an unknown or empty name or a NULL `out`;
   main thread. */
bool script_loader_renderer_parse(const char *name, ScriptLoaderRenderer *out);

/* Load everything in order for RENDERER: shims, libs, rmmz core scripts,
   enabled plugins, post-core shims, main.js; then start the game. In
   REAL_PIXI the game's own pixi.js replaces pixi_shim.js/tilemap_shim.js.
   Returns false on a missing argument or fatal shim failure; main thread. */
bool script_loader_load_all_for(JSEngine *engine, const char *shim_dir,
                                const char *game_dir, ScriptLoaderRenderer renderer);

/* Load only the shim files for RENDERER (for tests without game files). */
bool script_loader_load_shims_for(JSEngine *engine, const char *shim_dir,
                                  ScriptLoaderRenderer renderer);

/* NULL-terminated list of every shim file name RENDERER loads, in order
   (pre-core first, then post-core). The array is static; do not free. */
const char *const *script_loader_shim_files_for(ScriptLoaderRenderer renderer);

/* Return the script paths load_all_for would load for RENDERER, without
   loading them. Free with script_loader_free_path_list(). */
char **script_loader_get_load_order_for(ScriptLoaderRenderer renderer,
                                        const char *shim_dir, const char *game_dir,
                                        size_t *out_count);

/* Free a path list returned by script_loader_get_load_order_for(). */
void script_loader_free_path_list(char **list, size_t count);

/* Outsider-mode compatibility entry points. Existing scripts and the
   integration tests were written before real PIXI existed, so these keep
   returning the OUTSIDER plan exactly; main.c uses the *_for forms. */
/* Load everything in Outsider mode (see script_loader_load_all_for). */
bool script_loader_load_all(JSEngine *engine, const char *shim_dir, const char *game_dir);

/* Load only the Outsider-mode shim files (see script_loader_load_shims_for). */
bool script_loader_load_shims(JSEngine *engine, const char *shim_dir);

/* Every Outsider-mode shim file name in load order (see
   script_loader_shim_files_for); the array is static. */
const char *const *script_loader_shim_files(void);

/* Outsider-mode planned path list (see script_loader_get_load_order_for). */
char **script_loader_get_load_order(const char *shim_dir, const char *game_dir, size_t *out_count);

#endif /* RMMZ_SCRIPT_LOADER_H */
