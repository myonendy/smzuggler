+++
id = "M2-T004"
title = "ANGLE build on Windows/macOS and as the Linux CI oracle (-DMZ_GLES=angle)"
milestone = 2
deps = []
agent = "or-deepseek"
owns = ["cmake/**", "third_party/**", "CMakeLists.txt", "third_party/CMakeLists.txt", "tests/smz/test_angle_switch.c", "tests/smz/angle_switch.cmake"]
docs = ["docs/webgl.md", "docs/build.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Add the CMake build switch `-DMZ_GLES=angle` (§4.2) so ANGLE can be the GLES provider. On Windows and macOS, ANGLE is the intended production backend (D3D11 / Metal). On Linux, the default stays system Mesa GL/GLES (what the §14 laptop gate measures); the same switch builds the Linux CI reference oracle. Pin and vendor only what the build needs under `third_party/` (or a documented fetch that respects the offline-build rule in AGENTS.md).

## Acceptance criteria
- [ ] `-DMZ_GLES=angle` configures and links on Linux; `-DMZ_GLES=system` (or the equivalent default) remains the Linux default and does not pull ANGLE.
- [ ] Docs in `docs/build.md` and `docs/webgl.md` describe the switch, the Windows/macOS ANGLE path, and the Linux CI oracle use. They do **not** claim Q5 is closed.
- [ ] New ctest `angle_switch` (or a CMake/ctest smoke): when built with the angle switch, the binary reports the ANGLE-backed GL string (or a host probe the card documents); when built with the system default, it does not require ANGLE libs at runtime.
- [ ] CI wiring or a documented CI job fragment builds the ANGLE oracle on Linux. A full game boot in CI is required only when the runner has a GL device; otherwise the card delivers compile/link + the smoke test and records `LEFT OUT: game boot needs GPU runner` instead of faking a boot.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §4.2, §11 Q5. **Q5 is still open:** "ANGLE everywhere" versus "system GL on Linux/Deck + ANGLE on Windows/macOS and as the CI oracle". This card implements only what §4.2 already states (the switch, Windows/macOS ANGLE, Linux CI oracle). It must not invent an owner decision that closes Q5. If a step needs an owner Q5 ruling beyond that scope, stop and report `BLOCKED: needs owner Q5 ruling` rather than picking a side.

Owner 2026-11-04/05 agent workflow: run `forge check` once when finished and once after each fix; mid-work run just the one relevant test; read files by line range instead of whole files unless under 100 lines.
