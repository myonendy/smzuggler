/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Pins what the surface dump counts as a member (DEC-M0-T009b, spec §15.4):
 * no intrinsic function properties and no members of DOM element instances
 * reached from document, while real APIs stay. See docs/shims.md.
 * All streams and buffers belong to this main-thread test and are freed.
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

/** True when the borrowed dump has a row for exactly `path` (any kind/file). */
static int has_row(const char *dump, const char *path)
{
    size_t n = strlen(path);
    for (const char *p = dump; (p = strstr(p, path)); p += n)
        if ((p == dump || p[-1] == '\n') && p[n] == '\t') return 1;
    return 0;
}

/** True when the borrowed dump records `path` (length `n`) as a function. */
static int is_function_row(const char *dump, const char *path, size_t n)
{
    for (const char *p = dump; *p; p = strchr(p, '\n') + 1) {
        if (strncmp(p, path, n) == 0 && p[n] == '\t')
            return strncmp(p + n + 1, "function\t", 9) == 0;
    }
    return 0;
}

/** Fail if a row is a function's intrinsic length, name or prototype. */
static void check_no_intrinsic(const char *dump)
{
    static const char *const tails[] = {".length\t", ".name\t", ".prototype\t"};
    for (const char *line = dump; *line; line = strchr(line, '\n') + 1) {
        size_t path_len = strcspn(line, "\t");
        for (size_t i = 0; i < 3; ++i) {
            size_t t = strlen(tails[i]) - 1;
            if (path_len <= t || strncmp(line + path_len - t, tails[i], t + 1) != 0)
                continue;
            assert(!is_function_row(dump, line, path_len - t));
        }
    }
}

/** Prove both exclusions and that the named APIs survive them. */
int main(void)
{
    char *a = capture();
    check_no_intrinsic(a);
    assert(!strstr(a, "\ndocument.body."));
    assert(!strstr(a, "\ndocument.head."));
    assert(!strstr(a, "\ndocument.documentElement."));
    const char *kept[] = {"document.body", "document.head",
        "document.documentElement", "document.createElement", "PIXI.Sprite",
        "PIXI.Sprite.prototype.destroy", "nw.Window.get", "process.env",
        "navigator.userAgent"};
    for (size_t i = 0; i < sizeof(kept) / sizeof(kept[0]); ++i)
        assert(has_row(a, kept[i]));
    assert(!has_row(a, "PIXI.Sprite.prototype"));
    assert(!has_row(a, "PIXI.Sprite.length"));
    free(a);
    return 0;
}
