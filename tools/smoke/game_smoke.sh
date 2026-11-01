#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Run the read-only game smoke driver described in docs/testing.md.
# Keep all child output private: it may contain absolute game paths.
set -eu
exec python3 "$(dirname -- "$0")/game_smoke.py" "$@"
