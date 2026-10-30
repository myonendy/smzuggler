#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# forge `test` gate: run the full ctest suite of the build/ build, plus
# the Python unit tests of the repo's own tools.
#
# Why: this is the game-free suite. Tests that need the game read its path
# from SMZUGGLER_GAME_DIR (or Outsider's RMMZ_TEST_GAME_DIR) and are run by
# the separate game gates; here those variables are cleared so the result
# never depends on what is installed on the machine. Nothing is skipped by
# this script; ctest's own result decides.
set -euo pipefail
root=$(git rev-parse --show-toplevel)
bdir=${SMZ_BUILD_DIR:-$root/build}
[ -f "$bdir/CTestTestfile.cmake" ] || { echo "test: no build at $bdir; run tools/forge/build.sh" >&2; exit 1; }
unset SMZUGGLER_GAME_DIR RMMZ_TEST_GAME_DIR
ctest --test-dir "$bdir" --output-on-failure -j "${SMZ_JOBS:-4}" --timeout 600
cd "$root"
# Python tool tests: tools/privacy (setup, protected) and tests/tools (cards).
# Each directory is its own discovery root; tests reach the tools through
# subprocess or a sys.path entry for the repo root.
rc=0
for d in tools/privacy tests/tools; do
    [ -d "$d" ] || continue
    python3 -m unittest discover -s "$d" -p 'test_*.py' || rc=1
done
exit "$rc"
