/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Headless shim inventory entry points; see docs/shims.md.
 * Calls own their isolated engine and borrow streams on the calling thread.
 */
#ifndef SMZ_DUMP_SURFACE_H
#define SMZ_DUMP_SURFACE_H
#include <stdbool.h>
#include <stdio.h>

/** Write sorted name/type/source rows to borrowed output using shims in the
 * borrowed directory. Main thread only; false means evaluation or I/O failed. */
bool dump_surface(FILE *output, const char *shim_dir);

/** Handle CLI output ownership on the main thread. NULL selects stdout;
 * otherwise the borrowed path is opened for writing. Return a process status. */
int dump_surface_run(const char *path, const char *shim_dir);
#endif
