#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# forge `build` gate: configure and build smzuggler with its tests, offline.
#
# Why: every forge worktree builds in its own build/ directory, so parallel
# cards never share a build dir. build/ (not a subdirectory) because
# upstream's acceptance test looks for the binary at build/<name>. Dependencies come from a local
# source cache (SMZ_DEPS_DIR, default ~/.cache/smzuggler-dev/deps/<name>-src)
# with FetchContent fully disconnected, so a gate never touches the network
# and every card builds the same pinned sources. Fails loudly if the cache is
# missing; it never falls back to downloading.
#
# -Wno-error=discarded-qualifiers: QuickJS v0.9.0 builds itself with -Werror
# and GCC 16 rejects one of its const casts (quickjs.c:26366). This only stops
# that warning from being an error; it is still printed. See DEC-SETUP-003.
set -euo pipefail
root=$(git rev-parse --show-toplevel)
deps=${SMZ_DEPS_DIR:-$HOME/.cache/smzuggler-dev/deps}
bdir=${SMZ_BUILD_DIR:-$root/build}
jobs=${SMZ_JOBS:-4}
args=()
for pair in sdl3:SDL3 quickjs:QUICKJS soloud:SOLOUD glslang:GLSLANG spirv_cross:SPIRV_CROSS sdl_shadercross:SDL_SHADERCROSS; do
  name=${pair%%:*}; upper=${pair##*:}
  src="$deps/$name-src"
  if [ ! -d "$src" ]; then
    echo "build: missing dependency cache $src (see docs/FORGE.md, 'Dependency cache')" >&2
    exit 1
  fi
  args+=("-DFETCHCONTENT_SOURCE_DIR_${upper}=$src")
done
mkdir -p "$bdir"
gen=()
if [ ! -f "$bdir/CMakeCache.txt" ] && command -v ninja >/dev/null; then gen=(-G Ninja); fi
cmake -S "$root" -B "$bdir" "${gen[@]}" -DCMAKE_BUILD_TYPE=Release -DRMMZ_BUILD_TESTS=ON \
  -DFETCHCONTENT_FULLY_DISCONNECTED=ON -DCMAKE_C_FLAGS=-Wno-error=discarded-qualifiers \
  "${args[@]}" > "$bdir/forge-configure.log" 2>&1 || { tail -40 "$bdir/forge-configure.log"; exit 1; }
# grep exits 1 when an up-to-date build prints only progress lines, so its
# status is ignored; the build's own status is what the gate reports.
set +o pipefail
nice cmake --build "$bdir" -j "$jobs" 2>&1 | { grep -Ev '^\[[0-9]+/[0-9]+\] ' || true; } | tail -200
exit "${PIPESTATUS[0]}"
