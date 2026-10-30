/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Verify the build identity shared by the runtime and platform adapter.
 * See docs/DECISIONS.md (DEC-M0-T001); no platform resources are created.
 */
#include "smzuggler/names.h"
#include "platform/platform.h"
#include <stdio.h>
#include <string.h>

/** Run without platform initialization on any thread; return nonzero on mismatch. */
int main(void)
{
    if (strcmp(SMZUGGLER_NAME, EXPECTED_NAME) != 0 ||
        strcmp(SMZUGGLER_BIN, EXPECTED_BIN) != 0 ||
        strcmp(RMMZ_APP_NAME, SMZUGGLER_NAME) != 0) {
        fputs("Runtime naming constants disagree with the build identity\n", stderr);
        return 1;
    }
    return 0;
}
