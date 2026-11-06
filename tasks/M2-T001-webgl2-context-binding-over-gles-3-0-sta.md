+++
id = "M2-T001"
title = "WebGL2 context binding over GLES 3.0: state, buffers, VAOs"
milestone = 2
deps = []
agent = "or-deepseek"
owns = ["src/webgl/**", "CMakeLists.txt", "src/shims/dom_shim.js", "tests/smz/test_webgl_context.c", "tests/smz/webgl_context.cmake", "tests/smz/test_webgl_fast_api.cc", "tests/smz/webgl_fast_api.cmake"]
docs = ["docs/webgl.md", "docs/compat-surface.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Add `src/webgl/` and bind a native WebGL2 context to the SDL window's default framebuffer over the GLES 3.0 / GL 3.3-core API (§4.2). `HTMLCanvasElement.getContext('webgl2')` (and the `webgl` / `experimental-webgl` probe paths already stubbed in `src/shims/dom_shim.js`) returns that context instead of the hollow stub. Cover full context state, buffers, VAOs, and `drawArrays` / `drawElements`. Hot calls use V8 Fast API: `uniform*`, `bindBuffer`, `bufferSubData`, `drawElements`, `bindTexture`, `bindVertexArray`, `useProgram` (§4.2). Textures, FBOs, shaders, and extensions are later cards (M2-T002, M2-T003).

## Acceptance criteria
- [ ] New ctest `webgl_context`: create a WebGL2 context from `getContext('webgl2')`, set state (viewport, clear colour, enable/disable caps), create/bind/upload `ARRAY_BUFFER` / `ELEMENT_ARRAY_BUFFER`, create/bind a VAO, and issue a draw that clears or writes without a GL error.
- [ ] New ctest `webgl_fast_api`: a counter (or equivalent) shows each listed hot call is registered through V8 Fast API / `CFunction`, not only the slow callback path.
- [ ] The hollow WebGL stub object in `dom_shim.js` is gone for `webgl` / `webgl2` / `experimental-webgl`; those paths return the native context (or `null` only when no GL device is available, with the same honesty Chrome uses).
- [ ] `docs/webgl.md` is created and describes the context binding, default-framebuffer rule, and a pointer to §15.8 GL object lifetime (full lifetime rules land with M2-T002).
- [ ] Compat-surface rows exist for every WebGL entry point this card adds (`docs/compat-surface.md`).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment; GL object ownership follows §15.8 (wrappers own GL names; no `Local` stored past a `HandleScope`).
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.2, §15.8. Do not invent APIs beyond the WebGL2 surface named there. System GLES/GL on Linux is the default backend for this card; ANGLE is M2-T004. If the worker session has no GPU (`/dev/dri` missing), stop and report `BLOCKED: needs dev-laptop GUI run` instead of faking GL results; unit tests that need a context may skip only when no GL device exists, and the report must say so. Shared `src/webgl/**` with M2-T002 and M2-T003: forge serialises on overlapping owns.

Owner 2026-11-04/05 agent workflow: run `forge check` once when finished and once after each fix; mid-work run just the one relevant test (`ctest -R webgl_context` / `webgl_fast_api`); read files by line range instead of whole files unless under 100 lines.
