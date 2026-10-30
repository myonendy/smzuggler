#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# forge `privacy` gate (spec §16). forge commits with --no-verify, so the
# pre-commit hook does not run on agent commits; this gate runs the same scan
# over the whole tree and over every commit since FORGE_BASE (identity,
# message and added lines). FORGE_BASE is set by forge; without it the range
# is everything after the upstream mirror.
set -euo pipefail
root=$(git rev-parse --show-toplevel)
base=${FORGE_BASE:-upstream-main}
exec python3 "$root/tools/privacy/scan.py" --repo "$root" --tree HEAD --range "$base..HEAD"
