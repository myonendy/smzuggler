#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# forge `commit_subjects` gate: every new non-merge commit since FORGE_BASE
# must have a subject that says what it changes (CONTRIBUTING.md, "Commit
# messages"). forge commits with --no-verify, so the commit-msg hook does not
# see them; this gate does. AgentForge's own "<ID>: attempt N by <vendor>"
# subjects are counted, not failed: forge has no option to change them
# (docs/FORGE.md, "Commit subjects"). Without FORGE_BASE the range is
# everything after the upstream mirror.
set -euo pipefail
root=$(git rev-parse --show-toplevel)
base=${FORGE_BASE:-upstream-main}
exec python3 "$root/tools/commitmsg/check_subject.py" --repo "$root" --range "$base..HEAD"
