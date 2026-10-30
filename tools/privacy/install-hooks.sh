#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Installs this clone's pre-commit hook (tools/privacy/pre-commit, spec §16)
# and commit-msg hook (tools/commitmsg/commit-msg: §16 scan plus subject
# rules). Uses the common git dir, so every worktree shares them.
# Refuses to overwrite a different existing hook. Checks that the deny-list
# exists outside the repo, because the scan refuses to run without it.
set -euo pipefail
root=$(git rev-parse --show-toplevel)
hooks=$(git rev-parse --git-common-dir)/hooks
mkdir -p "$hooks"
# pre-commit: privacy scan of identity and staged diff.
# commit-msg: privacy scan of the message, then the commit subject rules.
for pair in pre-commit:tools/privacy/pre-commit commit-msg:tools/commitmsg/commit-msg; do
  name=${pair%%:*}; src="$root/${pair#*:}"
  dst="$hooks/$name"
  # An older install linked commit-msg to tools/privacy/pre-commit; replace that link.
  if [ "$name" = commit-msg ] && [ "$(readlink -f "$dst" 2>/dev/null)" = "$(readlink -f "$root/tools/privacy/pre-commit")" ]; then
    rm -f "$dst"
  fi
  if [ -e "$dst" ] && [ "$(readlink -f "$dst")" != "$(readlink -f "$src")" ]; then
    echo "install-hooks: $dst exists and is not ours; move it away first" >&2
    exit 1
  fi
  ln -sf "$src" "$dst"
  echo "install-hooks: $name -> ${pair#*:}"
done
deny="${SMZUGGLER_PRIVACY_DENYLIST:-$HOME/.config/smzuggler-rt/privacy-denylist.txt}"
if [ ! -s "$deny" ]; then
  echo "install-hooks: warning: no deny-list at ~/.config/smzuggler-rt/privacy-denylist.txt; commits will be refused until it exists" >&2
fi
