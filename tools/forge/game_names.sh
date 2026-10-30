#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# forge `names` gate (spec §13): the game's own names and IDs appear in source
# code only in src/smzuggler/game_profile.c. Docs and the spec may name the
# game; code may not spread game-specific values around.
set -euo pipefail
root=$(git rev-parse --show-toplevel)
cd "$root"
pattern='HIROTONFA|AmericanRevolution|American Revolution|3789080'
hits=$(git grep -nIiE "$pattern" -- 'src/**' 'tools/**' 'tests/**' 'CMakeLists.txt' 'third_party/**' \
  ':!src/smzuggler/game_profile.c' ':!tools/forge/game_names.sh' || true)
if [ -n "$hits" ]; then
  echo "names: game-specific names/IDs outside src/smzuggler/game_profile.c (spec §13):" >&2
  echo "$hits" | cut -d: -f1,2 >&2
  exit 1
fi
