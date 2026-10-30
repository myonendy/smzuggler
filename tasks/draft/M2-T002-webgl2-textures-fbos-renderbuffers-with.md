+++
id = "M2-T002"
title = "WebGL2 textures, FBOs, renderbuffers with stencil"
milestone = 2
deps = ["M2-T001"]
agent = "any"
owns = ["src/webgl/**"]
docs = ["docs/webgl.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
`texImage2D` from Canvas2D/images/typed arrays, FBOs, renderbuffers with stencil, readPixels; GL object lifetime rules (§15.8).

## Acceptance criteria
- [ ] Texture upload and FBO round-trip tests
- [ ] Stencil test
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/).

Spec §4.2, §15.8.
