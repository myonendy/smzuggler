+++
id = "M0-T009"
title = "--dump-surface: list the JS surface the shims install"
milestone = 0
deps = ["M0-T001"]
agent = "any"
owns = ["src/smzuggler/dump_surface.c", "src/smzuggler/dump_surface.h", "src/main.c", "src/engine/script_loader.c", "src/engine/script_loader.h", "CMakeLists.txt", "tests/smz/test_dump_surface.c", "tests/smz/dump_surface.cmake"]
docs = ["docs/shims.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `smzuggler-rt --dump-surface[=FILE]`: start the JS engine and run the shim load order of script_loader.c without a game, window or audio device, then print one sorted line per global, per own property of installed constructors and prototypes, and per `nw`/`process`/`navigator`/`document` member that the shims add (diff against the bare engine's globals), as `name<TAB>kind<TAB>shim file`. Exit without opening a window. Start `docs/shims.md` with the load order and this command.

## Acceptance criteria
- [ ] New test `tests/smz/test_dump_surface.c` (via `tests/smz/dump_surface.cmake`) runs the dump in-process and asserts it contains known members (`document.createElement`, `PIXI.Sprite`, `nw.Window.get`, `process.env`, `navigator.userAgent`), is sorted, and is identical across two runs.
- [ ] `smzuggler-rt --dump-surface` runs headless (no `SDL_Init(VIDEO)`) with `SDL_VIDEODRIVER` unset and no display, and does not read or write outside the build dir.
- [ ] `docs/shims.md` describes the load order with file and line references.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.

## Notes
Spec §15.4. Put the walker in `src/smzuggler/dump_surface.c`; main.c only parses the flag.
