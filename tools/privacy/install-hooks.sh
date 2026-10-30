#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Installs tools/privacy/pre-commit as this clone's pre-commit and commit-msg
# hooks (spec §16). Uses the common git dir, so every worktree shares them.
# Refuses to overwrite a different existing hook. Checks that the deny-list
# exists outside the repo, because the scan refuses to run without it.
set -euo pipefail
root=$(git rev-parse --show-toplevel)
hooks=$(git rev-parse --git-common-dir)/hooks
mkdir -p "$hooks"
src="$root/tools/privacy/pre-commit"
for name in pre-commit commit-msg; do
  dst="$hooks/$name"
  if [ -e "$dst" ] && [ "$(readlink -f "$dst")" != "$(readlink -f "$src")" ]; then
    echo "install-hooks: $dst exists and is not ours; move it away first" >&2
    exit 1
  fi
  ln -sf "$src" "$dst"
  echo "install-hooks: $name -> tools/privacy/pre-commit"
done
deny="${SMZUGGLER_PRIVACY_DENYLIST:-$HOME/.config/smzuggler-rt/privacy-denylist.txt}"
if [ ! -s "$deny" ]; then
  echo "install-hooks: warning: no deny-list at ~/.config/smzuggler-rt/privacy-denylist.txt; commits will be refused until it exists" >&2
fi
