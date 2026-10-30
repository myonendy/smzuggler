+++
id = "U13"
title = "U13: Canvas2D text fix upstream, if the M3 root cause is in Outsider's code"
milestone = 3
deps = ["M3-T001"]
agent = "any"
owns = ["src/rendering/canvas2d.c", "src/rendering/font_manager.c", "src/shims/**", "tests/**"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Draft. Once M3 has root-caused the missing HUD text (§5.5) and the cause is in Outsider's code (canvas2d.c, font_manager.c or a shim), port the minimal fix in upstream style with a golden test.

## Acceptance criteria
- [ ] Root cause documented in docs/canvas2d-text.md with file:line.
- [ ] Golden test reproduces the failure on upstream and passes with the fix.
- [ ] Same upstream-card rules as U1–U14.

## Notes
Spec §5.5, §8.1. Stays a draft until M3 card for the root cause lands.
