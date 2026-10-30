+++
id = "M4-T001"
title = "Opus source and WebM demux (nestegg) for SoLoud"
milestone = 4
deps = []
agent = "any"
owns = ["src/audio/**", "third_party/**"]
docs = ["docs/audio.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
`police radio.ogg` (WebM/Opus) and movie audio play through SoLoud.

## Acceptance criteria
- [ ] Decode tests on generated Opus/WebM samples

## Notes
First-pass draft from the §9 row; to be split and detailed when M4 is planned. Not scheduled (tasks/draft/).

Spec §4.3.
