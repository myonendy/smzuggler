/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Repeated gradient-style calls must not scan every previously created canvas.
 * Game-free, single runtime thread. See docs/canvas2d-lookup.md.
 */
#include "rendering/canvas2d.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

static double repeated_draws(Canvas2DHandle h)
{
    clock_t start = clock();
    for (int i = 0; i < 100000; i++) {
        canvas2d_set_fill_color(h, 40, 80, 120, 255);
        canvas2d_fill_rect(h, 0, 0, 1, 1);
    }
    assert(canvas2d_get_pixels(h)[0] == 40);
    return (double)(clock() - start) / CLOCKS_PER_SEC;
}

static void check_lookup_cost(void)
{
    Canvas2DHandle handles[4096];
    handles[0] = canvas2d_create(1, 1);
    assert(handles[0]);
    double sparse = repeated_draws(handles[0]);
    for (int i = 1; i < 4096; i++) {
        handles[i] = canvas2d_create(1, 1);
        assert(handles[i]);
    }
    /* CPU time, not wall time; allow substantial noise without allowing a
       4096-entry linear search on every set-color and one-pixel draw. */
    double dense = repeated_draws(handles[4095]);
    printf("Repeated canvas draws: sparse %.6fs, dense %.6fs\n", sparse, dense);
    assert(dense < sparse * 12.0 + 0.002);
    /* A cached slot must survive realloc, but not alias a destroyed handle. */
    Canvas2DHandle extra = canvas2d_create(2, 2);
    assert(extra);
    repeated_draws(handles[4095]);
    canvas2d_destroy(handles[4095]);
    assert(canvas2d_get_pixels(handles[4095]) == NULL);
    Canvas2DHandle reused = canvas2d_create(1, 1);
    assert(reused && reused != handles[4095]);
    canvas2d_set_fill_color(handles[4095], 255, 255, 255, 255);
    canvas2d_fill_rect(handles[4095], 0, 0, 1, 1);
    assert(canvas2d_get_pixels(reused)[3] == 0);
    repeated_draws(reused);
    canvas2d_resize(reused, 2, 3);
    assert(canvas2d_get_pixels(reused)[3] == 0);
    repeated_draws(reused);
    assert(canvas2d_get_pixels(CANVAS2D_HANDLE_INVALID) == NULL);
    for (int i = 0; i < 4095; i++) canvas2d_destroy(handles[i]);
    canvas2d_destroy(reused);
    canvas2d_destroy(extra);
    assert(canvas2d_context_count() == 0);
}

int main(void)
{
    for (int pass = 0; pass < 2; pass++) {
        canvas2d_init();
        check_lookup_cost();
        canvas2d_shutdown();
        assert(canvas2d_get_pixels(CANVAS2D_HANDLE_INVALID) == NULL);
    }
    puts("canvas2d lookup regression passed");
    return 0;
}
