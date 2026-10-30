+++
id = "M2-T003"
title = "WebGL2 shaders, getExtension and getParameter like Chrome 117"
milestone = 2
deps = ["M2-T001"]
agent = "any"
owns = ["src/webgl/**"]
docs = ["docs/webgl.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
GLSL ES 1.00/3.00 to the driver (rewrite only for desktop GL 3.3 core); `getExtension`/`getParameter` answer as Chrome 117 for what PIXI 5.3.12 probes.

## Acceptance criteria
- [ ] Extension list equals the recorded Chrome 117 list
- [ ] PIXI's shader compile path succeeds
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
First-pass draft from the §9 row; to be split and detailed when M2 is planned. Not scheduled (tasks/draft/).

Spec §4.2. Q5 (ANGLE scope) must be ruled first.
