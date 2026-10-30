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

/* ASCII filename continuations preserve sibling paths without locale dependence.
 * Accepts a byte; returns a boolean, keeps no state and is safe on any thread.
 */
static int alphanumeric(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9');
}

/* Fold Windows path bytes without allocating or depending on the process locale.
 * Accepts/returns one byte; callable on any thread with no retained state.
 */
static unsigned char path_byte(unsigned char c)
{
#ifdef _WIN32
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
#endif
    return c;
}

/* Match a complete home prefix, preserving sibling paths with similar names.
 * Borrows NUL-terminated strings and positive len bytes of home; returns 0 or 1.
 * Any thread with stable environment may call; no pointers are retained.
 */
static int home_match(const char *in, const char *home, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        if (!in[i]) return 0;
        if (path_byte(in[i]) != path_byte(home[i]) &&
            !(separator(in[i]) && separator(home[i])))
            return 0;
    }
    unsigned char next = (unsigned char)in[len];
    return !(alphanumeric(next) || next == '-' || next == '_' ||
             (next == '.' && alphanumeric((unsigned char)in[len + 1])));
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
    if ((len == 1 && separator(home[0])) ||
        (len == 2 && alphanumeric(home[0]) && home[1] == ':')) len = 0;
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
