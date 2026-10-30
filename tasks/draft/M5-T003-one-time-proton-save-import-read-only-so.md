+++
id = "M5-T003"
title = "One-time Proton save import (read-only source)"
milestone = 5
deps = ["M5-T001"]
agent = "any"
owns = ["src/smzuggler/**", "tools/**"]
docs = ["docs/saves.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Copy saves out of the Proton folder once; never write it.

## Acceptance criteria
- [ ] Import test on a fake Proton tree; source unchanged

## Notes
First-pass draft from the §9 row; to be split and detailed when M5 is planned. Not scheduled (tasks/draft/).

Spec §5.3.
