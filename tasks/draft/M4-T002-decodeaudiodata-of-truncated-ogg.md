+++
id = "M4-T002"
title = "decodeAudioData of truncated Ogg"
milestone = 4
deps = []
agent = "any"
owns = ["src/audio/**", "src/shims/webaudio_shim.js"]
docs = ["docs/audio.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Decode truncated Ogg as Chrome does (v0.2 §5.6).

## Acceptance criteria
- [ ] Truncated sample decodes to the expected length

## Notes
First-pass draft from the §9 row; to be split and detailed when M4 is planned. Not scheduled (tasks/draft/).

Spec §4.3.
