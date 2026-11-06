+++
id = "M2-T002"
title = "WebGL2 textures, FBOs, renderbuffers with stencil"
milestone = 2
deps = ["M2-T001"]
agent = "or-deepseek"
owns = ["src/webgl/**", "tests/smz/test_webgl_textures.c", "tests/smz/webgl_textures.cmake", "tests/smz/test_webgl_fbo.c", "tests/smz/webgl_fbo.cmake"]
docs = ["docs/webgl.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Complete the WebGL2 surface for textures, framebuffers and renderbuffers (§4.2): `texImage2D` / `texSubImage2D` from Canvas2D, images and typed arrays (honour `UNPACK_PREMULTIPLY_ALPHA_WEBGL` and `UNPACK_FLIP_Y_WEBGL`); FBOs; renderbuffers including stencil; `readPixels`. Enforce the §15.8 GL object lifetime rules in code and in `docs/webgl.md`.

## Acceptance criteria
- [ ] New ctest `webgl_textures`: upload from a typed array and from a Canvas2D source; round-trip sample or `readPixels` matches; unpack flip/premultiply flags change the result the way WebGL2 specifies.
- [ ] New ctest `webgl_fbo`: attach colour and stencil renderbuffers (or a depth-stencil renderbuffer), draw, `readPixels` from the FBO colour attachment, then restore the default framebuffer.
- [ ] Stencil enable/test path is covered (a draw that fails the stencil test leaves the colour buffer unchanged).
- [ ] Every GL object wrapper owns exactly one GL name; `delete*()` marks the wrapper dead; GC finalizers only queue names for drain on the main thread at end of frame (§15.8). Documented in `docs/webgl.md`.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.2, §15.8. Depends on M2-T001. Do not invent texture formats or extension-only paths here; extensions are M2-T003. If no GL device is available, stop and report `BLOCKED: needs dev-laptop GUI run` instead of faking results.

Owner 2026-11-04/05 agent workflow: run `forge check` once when finished and once after each fix; mid-work run just the one relevant test (`ctest -R webgl_textures` / `webgl_fbo`); read files by line range instead of whole files unless under 100 lines.
