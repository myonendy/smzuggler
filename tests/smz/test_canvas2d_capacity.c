/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Keep late-created HUD/message canvases usable beyond the former 256 slots.
 * See docs/canvas2d-capacity.md. Single-threaded; all handles are test-owned.
 */
#include "rendering/canvas2d.h"

#include <assert.h>
#include <stdio.h>

/* Growth must preserve pixels and saved drawing state of existing handles. */
static void check_growth(void)
{
    Canvas2DHandle handles[1025];
    for (size_t i = 0; i < 1025; i++) {
        handles[i] = canvas2d_create(4, 4);
        if (handles[i] == CANVAS2D_HANDLE_INVALID) {
            fprintf(stderr, "Canvas allocation failed at live context %zu\n", i + 1);
        }
        assert(handles[i] != CANVAS2D_HANDLE_INVALID);
        canvas2d_set_fill_color(handles[i], (uint8_t)i, 80, 160, 255);
        canvas2d_fill_rect(handles[i], 0, 0, 4, 4);
        canvas2d_save(handles[i]);
        canvas2d_set_fill_color(handles[i], 0, 0, 0, 0);
    }
    assert(canvas2d_context_count() == 1025);
    for (size_t i = 0; i < 1025; i++) {
        uint8_t *pixels = canvas2d_get_pixels(handles[i]);
        assert(pixels && pixels[0] == (uint8_t)i && pixels[3] == 255);
        canvas2d_clear_rect(handles[i], 0, 0, 4, 4);
        canvas2d_restore(handles[i]);
        canvas2d_fill_rect(handles[i], 0, 0, 4, 4);
        assert(pixels[0] == (uint8_t)i && pixels[1] == 80 && pixels[3] == 255);
        canvas2d_destroy(handles[i]);
        assert(canvas2d_get_pixels(handles[i]) == NULL);
    }
    assert(canvas2d_context_count() == 0);
    Canvas2DHandle reused = canvas2d_create(2, 3);
    assert(reused != CANVAS2D_HANDLE_INVALID && reused != handles[0]);
    assert(canvas2d_get_pixels(reused)[3] == 0);
    /* Shutdown must release both registry capacity and still-live canvases. */
}

/* A second lifecycle must work after the expanded registry has been released. */
int main(void)
{
    for (int cycle = 0; cycle < 2; cycle++) {
        canvas2d_init();
        check_growth();
        canvas2d_shutdown();
        assert(canvas2d_context_count() == 0);
    }
    puts("Canvas2D capacity, state preservation and lifecycle passed");
    return 0;
}
