+++
id = "M0-T016"
title = "Read-only game smoke run"
milestone = 0
deps = ["M0-T001"]
agent = "any"
owns = ["tools/smoke/**", "tests/tools/test_game_smoke.py", "ARCHITECTURE.md"]
docs = ["docs/testing.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `tools/smoke/game_smoke.sh`: runs `smzuggler-rt` in control mode against `SMZUGGLER_GAME_DIR` for N frames with a scratch `XDG_*` tree, the game dir bind-mounted read-only with bwrap where available, and a before/after SHA-256 tree listing of the game dir. Fails if any file in the game dir was created, changed or removed, if any write lands outside the scratch tree, or if the runtime exits non-zero. Prints only relative paths.

## Acceptance criteria
- [ ] `ARCHITECTURE.md` has a directory-map row for `tools/smoke/` (required by `tests/tools/test_architecture_refs.py`); no other ARCHITECTURE.md change.
- [ ] `tests/tools/test_game_smoke.py` runs the script against a tiny fake game dir and a fake runtime (a shell stub that tries to write into the game dir) and asserts the script fails, and passes with a well-behaved stub.
- [ ] The script refuses Steam/Proton paths and an unset `SMZUGGLER_GAME_DIR` (exit 2, clear message).
- [ ] One real run against the test copy is attached to the card report (frame count, exit code, `game dir unchanged`).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §5.6, §9 M0 exit. Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results. A human adds this as a gate once it passes (DEC-SETUP-006).
