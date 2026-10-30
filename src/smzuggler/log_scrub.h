/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Diagnostic privacy boundary; see docs/privacy.md. Replaces private home paths
 * before engine logging; callers own buffers and must serialize environment changes.
 */
#ifndef SMZUGGLER_LOG_SCRUB_H
#define SMZUGGLER_LOG_SCRUB_H
#include <stddef.h>

/* Copy a NUL-terminated diagnostic into out (n bytes), replacing home path
 * prefixes with ~. No allocation or retained pointers; buffers must not overlap.
 * Any thread may call with stable environment; n == 0 permits NULL out.
 * Output is NUL-terminated when n > 0, truncating safely if necessary.
 */
void log_scrub(const char *in, char *out, size_t n);
#endif
