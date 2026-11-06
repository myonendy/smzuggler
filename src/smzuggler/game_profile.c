/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Game-specific fixture map ids for the §14 perf harness (spec §14.10).
 * AGENTS.md §4 keeps the game's own names and ids in this file only.
 */
#include <stddef.h>

/** A perf fixture and the map ids its scene accepts; maps[1] is 0 when only
 * one map is valid. */
typedef struct {
    const char *name;
    int maps[2];
} PerfFixture;

/* S2, S3, S8 and S9 start from the map a new game also uses. */
static const PerfFixture smz_game_fixtures[] = {
    {"fx-map92", {92, 0}},
    {"fx-trade", {92, 93}},
    {"fx-combat", {55, 0}}
};

static const size_t smz_game_fixture_count =
    sizeof(smz_game_fixtures) / sizeof(smz_game_fixtures[0]);
