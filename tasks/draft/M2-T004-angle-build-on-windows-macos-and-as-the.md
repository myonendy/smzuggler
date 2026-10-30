+++
id = "M2-T004"
title = "ANGLE build on Windows/macOS and as the Linux CI oracle (-DMZ_GLES=angle)"
milestone = 2
deps = []
agent = "any"
owns = ["cmake/**", "third_party/**", "CMakeLists.txt"]
docs = ["docs/webgl.md", "docs/build.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Build switch for ANGLE everywhere; Linux CI reference run.

## Acceptance criteria
- [ ] ANGLE build boots the title on Linux in CI

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/).

Spec §4.2, Q5.
