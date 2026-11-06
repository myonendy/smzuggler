/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Planned script list for the two loader renderer modes (spec §4.2).
 *
 * Why: real PIXI and the Outsider shim are mutually exclusive, but both keep
 * the same NW.js load order around that choice. This test pins the plan
 * without evaluating any game script: real-PIXI must load the game's own
 * js/libs/pixi.js and skip pixi_shim.js/tilemap_shim.js, while
 * --renderer=outsider must keep the pre-WebGL2 list and order. It also pins
 * the accepted --renderer spellings so main.c cannot drift from the loader.
 * Main thread only; no window, GL context or game copy is needed.
 */

#include "engine/script_loader.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* True when NEEDLE occurs in any of the COUNT paths. */
static int order_has(char **order, size_t count, const char *needle)
{
    for (size_t i = 0; i < count; i++) {
        if (strstr(order[i], needle) != NULL) return 1;
    }
    return 0;
}

/* Index of the first path containing NEEDLE, or -1. */
static int order_index(char **order, size_t count, const char *needle)
{
    for (size_t i = 0; i < count; i++) {
        if (strstr(order[i], needle) != NULL) return (int)i;
    }
    return -1;
}

/* Checks that the real-PIXI plan uses the game's PIXI and no PIXI shims. */
static void check_real_pixi_plan(void)
{
    size_t count = 0;
    char **order = script_loader_get_load_order_for(
        SCRIPT_LOADER_RENDERER_REAL_PIXI, "shims", "game", &count);
    assert(order != NULL);

    int pixi = order_index(order, count, "js/libs/pixi.js");
    int pako = order_index(order, count, "js/libs/pako.min.js");
    int core = order_index(order, count, "js/rmmz_core.js");
    int main_js = order_index(order, count, "js/main.js");

    assert(pixi >= 0);
    assert(!order_has(order, count, "pixi_shim.js"));
    assert(!order_has(order, count, "tilemap_shim.js"));
    assert(pako >= 0);
    assert(core >= 0);
    assert(main_js >= 0);
    assert(pixi < pako);
    assert(pako < core);
    assert(core < main_js);

    script_loader_free_path_list(order, count);
}

/* Checks that the Outsider plan keeps both shims and does not force PIXI in. */
static void check_outsider_plan(void)
{
    size_t count = 0;
    char **order = script_loader_get_load_order_for(
        SCRIPT_LOADER_RENDERER_OUTSIDER, "shims", "game", &count);
    assert(order != NULL);

    assert(order_has(order, count, "pixi_shim.js"));
    assert(order_has(order, count, "tilemap_shim.js"));
    assert(!order_has(order, count, "js/libs/pixi.js"));

    script_loader_free_path_list(order, count);
}

/* The legacy query must stay byte-for-byte the Outsider plan. */
static void check_outsider_matches_legacy(void)
{
    size_t legacy_count = 0;
    size_t mode_count = 0;
    char **legacy = script_loader_get_load_order("shims", "game", &legacy_count);
    char **mode = script_loader_get_load_order_for(
        SCRIPT_LOADER_RENDERER_OUTSIDER, "shims", "game", &mode_count);
    assert(legacy != NULL);
    assert(mode != NULL);
    assert(legacy_count == mode_count);
    for (size_t i = 0; i < legacy_count; i++) {
        assert(strcmp(legacy[i], mode[i]) == 0);
    }
    script_loader_free_path_list(legacy, legacy_count);
    script_loader_free_path_list(mode, mode_count);
}

/* The combined shim inventory follows the same mode split. */
static void check_shim_inventory(void)
{
    const char *const *real = script_loader_shim_files_for(SCRIPT_LOADER_RENDERER_REAL_PIXI);
    const char *const *outsider = script_loader_shim_files_for(SCRIPT_LOADER_RENDERER_OUTSIDER);
    int real_pixi = 0, real_tilemap = 0, out_pixi = 0, out_tilemap = 0;
    for (int i = 0; real[i]; i++) {
        if (strcmp(real[i], "pixi_shim.js") == 0) real_pixi = 1;
        if (strcmp(real[i], "tilemap_shim.js") == 0) real_tilemap = 1;
    }
    for (int i = 0; outsider[i]; i++) {
        if (strcmp(outsider[i], "pixi_shim.js") == 0) out_pixi = 1;
        if (strcmp(outsider[i], "tilemap_shim.js") == 0) out_tilemap = 1;
    }
    assert(!real_pixi);
    assert(!real_tilemap);
    assert(out_pixi);
    assert(out_tilemap);
}

/* Checks every accepted --renderer spelling and the rejected ones. */
static void check_renderer_names(void)
{
    ScriptLoaderRenderer mode = SCRIPT_LOADER_RENDERER_OUTSIDER;
    assert(script_loader_renderer_parse("outsider", &mode));
    assert(mode == SCRIPT_LOADER_RENDERER_OUTSIDER);

    assert(script_loader_renderer_parse("real-pixi", &mode));
    assert(mode == SCRIPT_LOADER_RENDERER_REAL_PIXI);

    assert(script_loader_renderer_parse(NULL, &mode));
    assert(mode == SCRIPT_LOADER_RENDERER_REAL_PIXI);

    mode = SCRIPT_LOADER_RENDERER_OUTSIDER;
    assert(!script_loader_renderer_parse("", &mode));
    assert(mode == SCRIPT_LOADER_RENDERER_OUTSIDER);

    assert(!script_loader_renderer_parse("vulkan", &mode));
    assert(mode == SCRIPT_LOADER_RENDERER_OUTSIDER);
}

/* Runs every planned-list and renderer-name check. */
int main(void)
{
    check_real_pixi_plan();
    check_outsider_plan();
    check_outsider_matches_legacy();
    check_shim_inventory();
    check_renderer_names();

    printf("real_pixi_loader: all checks passed\n");
    return 0;
}
