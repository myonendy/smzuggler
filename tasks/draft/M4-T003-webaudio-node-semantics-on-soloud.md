+++
id = "M4-T003"
title = "WebAudio node semantics on SoLoud"
milestone = 4
deps = ["M4-T001"]
agent = "any"
owns = ["src/audio/**", "src/shims/webaudio_shim.js"]
docs = ["docs/audio.md", "docs/compat-surface.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
GainNode ramps, Panner/StereoPanner, playbackRate, loopStart/loopEnd mapped onto SoLoud.

## Acceptance criteria
- [ ] Audio conformance suite (v0.2 §10.2) passes

## Notes
First-pass draft from the §9 row; to be split and detailed when M4 is planned. Not scheduled (tasks/draft/).

Spec §4.3; custom AudioSource node only if fades differ audibly.
