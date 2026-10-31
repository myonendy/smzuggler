#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Extract assigned upstream patches without changing the caller's checkout.
# See docs/UPSTREAM.md; all work happens synchronously in a disposable worktree.
set -euo pipefail
exec python3 "$(dirname -- "${BASH_SOURCE[0]}")/cut_up_branch.py" "$@"
