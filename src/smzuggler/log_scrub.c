/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Allocation-free diagnostic privacy boundary for engine logging.
 * See docs/privacy.md; environment changes must be serialized by the caller.
 */
#include "smzuggler/log_scrub.h"
#include <stdlib.h>
#include <string.h>

/* Both separators are accepted so foreign-style paths cannot bypass filtering. */
static int separator(char c)
{
    return c == '/' || c == '\\';
}

/* Match a complete home prefix, preserving sibling paths with similar names.
 * Borrows NUL-terminated strings and positive len bytes of home; returns 0 or 1.
 * Any thread with stable environment may call; no pointers are retained.
 */
static int home_match(const char *in, const char *home, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        if (!in[i]) return 0;
        if (in[i] != home[i] && !(separator(in[i]) && separator(home[i])))
            return 0;
    }
    return !in[len] || separator(in[len]) || separator(home[len - 1]) ||
           strchr(" \t\r\n\"':;,)]}", in[len]) != NULL;
}

/* See the public header for buffer ownership, threading and truncation contract. */
void log_scrub(const char *in, char *out, size_t n)
{
#ifdef _WIN32
    const char *home = getenv("USERPROFILE");
#else
    const char *home = getenv("HOME");
#endif
    size_t len = home ? strlen(home) : 0;
    while (len > 1 && separator(home[len - 1])) --len;
    if (!n) return;
    size_t used = 0;
    while (*in && used < n - 1) {
        if (len && home_match(in, home, len)) {
            out[used++] = '~';
            in += len;
        } else {
            out[used++] = *in++;
        }
    }
    out[used] = '\0';
}
