/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * CLI, fixture and scratch helpers for the §14 perf harness.
 * See docs/perf.md and docs/testing-fixtures.md. Callers own output buffers.
 */
#ifndef SMZUGGLER_PERF_SUITE_H
#define SMZUGGLER_PERF_SUITE_H

#include <stdbool.h>
#include <stddef.h>

/** Report profile (spec §14.7, §14.9); it decides the gate word, not limits. */
typedef enum {
    SMZ_PERF_PROFILE_DEV_LAPTOP = 0, /* operative battery gate */
    SMZ_PERF_PROFILE_STEAM_DECK,     /* confirmation pass */
    SMZ_PERF_PROFILE_DECK_PROXY,     /* optional proxy */
    SMZ_PERF_PROFILE_EXTRA           /* AC/balanced extra report */
} SmzPerfProfile;

/** Parsed harness options; strings borrow argv. */
typedef struct {
    bool requested;
    bool import_requested;
    SmzPerfProfile profile;
    const char *frame_stats_dir;
    int import_slot;
    const char *import_fixture;
} SmzPerfSuiteOptions;

/** Parse and validate the harness options. Returns 0, or 2 with one clear
 * line in ERR. ARGV is borrowed; OUT is zeroed first. Main thread only. */
int smz_perf_suite_parse_args(int argc, char **argv, SmzPerfSuiteOptions *out,
                              char *err, size_t err_size);

/** `dev-laptop`, `steam-deck`, `deck-proxy` or `extra`; never identity. */
const char *smz_perf_profile_name(SmzPerfProfile profile);

/** True for one of the three spec §14.10 fixture names. */
bool smz_perf_fixture_name_valid(const char *name);

/** `$XDG_DATA_HOME/smzuggler-rt/fixtures` (or the home fallback); no mkdir. */
bool smz_perf_fixtures_dir(char *out, size_t out_size);

/** `$XDG_DATA_HOME/smzuggler-rt/scratch` (or the home fallback); no mkdir. */
bool smz_perf_scratch_dir(char *out, size_t out_size);

/** Read a JSON save's `_mapId` (then `mapId`). Returns 0, or 1 with ERR. */
int smz_perf_read_save_map_id(const char *path, int *map_id, char *err,
                              size_t err_size);

/** Copy one read-only save slot into the fixture directory, refusing a
 * missing slot or a save the fixture's scene does not accept. 0 or 1 + ERR. */
int smz_perf_import_fixture(const SmzPerfSuiteOptions *options, char *err,
                            size_t err_size);

/** Check that one fixture exists and is on an accepted map. 0 or 1 + ERR. */
int smz_perf_validate_fixture(const char *name, char *err, size_t err_size);

#endif /* SMZUGGLER_PERF_SUITE_H */
