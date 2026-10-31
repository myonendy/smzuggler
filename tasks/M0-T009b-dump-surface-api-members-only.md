+++
id = "M0-T009b"
title = "--dump-surface lists API members only, per spec §15.4"
milestone = 0
deps = ["M0-T009"]
agent = "any"
owns = ["src/smzuggler/dump_surface.c", "src/smzuggler/dump_surface.h", "tests/smz/test_dump_surface_members.c", "tests/smz/dump_surface_members.cmake"]
docs = ["docs/shims.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Make the dump list what spec §15.4 calls the surface ("every global and prototype member the shims install"), per DEC-M0-T009b. Today it also lists (a) the intrinsic own properties every function has (`length`, `name`, and `prototype` as a row of its own), about 2,300 of 6,124 lines, and (b) the members of object instances reached from `document` (`document.body.*`, `document.head.*`, `document.documentElement.*`), about 2,000 lines that repeat `HTMLElement.prototype` members. Drop both. `document.body`, `document.head` and `document.documentElement` stay as one row each. Members of `X.prototype` keep their `X.prototype.member` name. Everything else is unchanged, including `_`-prefixed members.

## Acceptance criteria
- [ ] New test `tests/smz/test_dump_surface_members.c` (via `tests/smz/dump_surface_members.cmake`) asserts that the dump has no line ending in `.length`, `.name` or `.prototype` for a function-valued parent; that it has no `document.body.`/`document.head.`/`document.documentElement.` sub-rows; and that it still has `document.body`, `document.createElement`, `PIXI.Sprite`, `PIXI.Sprite.prototype.destroy`, `nw.Window.get`, `process.env` and `navigator.userAgent`.
- [ ] The existing `tests/smz/test_dump_surface.c` passes unchanged.
- [ ] `docs/shims.md` says what the dump leaves out and why (DEC-M0-T009b).
- [ ] The report gives the new line count of `./build/smzuggler-rt --dump-surface` (expected ~2,000).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §15.4. Measured 2026-10-31 on the M0-T010 worktree build: 6,124 lines raw, ~1,990 after these two exclusions.
