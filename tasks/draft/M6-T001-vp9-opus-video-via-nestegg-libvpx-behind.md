+++
id = "M6-T001"
title = "VP9/Opus video via nestegg + libvpx behind video_player.h"
milestone = 6
deps = []
agent = "any"
owns = ["src/video/**", "src/shims/dom_shim.js", "third_party/**"]
docs = ["docs/video.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Replace pl_mpeg; no transcoding; canPlayType updated.

## Acceptance criteria
- [ ] Both movies play with A/V sync ±45 ms

## Notes
First-pass draft from the §9 row; to be split and detailed when M6 is planned. Not scheduled (tasks/draft/). Runs on dev-laptop on battery with `power-saver` (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%), against the test copy of the game in `SMZUGGLER_GAME_DIR`, never the Steam folder. If the agent cannot open a window or reach the game copy, it stops and reports `BLOCKED: needs dev-laptop GUI run` instead of faking results.

Spec §4.4.
