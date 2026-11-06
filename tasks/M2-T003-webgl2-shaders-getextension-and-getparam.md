+++
id = "M2-T003"
title = "WebGL2 shaders, getExtension and getParameter like Chrome 117"
milestone = 2
deps = ["M2-T001"]
agent = "or-deepseek"
owns = ["src/webgl/**", "tests/smz/test_webgl_shaders.c", "tests/smz/webgl_shaders.cmake", "tests/smz/test_webgl_extensions.c", "tests/smz/webgl_extensions.cmake"]
docs = ["docs/webgl.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Compile and link GLSL ES 1.00 / 3.00 shaders through the driver (rewrite only when the backend is desktop GL 3.3 core, per §4.2). Implement `getExtension` and `getParameter` so they answer as Chrome 117 for the extensions and limits PIXI 5.3.12 probes. Shader compile errors log source line numbers (§15.8).

## Acceptance criteria
- [ ] New ctest `webgl_shaders`: a minimal GLSL ES 3.00 vertex+fragment program compiles and links; a deliberate syntax error returns a log that includes a source line; `useProgram` then a draw succeeds without a GL error.
- [ ] New ctest `webgl_extensions`: `getExtension` returns only backed extensions; the set of names PIXI 5.3.12 probes matches the recorded Chrome 117 list kept under `docs/` or `tests/fixtures/` (add the fixture in owns if new); unbacked names return `null`.
- [ ] `getParameter` reports limits honestly for the values PIXI reads (at least `MAX_TEXTURE_SIZE`, `MAX_VERTEX_ATTRIBS`, `MAX_TEXTURE_IMAGE_UNITS`, and the other parameters the Chrome 117 capture lists for those probes).
- [ ] `docs/webgl.md` documents the extension policy and the desktop-GL rewrite rule.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.2. Q5 (ANGLE scope) is still open in SPEC §11: this card targets the system GLES/GL path and must not invent a closed Q5. ANGLE-vs-system differences are M2-T004. If no GL device is available, stop and report `BLOCKED: needs dev-laptop GUI run` instead of faking results. Shared `src/webgl/**` with M2-T002: forge serialises on overlapping owns.

Owner 2026-11-04/05 agent workflow: run `forge check` once when finished and once after each fix; mid-work run just the one relevant test (`ctest -R webgl_shaders` / `webgl_extensions`); read files by line range instead of whole files unless under 100 lines.
