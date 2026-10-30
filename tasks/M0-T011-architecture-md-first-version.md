+++
id = "M0-T011"
title = "ARCHITECTURE.md, first version"
milestone = 0
deps = ["M0-T008"]
agent = "any"
owns = ["ARCHITECTURE.md", "tests/tools/test_architecture_refs.py"]
docs = ["ARCHITECTURE.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Write `ARCHITECTURE.md` at the repo root as §15.2 asks: what smzuggler is (one paragraph), the process and thread model, the boot sequence from `main()` to the first frame with file references, the frame loop (game_loop.c phases), the JS ↔ native seam (today QuickJS bindings, `jsb` from M1), where shims sit in the load order, the data directories (§5.3, §13), and a map of `src/` and `tools/` with one line per directory. Mark clearly what is upstream Outsider and what is smzuggler.

## Acceptance criteria
- [ ] Every file or function the document names exists (checked by `tests/tools/test_architecture_refs.py`, which extracts backticked `path` and `path:function` references and verifies them).
- [ ] `tools/doclint/mdlinks.py` passes on it.
- [ ] Every directory under `src/` and `tools/` is listed.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.

## Notes
Spec §15.2. Keep it under ~400 lines; detail belongs in docs/.
