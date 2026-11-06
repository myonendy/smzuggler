#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Apply the optional Deck-proxy caps from spec §14.9, or undo them.
#
# Why: until a real Deck-equivalent report exists, the owner can run the same
# commit under Deck-like CPU and GPU limits on dev-laptop as an early warning.
# This is a proxy. Reports use profile: deck-proxy and gate: proxy, and the
# analyser never turns one into a Deck pass. The laptop gate stays operative.
# See tools/perf/deck-proxy.md and docs/perf.md.
#
# Rules a reader must not break:
#   * Every write is recorded first and undone by --restore.
#   * Never touch the game or a save; this script changes host cap files only.
#   * The numbers are first guesses until calibrated against a real Deck run.
#
# Usage: sudo tools/perf/deck-proxy.sh [--run -- COMMAND...]
#        sudo tools/perf/deck-proxy.sh --restore
#        tools/perf/deck-proxy.sh --help
#
# --run drops back to the sudo caller before starting COMMAND, so the game's
# HOME and XDG state go to that user's directories, not root's. When the
# script is started as root directly (no SUDO_USER) COMMAND runs as root.

set -eu

CPU_LIST=${DECK_PROXY_CPU_LIST:-0-7}
CPU_MAX_KHZ=${DECK_PROXY_CPU_MAX_KHZ:-2800000}
GPU_MAX_MHZ=${DECK_PROXY_GPU_MAX_MHZ:-1250}
SYSFS=${DECK_PROXY_SYSFS:-/sys}
STATE_DIR=${DECK_PROXY_STATE_DIR:-/run/smzuggler-deck-proxy}

# Print the supported commands and the never-a-pass label.
usage() {
    cat <<'USAGE'
deck-proxy: optional Deck-like caps on dev-laptop (spec §14.9).

  sudo tools/perf/deck-proxy.sh
      Cap CPU at 2.8 GHz and the GPU at 1250 MHz, set balanced power, and
      print the taskset/gamescope command. Every change is recorded.

  sudo tools/perf/deck-proxy.sh --run -- COMMAND...
      Apply the caps, then run COMMAND under taskset -c 0-7.

  sudo tools/perf/deck-proxy.sh --restore
      Undo every recorded change. Safe to run when nothing was applied.

  tools/perf/deck-proxy.sh --help
      Show this text.

This is a proxy, not a pass. Reports say profile: deck-proxy, gate: proxy.
USAGE
}

# Refuse to change host settings unless the caller has root.
need_root() {
    if [ "$(id -u)" -ne 0 ]; then
        printf '%s\n' 'deck-proxy: run with sudo (or as root) for this step' >&2
        exit 2
    fi
}

# Create the directory that records the values to restore.
prepare_state() {
    mkdir -p "$STATE_DIR"
    : >"$STATE_DIR/paths"
}

# Turn an absolute sysfs path into a safe state-file name.
state_key() {
    printf '%s' "$1" | tr '/ ' '__'
}

# Record one file's current value once, so a repeated run cannot lose the original.
save_value() {
    path=$1
    [ -r "$path" ] || return 0
    key=$(state_key "$path")
    if [ ! -f "$STATE_DIR/$key" ]; then
        cat "$path" >"$STATE_DIR/$key"
        printf '%s\n' "$path" >>"$STATE_DIR/paths"
    fi
}

# Put back every recorded value; a missing state tree means nothing to undo.
restore_values() {
    [ -f "$STATE_DIR/paths" ] || return 0
    sort -u "$STATE_DIR/paths" | while read -r path; do
        key=$(state_key "$path")
        if [ -f "$STATE_DIR/$key" ]; then
            cat "$STATE_DIR/$key" >"$path" 2>/dev/null ||
                printf 'deck-proxy: could not restore %s\n' "$path" >&2
        fi
    done
    if [ -f "$STATE_DIR/power-profile" ]; then
        command -v powerprofilesctl >/dev/null 2>&1 &&
            powerprofilesctl set "$(cat "$STATE_DIR/power-profile")" ||
            printf '%s\n' 'deck-proxy: power profile not restored' >&2
    fi
}

# Cap every cpufreq policy's maximum at the Deck-proxy clock.
cap_cpu() {
    found=0
    for policy in "$SYSFS"/devices/system/cpu/cpufreq/policy*; do
        [ -d "$policy" ] || continue
        found=1
        save_value "$policy/scaling_max_freq"
        if [ -w "$policy/scaling_max_freq" ]; then
            printf '%s\n' "$CPU_MAX_KHZ" >"$policy/scaling_max_freq" 2>/dev/null ||
                printf 'deck-proxy: could not cap %s\n' "$policy" >&2
        fi
    done
    [ "$found" -eq 1 ] ||
        printf '%s\n' 'deck-proxy: no cpufreq policies found; CPU cap skipped' >&2
}

# Cap the first Intel gt_max_freq_mhz node; other GPUs have no such file.
cap_gpu() {
    found=0
    for node in "$SYSFS"/class/drm/card*/gt_max_freq_mhz; do
        [ -e "$node" ] || continue
        found=1
        save_value "$node"
        if [ -w "$node" ]; then
            printf '%s\n' "$GPU_MAX_MHZ" >"$node" 2>/dev/null ||
                printf 'deck-proxy: could not cap %s\n' "$node" >&2
        fi
        break
    done
    [ "$found" -eq 1 ] ||
        printf '%s\n' 'deck-proxy: no Intel gt_max_freq_mhz found; GPU cap skipped' >&2
}

# Select balanced power and warn when the machine is on battery.
cap_power() {
    if command -v powerprofilesctl >/dev/null 2>&1; then
        current=$(powerprofilesctl get 2>/dev/null || true)
        [ -n "$current" ] && printf '%s\n' "$current" >"$STATE_DIR/power-profile"
        powerprofilesctl set balanced 2>/dev/null ||
            printf '%s\n' 'deck-proxy: could not select balanced power' >&2
    else
        printf '%s\n' 'deck-proxy: powerprofilesctl missing; select balanced manually' >&2
    fi
    online=unknown
    for node in "$SYSFS"/class/power_supply/A*/online; do
        [ -e "$node" ] || continue
        online=$(cat "$node")
        break
    done
    if [ "$online" = "0" ]; then
        printf '%s\n' 'deck-proxy: on battery; plug in AC for a comparable proxy' >&2
    fi
}

# Print the matching gamescope/taskset command and the proxy labels.
print_run_command() {
    cat <<EOF
deck-proxy caps applied. Run the suite at 1280x800 with taskset:

  gamescope -W 1280 -H 800 -r 60 -- \\
    taskset -c $CPU_LIST ./build/smzuggler-rt --game "<game folder>" \\
    --perf-suite=deck-proxy --frame-stats="\$HOME/perf/"

Afterwards restore every cap:

  sudo $0 --restore

profile: deck-proxy, gate: proxy. This is not a pass.
EOF
}

# Undo the caps and remove the state tree; safe when nothing was applied.
restore_all() {
    if [ ! -d "$STATE_DIR" ]; then
        printf '%s\n' 'deck-proxy: nothing to restore'
        return 0
    fi
    need_root
    restore_values
    rm -rf "$STATE_DIR"
    printf '%s\n' 'deck-proxy: restored the previous caps'
}

# Apply every cap after checking root and preparing the state tree.
apply_all() {
    need_root
    prepare_state
    cap_cpu
    cap_gpu
    cap_power
    print_run_command
}

# Home directory of a login name, or nothing when the account is unknown.
home_of() {
    if command -v getent >/dev/null 2>&1; then
        getent passwd "$1" | cut -d: -f6
    else
        awk -F: -v user="$1" '$1 == user { print $6 }' /etc/passwd
    fi
}

# Start COMMAND under the CPU list as the sudo caller, so the game writes its
# state to that user's HOME. A root invocation has no caller identity and
# deliberately keeps root (documented above); an unknown caller fails loudly.
run_capped() {
    target=${SUDO_USER:-}
    if [ -z "$target" ] || [ "$target" = root ]; then
        exec taskset -c "$CPU_LIST" "$@"
    fi
    target_home=$(home_of "$target")
    if [ -z "$target_home" ]; then
        printf 'deck-proxy: cannot find home for %s\n' "$target" >&2
        exit 1
    fi
    exec runuser -u "$target" -- env HOME="$target_home" \
        taskset -c "$CPU_LIST" "$@"
}

# Entry point: help, restore, capped run, or apply-and-print.
main() {
    case ${1:-} in
        --help|-h)
            usage
            ;;
        --restore)
            restore_all
            ;;
        --run)
            shift
            [ "${1:-}" = "--" ] && shift
            [ "$#" -gt 0 ] || { usage >&2; exit 2; }
            apply_all
            run_capped "$@"
            ;;
        '')
            apply_all
            ;;
        *)
            usage >&2
            exit 2
            ;;
    esac
}

main "$@"
