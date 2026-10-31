/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Tests the game-free surface inventory; see docs/shims.md.
 * All streams belong to this main-thread test and are closed before exit.
 */
#include "smzuggler/dump_surface.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/** Capture one dump in an owned string; abort on any engine or stream error. */
static char *capture(void)
{
    FILE *f = tmpfile();
    assert(f && dump_surface(f, SHIM_DIR));
    long size = ftell(f);
    assert(size > 0 && fseek(f, 0, SEEK_SET) == 0);
    char *s = malloc((size_t)size + 1);
    assert(s && fread(s, 1, (size_t)size, f) == (size_t)size);
    s[size] = 0;
    assert(fclose(f) == 0);
    return s;
}

/** Prove known APIs, ordering and repeatability without initializing SDL. */
int main(void)
{
    char *a = capture(), *b = capture();
    assert(strcmp(a, b) == 0);
    assert(!strstr(a, "\nNaN\t"));
    assert(!strstr(a, "__smz_surface"));
    assert(strstr(a, "document.createElement\tfunction\tdom_shim.js\n"));
    assert(strstr(a, "PIXI.Sprite.prototype."));
    const char *names[] = {"document.createElement\t", "PIXI.Sprite\t",
        "nw.Window.get\t", "process.env\t", "navigator.userAgent\t"};
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        assert(strstr(a, names[i]));
    char *previous = NULL;
    for (char *line = strtok(a, "\n"); line; line = strtok(NULL, "\n")) {
        if (previous) assert(strcmp(previous, line) < 0);
        char *tab = strchr(line, '\t');
        assert(tab && strchr(tab + 1, '\t'));
        previous = line;
    }
    free(a);
    free(b);
    return 0;
}
