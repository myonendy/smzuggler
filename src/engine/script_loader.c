/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-05: document the loader's public functions (M1-T030).
 * Modified 2026-11-06: select real PIXI or the Outsider shims at load time (M2-T005).
 *
 * Loads the shims, library scripts, RMMZ core scripts, enabled plugins and
 * main.js in the order NW.js would, then hands control to the game. Why:
 * RPG Maker MZ expects a browser load order, and plugins mutate globals in
 * that order, so the loader is the single place that establishes it.
 * See docs/js-binding.md ("Game loop", "js_engine on V8"); main thread only.
 */

#include "engine/script_loader.h"
#include "io/file_io.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Reject plugin names containing path separators or traversal. */
/* Plugin names are paths under js/plugins/ and may use subfolders
   ("Pocket/Pocket_Mirror_BASE"); only escaping that folder is rejected. */
static bool is_valid_plugin_name(const char *name)
{
    if (!name || !*name || name[0] == '/' || strchr(name, '\\') || strchr(name, ':')) return false;
    return strstr(name, "..") == NULL;
}

/* Join dir + file with exactly one separator. Returns a malloc'd string. */
static char *join_path(const char *dir, const char *file)
{
    if (!dir || !file) return NULL;
    size_t dlen = strlen(dir);
    size_t flen = strlen(file);
    bool has_sep = (dlen > 0 && (dir[dlen - 1] == '/' || dir[dlen - 1] == '\\'));
    size_t total = dlen + flen + (has_sep ? 0 : 1) + 1;
    char *out = malloc(total);
    if (!out) return NULL;
    if (has_sep) {
        snprintf(out, total, "%s%s", dir, file);
    } else {
        snprintf(out, total, "%s/%s", dir, file);
    }
    return out;
}

/* Plugin parser */

static const char *skip_ws(const char *p)
{
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

/* Copy the quoted string at *pp (which must point at the opening quote) and
   advance *pp past the closing quote. Returns a malloc'd string. */
static char *extract_quoted_string(const char **pp)
{
    const char *p = *pp;
    if (*p != '"' && *p != '\'') return NULL;
    char quote = *p;
    p++;
    const char *start = p;
    while (*p && *p != quote) {
        if (*p == '\\' && *(p + 1)) p++;
        p++;
    }
    size_t len = (size_t)(p - start);
    char *result = malloc(len + 1);
    if (result) {
        memcpy(result, start, len);
        result[len] = '\0';
    }
    if (*p == quote) p++;
    *pp = p;
    return result;
}

/* Parses plugins.js into enabled/disabled plugin names.
 *
 * Why: load_all must evaluate only enabled plugin scripts, in file order,
 * without running plugins.js a second time. `plugins_js_path` is borrowed.
 * Returns a malloc'd list the caller frees with
 * script_loader_free_plugins, or NULL after reporting a read failure or
 * allocation failure on stderr. Main thread. */
PluginList *script_loader_parse_plugins(const char *plugins_js_path)
{
    if (!plugins_js_path) return NULL;

    size_t file_size = 0;
    char *content = file_io_read_text(plugins_js_path, &file_size);
    if (!content) {
        fprintf(stderr, "script_loader: failed to read plugins file: %s\n", plugins_js_path);
        return NULL;
    }

    PluginList *list = calloc(1, sizeof(PluginList));
    if (!list) {
        free(content);
        return NULL;
    }

    size_t capacity = 64;
    list->entries = malloc(sizeof(PluginEntry) * capacity);
    if (!list->entries) {
        free(list);
        free(content);
        return NULL;
    }
    list->count = 0;

    /* Scan for each {name:"...", status:..., ...} object with a small
       hand-rolled scanner rather than a full JS parser. */
    const char *p = content;
    while (*p) {
        const char *brace = strchr(p, '{');
        if (!brace) break;
        p = brace + 1;

        char *name = NULL;
        bool status = false;
        bool found_name = false;
        bool found_status = false;

        while (*p && *p != '}') {
            p = skip_ws(p);
            if (*p == '}') break;

            /* Key may be quoted or bare. */
            const char *key_start = p;
            char *key = NULL;
            if (*p == '"' || *p == '\'') {
                key = extract_quoted_string(&p);
            } else {
                while (*p && *p != ':' && *p != ',' && *p != '}' && !isspace((unsigned char)*p)) p++;
                size_t klen = (size_t)(p - key_start);
                key = malloc(klen + 1);
                if (key) {
                    memcpy(key, key_start, klen);
                    key[klen] = '\0';
                }
            }

            p = skip_ws(p);
            if (*p == ':') p++;
            p = skip_ws(p);

            if (key && strcmp(key, "name") == 0) {
                name = extract_quoted_string(&p);
                found_name = (name != NULL);
            } else if (key && strcmp(key, "status") == 0) {
                if (strncmp(p, "true", 4) == 0) {
                    status = true;
                    p += 4;
                } else if (strncmp(p, "false", 5) == 0) {
                    status = false;
                    p += 5;
                } else {
                    /* Quoted "true"/"false"/"ON" */
                    char *val = extract_quoted_string(&p);
                    if (val) {
                        status = (strcmp(val, "true") == 0 || strcmp(val, "ON") == 0);
                        free(val);
                    }
                }
                found_status = true;
            } else {
                /* Skip any other value; strings inside arrays/objects are consumed
                   whole so brackets within them are not miscounted. */
                if (*p == '"' || *p == '\'') {
                    char *val = extract_quoted_string(&p);
                    free(val);
                } else if (*p == '[') {
                    int depth = 1;
                    p++;
                    while (*p && depth > 0) {
                        if (*p == '"' || *p == '\'') {
                            char *s = extract_quoted_string(&p);
                            free(s);
                        } else {
                            if (*p == '[') depth++;
                            else if (*p == ']') depth--;
                            p++;
                        }
                    }
                } else if (*p == '{') {
                    int depth = 1;
                    p++;
                    while (*p && depth > 0) {
                        if (*p == '"' || *p == '\'') {
                            char *s = extract_quoted_string(&p);
                            free(s);
                        } else {
                            if (*p == '{') depth++;
                            else if (*p == '}') depth--;
                            p++;
                        }
                    }
                } else {
                    while (*p && *p != ',' && *p != '}') p++;
                }
            }

            free(key);

            p = skip_ws(p);
            if (*p == ',') p++;
        }

        if (*p == '}') p++;

        if (found_name && name) {
            if (list->count >= capacity) {
                capacity *= 2;
                PluginEntry *tmp = realloc(list->entries, sizeof(PluginEntry) * capacity);
                if (!tmp) {
                    free(name);
                    break;
                }
                list->entries = tmp;
            }
            list->entries[list->count].name = name;
            list->entries[list->count].status = found_status ? status : true;
            list->count++;
        } else {
            free(name);
        }
    }

    free(content);
    return list;
}

/* Frees a list from script_loader_parse_plugins and every name in it.
 * NULL is ignored. Main thread; the list must not be used afterwards. */
void script_loader_free_plugins(PluginList *list)
{
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) {
        free(list->entries[i].name);
    }
    free(list->entries);
    free(list);
}

/* Maps a --renderer value to a mode.
 *
 * Why: main.c and the evaluator must accept the same spellings, and a typo
 * must fail loudly instead of silently booting the other renderer. `name`
 * is borrowed; NULL means the default real-PIXI mode, while an empty or
 * unknown name is rejected. `*out` is written only on success. Returns false
 * for an unknown name or NULL `out`; main thread, before any script is
 * loaded. */
bool script_loader_renderer_parse(const char *name, ScriptLoaderRenderer *out)
{
    if (!out) return false;
    if (!name ||
        strcmp(name, "real-pixi") == 0 ||
        strcmp(name, "real_pixi") == 0 ||
        strcmp(name, "realpixi") == 0) {
        *out = SCRIPT_LOADER_RENDERER_REAL_PIXI;
        return true;
    }
    if (strcmp(name, "outsider") == 0) {
        *out = SCRIPT_LOADER_RENDERER_OUTSIDER;
        return true;
    }
    return false;
}

/* Tell the DOM shim a game script (path relative to the game directory)
   has been evaluated, so a <script src> the game inserts for the same file
   later is not run a second time (browsers never run a script twice
   either; FOSSIL and similar loaders re-request the core scripts). */
static void note_script_loaded(JSEngine *engine, const char *rel_path)
{
    char code[1024];
    snprintf(code, sizeof(code),
             "if (typeof __dom_noteScriptLoaded === 'function') __dom_noteScriptLoaded('%s');",
             rel_path);
    js_engine_eval(engine, code, "<note-script-loaded>");
}

/* Shim files loaded before any game scripts in Outsider mode. Order matters. */
static const char *SHIM_FILES[] = {
    "dom_shim.js",
    "canvas2d_shim.js",
    "font_shim.js",
    "navigator_shim.js",
    "xhr_shim.js",
    "storage_shim.js",
    "pixi_shim.js",
    "webaudio_shim.js",
    "effekseer_shim.js",
    "dom_overlay.js",
    "plugin_compat.js",
    NULL
};

/* Pre-core shims in real-PIXI mode. The same list minus pixi_shim.js: the
   game's own js/libs/pixi.js defines PIXI, and the shim would replace it. */
static const char *SHIM_FILES_REAL_PIXI[] = {
    "dom_shim.js",
    "canvas2d_shim.js",
    "font_shim.js",
    "navigator_shim.js",
    "xhr_shim.js",
    "storage_shim.js",
    "webaudio_shim.js",
    "effekseer_shim.js",
    "dom_overlay.js",
    "plugin_compat.js",
    NULL
};

/* Returns the pre-core shim list for RENDERER; never NULL. */
static const char *const *pre_core_shims(ScriptLoaderRenderer renderer)
{
    return renderer == SCRIPT_LOADER_RENDERER_REAL_PIXI ? SHIM_FILES_REAL_PIXI : SHIM_FILES;
}

/* Loads every pre-core shim in RENDERER's order.
 *
 * Why: the compatibility layer must exist before libraries or game scripts
 * touch browser globals, and its own order matters. `engine` and
 * `shim_dir` are borrowed. Returns false at the first shim that cannot be
 * joined or evaluated; the same shims are still evaluated by load_all_for.
 * Main thread, outside any JS call. */
bool script_loader_load_shims_for(JSEngine *engine, const char *shim_dir,
                                  ScriptLoaderRenderer renderer)
{
    if (!engine || !shim_dir) return false;

    const char *const *files = pre_core_shims(renderer);
    for (int i = 0; files[i] != NULL; i++) {
        char *path = join_path(shim_dir, files[i]);
        if (!path) return false;

        bool ok = js_engine_eval_file(engine, path);
        if (!ok) {
            fprintf(stderr, "script_loader: failed to load shim: %s\n", path);
            free(path);
            return false;
        }
        js_engine_execute_pending_jobs(engine);
        free(path);
    }
    return true;
}

/* Outsider-mode wrapper kept for existing callers and tests. */
bool script_loader_load_shims(JSEngine *engine, const char *shim_dir)
{
    return script_loader_load_shims_for(engine, shim_dir,
                                        SCRIPT_LOADER_RENDERER_OUTSIDER);
}

/* RPG Maker MZ core scripts in load order. */
static const char *RMMZ_CORE_SCRIPTS[] = {
    "js/rmmz_core.js",
    "js/rmmz_managers.js",
    "js/rmmz_objects.js",
    "js/rmmz_scenes.js",
    "js/rmmz_sprites.js",
    "js/rmmz_windows.js",
    NULL
};

/* Shims loaded after the core scripts and plugins in Outsider mode, to patch
   RPG Maker classes that depend on PIXI internals the runtime does not
   implement. Real-PIXI mode has no post-core shims (spec §4.2). */
static const char *POST_CORE_SHIMS[] = {
    "tilemap_shim.js",
    NULL
};

/* Returns the post-core shim list for RENDERER; real PIXI loads none. */
static const char *const *post_core_shims(ScriptLoaderRenderer renderer)
{
    static const char *none[] = { NULL };
    return renderer == SCRIPT_LOADER_RENDERER_REAL_PIXI ? none : POST_CORE_SHIMS;
}

/* Returns RENDERER's combined shim file list in load order.
 *
 * Why: --dump-surface and Android extraction need every shim name without
 * loading them. Built once per mode into static storage; the returned array
 * is borrowed and must not be freed. Main thread. */
const char *const *script_loader_shim_files_for(ScriptLoaderRenderer renderer)
{
    static const char *outsider_all[32];
    static const char *real_pixi_all[32];
    static bool built = false;
    const char *const *all = renderer == SCRIPT_LOADER_RENDERER_REAL_PIXI
                                 ? real_pixi_all : outsider_all;

    if (!built) {
        int n = 0;
        for (int i = 0; SHIM_FILES[i] && n < 31; i++) outsider_all[n++] = SHIM_FILES[i];
        for (int i = 0; POST_CORE_SHIMS[i] && n < 31; i++) outsider_all[n++] = POST_CORE_SHIMS[i];
        outsider_all[n] = NULL;

        n = 0;
        for (int i = 0; SHIM_FILES_REAL_PIXI[i] && n < 31; i++) real_pixi_all[n++] = SHIM_FILES_REAL_PIXI[i];
        real_pixi_all[n] = NULL;
        built = true;
    }
    return all;
}

/* Outsider-mode wrapper kept for existing callers and tests. */
const char *const *script_loader_shim_files(void)
{
    return script_loader_shim_files_for(SCRIPT_LOADER_RENDERER_OUTSIDER);
}

/* Library scripts loaded before core in Outsider mode, which supplies its own
   PIXI through pixi_shim.js. */
static const char *LIB_SCRIPTS[] = {
    "js/libs/pako.min.js",
    NULL
};

/* Real-PIXI library scripts: the game's own pixi.js first, then pako, the
   order its main.js uses (spec §4.2 and §13). */
static const char *LIB_SCRIPTS_REAL_PIXI[] = {
    "js/libs/pixi.js",
    "js/libs/pako.min.js",
    NULL
};

/* Returns RENDERER's pre-core library list; never NULL. */
static const char *const *lib_scripts(ScriptLoaderRenderer renderer)
{
    return renderer == SCRIPT_LOADER_RENDERER_REAL_PIXI ? LIB_SCRIPTS_REAL_PIXI : LIB_SCRIPTS;
}

/* Loads the whole game in NW.js order for RENDERER, then starts its first scene.
 *
 * Why: RPG Maker MZ assumes the browser loaded shims, libraries, core
 * scripts, enabled plugins, post-core shims and main.js in this exact
 * order; doing it here keeps that contract in one place. In real-PIXI mode
 * the game's own pixi.js is loaded instead of pixi_shim.js/tilemap_shim.js
 * (spec §4.2). `engine`, `shim_dir` and `game_dir` are borrowed for the
 * call. Returns false for a missing argument or failed shim load; later
 * script and plugin errors are logged and do not stop the route, matching a
 * browser's tolerant `<script>` loading. Main thread, outside any JS call. */
bool script_loader_load_all_for(JSEngine *engine, const char *shim_dir,
                                const char *game_dir, ScriptLoaderRenderer renderer)
{
    if (!engine || !shim_dir || !game_dir) return false;

    /* Outsider keeps getContext('webgl') on the capability stub so startup
       does not create an EGL context beside the GPU window. Real-PIXI opts
       into the native WebGL2 module. */
    if (!js_engine_eval(engine,
            renderer == SCRIPT_LOADER_RENDERER_REAL_PIXI
                ? "globalThis.__SMZUGGLER_WEBGL_NATIVE = true;"
                : "globalThis.__SMZUGGLER_WEBGL_NATIVE = false;",
            "<set-webgl-native>")) {
        return false;
    }

    /* Step 1: Load shims */
    if (!script_loader_load_shims_for(engine, shim_dir, renderer)) {
        return false;
    }
    printf("script_loader: shims loaded\n");

    /* Step 2: Load library scripts (the game's pixi.js in real-PIXI mode). */
    const char *const *libs = lib_scripts(renderer);
    for (int i = 0; libs[i] != NULL; i++) {
        char *path = join_path(game_dir, libs[i]);
        if (!path) return false;

        if (file_io_exists(path)) {
            bool ok = js_engine_eval_file(engine, path);
            if (!ok) {
                fprintf(stderr, "script_loader: warning: failed to load lib: %s\n", path);
            }
            js_engine_execute_pending_jobs(engine);
            note_script_loaded(engine, libs[i]);
        } else {
            fprintf(stderr, "script_loader: warning: lib not found: %s\n", path);
        }
        free(path);
    }
    printf("script_loader: libraries loaded\n");

    /* Step 3: Load RPG Maker MZ core scripts */
    for (int i = 0; RMMZ_CORE_SCRIPTS[i] != NULL; i++) {
        char *path = join_path(game_dir, RMMZ_CORE_SCRIPTS[i]);
        if (!path) return false;

        if (file_io_exists(path)) {
            bool ok = js_engine_eval_file(engine, path);
            if (!ok) {
                /* Non-fatal: core scripts may reference PIXI features the shim lacks. */
                fprintf(stderr, "script_loader: note: errors during %s (may be expected for PIXI deps)\n",
                        RMMZ_CORE_SCRIPTS[i]);
            }
            js_engine_execute_pending_jobs(engine);
            note_script_loaded(engine, RMMZ_CORE_SCRIPTS[i]);
        } else {
            fprintf(stderr, "script_loader: warning: core script not found: %s\n", path);
        }
        free(path);
    }
    printf("script_loader: core scripts loaded\n");

    /* Step 4: Load enabled plugins */
    char *plugins_path = join_path(game_dir, "js/plugins.js");
    if (plugins_path && file_io_exists(plugins_path)) {
        /* Evaluating plugins.js defines $plugins. */
        js_engine_eval_file(engine, plugins_path);
        js_engine_execute_pending_jobs(engine);
        note_script_loaded(engine, "js/plugins.js");

        /* Register every plugin's parameters and name before evaluating any
           plugin script. In a browser PluginManager.setup() does both for all
           plugins before any injected script tag runs, and plugins rely on it:
           they call PluginManager.parameters() at eval time and probe
           PluginManager._scripts to detect sibling plugins. Mirrors setup()
           minus loadScript(), so the later setup() fallback is skipped. */
        js_engine_eval(engine,
            "if (typeof PluginManager !== 'undefined' && typeof $plugins !== 'undefined') {"
            "    for (var _i = 0; _i < $plugins.length; _i++) {"
            "        var _p = $plugins[_i];"
            "        var _n = (typeof Utils !== 'undefined' && Utils.extractFileName)"
            "                 ? Utils.extractFileName(_p.name) : _p.name;"
            "        if (_p.status && !PluginManager._scripts.includes(_n)) {"
            "            PluginManager.setParameters(_n, _p.parameters);"
            "            PluginManager._scripts.push(_n);"
            "        }"
            "    }"
            "}",
            "<plugin-params-setup>");
        js_engine_execute_pending_jobs(engine);

        PluginList *plugins = script_loader_parse_plugins(plugins_path);
        if (plugins) {
            for (size_t i = 0; i < plugins->count; i++) {
                if (!plugins->entries[i].status) continue;

                const char *pname = plugins->entries[i].name;
                if (!is_valid_plugin_name(pname)) {
                    fprintf(stderr, "Skipping plugin with invalid name: %s\n", pname);
                    continue;
                }

                char plugin_file[512];
                snprintf(plugin_file, sizeof(plugin_file),
                         "js/plugins/%s.js", pname);

                char *path = join_path(game_dir, plugin_file);
                if (!path) continue;

                if (file_io_exists(path)) {
                    /* Plugins read document.currentScript.src to find their own
                       name, so set it before eval. The name is escaped because it
                       is spliced into a JS string literal. */
                    char safe_name[512];
                    size_t si = 0;
                    const char *pn = plugins->entries[i].name;
                    for (size_t j = 0; pn[j] && si < sizeof(safe_name) - 2; j++) {
                        char ch = pn[j];
                        if (ch == '\'' || ch == '\\') {
                            safe_name[si++] = '\\';
                            safe_name[si++] = ch;
                        } else if (ch == '\n') {
                            safe_name[si++] = '\\';
                            safe_name[si++] = 'n';
                        } else if (ch == '\r') {
                            safe_name[si++] = '\\';
                            safe_name[si++] = 'r';
                        } else if ((unsigned char)ch < 0x20) {
                            /* drop other control characters */
                        } else {
                            safe_name[si++] = ch;
                        }
                    }
                    safe_name[si] = '\0';

                    char set_script[1024];
                    snprintf(set_script, sizeof(set_script),
                             "__dom_setCurrentScript('js/plugins/%s.js');",
                             safe_name);
                    js_engine_eval(engine, set_script, "<set-current-script>");

                    /* Logged before eval so a native crash inside a plugin
                       leaves its name as the last line of the log. */
                    printf("script_loader: plugin %s\n", plugins->entries[i].name);
                    bool ok = js_engine_eval_file(engine, path);
                    if (!ok) {
                        fprintf(stderr, "script_loader: warning: plugin error: %s\n",
                                plugins->entries[i].name);
                    }
                    js_engine_execute_pending_jobs(engine);

                    js_engine_eval(engine, "__dom_setCurrentScript(null);",
                                   "<clear-current-script>");
                } else {
                    fprintf(stderr, "script_loader: warning: plugin not found: %s\n", path);
                }
                free(path);
            }
            printf("script_loader: %zu plugins processed\n", plugins->count);
            script_loader_free_plugins(plugins);
        }
    }
    free(plugins_path);

    /* Step 4b: Post-core shims go after plugins so they win over plugin
       patches to the same engine classes (e.g. Tilemap.Layer.prototype.render).
       Real-PIXI mode has none. */
    const char *const *post = post_core_shims(renderer);
    for (int i = 0; post[i] != NULL; i++) {
        char *path = join_path(shim_dir, post[i]);
        if (!path) return false;

        bool ok = js_engine_eval_file(engine, path);
        if (!ok) {
            fprintf(stderr, "script_loader: warning: failed to load post-core shim: %s\n", path);
        }
        js_engine_execute_pending_jobs(engine);
        free(path);
    }
    printf("script_loader: post-core shims loaded\n");

    /* Step 5: Load main.js */
    char *main_path = join_path(game_dir, "js/main.js");
    if (main_path && file_io_exists(main_path)) {
        js_engine_eval(engine, "__dom_setCurrentScript('js/main.js');",
                       "<set-main-script>");
        bool ok = js_engine_eval_file(engine, main_path);
        if (!ok) {
            fprintf(stderr, "script_loader: warning: errors during main.js\n");
        }
        js_engine_execute_pending_jobs(engine);
        js_engine_eval(engine, "__dom_setCurrentScript(null);",
                       "<clear-main-script>");
        printf("script_loader: main.js loaded\n");
    }
    free(main_path);

    /* Step 6: Start the game. main.js waits for DOM script-load callbacks that
       never fire here (scripts are pre-loaded), so run PluginManager.setup and
       SceneManager.run ourselves unless main.js already did. */
    js_engine_eval(engine,
        "if (typeof PluginManager !== 'undefined' && typeof $plugins !== 'undefined'"
        "    && !PluginManager._scripts.length) {"
        "    PluginManager.setup($plugins);"
        "}",
        "<plugin-setup>");
    js_engine_execute_pending_jobs(engine);

    js_engine_eval(engine,
        "if (typeof SceneManager !== 'undefined'"
        "    && !SceneManager._scene && !SceneManager._nextScene) {"
        "    if (typeof effekseer !== 'undefined') {"
        "        effekseer.initRuntime('', function() {"
        "            SceneManager.run(Scene_Boot);"
        "        });"
        "    } else {"
        "        SceneManager.run(Scene_Boot);"
        "    }"
        "}",
        "<game-start>");
    js_engine_execute_pending_jobs(engine);

    return true;
}

/* Outsider-mode wrapper kept for existing callers and tests. */
bool script_loader_load_all(JSEngine *engine, const char *shim_dir, const char *game_dir)
{
    return script_loader_load_all_for(engine, shim_dir, game_dir,
                                      SCRIPT_LOADER_RENDERER_OUTSIDER);
}

/* Load order query (for testing) */

static bool append_path(char ***list, size_t *count, size_t *cap, const char *path)
{
    if (*count >= *cap) {
        *cap = (*cap == 0) ? 64 : *cap * 2;
        char **tmp = realloc(*list, sizeof(char *) * (*cap));
        if (!tmp) return false;
        *list = tmp;
    }
    (*list)[*count] = strdup(path);
    if (!(*list)[*count]) return false;
    (*count)++;
    return true;
}

/* Returns the paths load_all_for would use for RENDERER, without evaluating any of them.
 *
 * Why: the evaluator and tests need to assert the browser-compatible load
 * order without a runnable game, including which renderer's scripts appear.
 * `shim_dir` and `game_dir` are borrowed; `*out_count` (may be NULL)
 * receives the number of paths. Returns a malloc'd NULL-terminated array,
 * or NULL on a missing argument or allocation failure; free with
 * script_loader_free_path_list. Main thread. */
char **script_loader_get_load_order_for(ScriptLoaderRenderer renderer,
                                        const char *shim_dir, const char *game_dir,
                                        size_t *out_count)
{
    if (!shim_dir || !game_dir) return NULL;

    char **list = NULL;
    size_t count = 0;
    size_t cap = 0;

    /* 1. Shims */
    const char *const *pre = pre_core_shims(renderer);
    for (int i = 0; pre[i] != NULL; i++) {
        char *path = join_path(shim_dir, pre[i]);
        if (path) {
            append_path(&list, &count, &cap, path);
            free(path);
        }
    }

    /* 2. Libs (the game's pixi.js in real-PIXI mode) */
    const char *const *libs = lib_scripts(renderer);
    for (int i = 0; libs[i] != NULL; i++) {
        char *path = join_path(game_dir, libs[i]);
        if (path) {
            append_path(&list, &count, &cap, path);
            free(path);
        }
    }

    /* 3. Core scripts */
    for (int i = 0; RMMZ_CORE_SCRIPTS[i] != NULL; i++) {
        char *path = join_path(game_dir, RMMZ_CORE_SCRIPTS[i]);
        if (path) {
            append_path(&list, &count, &cap, path);
            free(path);
        }
    }

    /* 3b. Post-core shims */
    const char *const *post = post_core_shims(renderer);
    for (int i = 0; post[i] != NULL; i++) {
        char *path = join_path(shim_dir, post[i]);
        if (path) {
            append_path(&list, &count, &cap, path);
            free(path);
        }
    }

    /* 4. Plugins (from plugins.js if it exists) */
    char *plugins_path = join_path(game_dir, "js/plugins.js");
    if (plugins_path && file_io_exists(plugins_path)) {
        PluginList *plugins = script_loader_parse_plugins(plugins_path);
        if (plugins) {
            for (size_t i = 0; i < plugins->count; i++) {
                if (!plugins->entries[i].status) continue;
                if (!is_valid_plugin_name(plugins->entries[i].name)) continue;
                char plugin_file[512];
                snprintf(plugin_file, sizeof(plugin_file),
                         "js/plugins/%s.js", plugins->entries[i].name);
                char *path = join_path(game_dir, plugin_file);
                if (path) {
                    append_path(&list, &count, &cap, path);
                    free(path);
                }
            }
            script_loader_free_plugins(plugins);
        }
    }
    free(plugins_path);

    /* 5. main.js */
    {
        char *path = join_path(game_dir, "js/main.js");
        if (path) {
            append_path(&list, &count, &cap, path);
            free(path);
        }
    }

    if (out_count) *out_count = count;
    return list;
}

/* Outsider-mode wrapper kept for existing callers and tests. */
char **script_loader_get_load_order(const char *shim_dir, const char *game_dir, size_t *out_count)
{
    return script_loader_get_load_order_for(SCRIPT_LOADER_RENDERER_OUTSIDER,
                                            shim_dir, game_dir, out_count);
}

/* Frees `count` strings and a list from script_loader_get_load_order_for.
 * NULL is ignored. Main thread; the strings must not be used afterwards. */
void script_loader_free_path_list(char **list, size_t count)
{
    if (!list) return;
    for (size_t i = 0; i < count; i++) {
        free(list[i]);
    }
    free(list);
}
