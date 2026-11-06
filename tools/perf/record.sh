#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Record a perf call-graph profile of the runtime with V8 JIT symbols
# (spec section 6, card M1-T029).
#
# Why: `perf record` alone sees JIT frames as anonymous addresses. V8's
# --perf-prof writes jit-<pid>.dump next to the working directory, and
# `perf inject --jit` folds those records into the profile so shim and game
# JavaScript separate from native code. This script keeps that recipe in one
# place: it validates nothing (the runtime's --js-flags allow-list does), runs
# perf, moves the JIT dump into the output directory and injects it.
# See docs/perf.md, "Recording a V8 profile".
#
# Usage:
#   tools/perf/record.sh --out DIR [--js-flags "FLAGS"] -- COMMAND [ARGS...]
#
# Run it from the repository root with the runtime command after `--`, for
# example `./build/smzuggler-rt --game /path/to/game`. Outputs under DIR are
# run.perf.data, run.jit.perf.data (when a JIT dump exists) and run.perf.txt.
# Never commit a captured profile or a game path.

set -eu

# Prints the usage block from this file's header, so the recipe and the
# argument list never drift apart. Reads only this script; exits 0.
usage()
{
    sed -n 's/^# \{0,1\}//p' "$0" | sed -n '/^Usage:/,/^Never commit/p'
}

out=perf-run
js_flags=--perf-prof

while [ "$#" -gt 0 ]; do
    case "$1" in
        --out)
            [ "$#" -ge 2 ] || { echo "record.sh: --out needs a directory" >&2; exit 2; }
            out=$2
            shift 2
            ;;
        --out=*)
            out=${1#*=}
            shift
            ;;
        --js-flags)
            [ "$#" -ge 2 ] || { echo "record.sh: --js-flags needs a value" >&2; exit 2; }
            js_flags=$2
            shift 2
            ;;
        --js-flags=*)
            js_flags=${1#*=}
            shift
            ;;
        --)
            shift
            break
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            break
            ;;
    esac
done

if [ "$#" -eq 0 ]; then
    echo "record.sh: give the runtime command after --" >&2
    exit 2
fi

if ! command -v perf >/dev/null 2>&1; then
    echo "record.sh: perf is not on PATH" >&2
    exit 1
fi

for arg in "$@"; do
    case "$arg" in
        --js-flags|--js-flags=*)
            echo "record.sh: pass --js-flags to record.sh, not to the runtime" >&2
            exit 2
            ;;
    esac
done

mkdir -p "$out"
tmp_before=${TMPDIR:-/tmp}/smz-jit-before.$$
trap 'rm -f "$tmp_before"' EXIT HUP INT TERM

# Note dumps that already exist so only this run's file is moved.
for file in jit-*.dump; do
    [ -e "$file" ] && printf '%s\n' "$file"
done > "$tmp_before"

if ! perf record -g --call-graph dwarf -e cycles:u -F 499 \
        -o "$out/run.perf.data" -- "$@" "--js-flags=$js_flags"; then
    echo "record.sh: perf record failed" >&2
    exit 1
fi

for file in jit-*.dump; do
    [ -e "$file" ] || continue
    if ! grep -Fqx "$file" "$tmp_before"; then
        mv "$file" "$out/"
    fi
done

profile=run.perf.data
if [ -n "$(find "$out" -maxdepth 1 -name 'jit-*.dump' -print -quit 2>/dev/null)" ]; then
    # perf inject looks for jit-*.dump in its working directory.
    (cd "$out" && perf inject --jit -i run.perf.data -o run.jit.perf.data)
    profile=run.jit.perf.data
fi

perf script -i "$out/$profile" > "$out/run.perf.txt"

echo "record.sh: wrote $out/run.perf.txt (attribution: tools/perf/attribute.py)"
echo "record.sh: scan reports with tools/privacy/scan.py before sharing"
