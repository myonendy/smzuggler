+++
id = "M2-T001"
title = "WebGL2 context binding over GLES 3.0: state, buffers, VAOs"
milestone = 2
deps = []
agent = "any"
owns = ["src/webgl/**"]
docs = ["docs/webgl.md", "docs/compat-surface.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Native `webgl2` context from `canvas.getContext('webgl2')` bound to the SDL window's default framebuffer; full context state, buffers, VAOs, draw calls; Fast API for the hot calls (§4.2).

## Acceptance criteria
- [ ] Conformance subset for state/buffers/VAOs passes
- [ ] Hot calls use V8 Fast API (checked by a counter test)

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/).

Spec §4.2.
