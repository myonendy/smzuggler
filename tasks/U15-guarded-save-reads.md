+++
id = "U15"
title = "U15: let save reads follow the write guard to absolute save paths"
milestone = 0
deps = ["U7", "U9", "M0-T021"]
agent = "any"
owns = ["src/bindings/bind_io.c", "src/io/file_io.c", "src/io/file_io.h", "tests/test_file_io.c", "tests/test_storage.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
With the U7 save-path guard on (`file_io_set_write_guard(true)`, which `src/main.c` enables), writes go to absolute paths outside the game root, but the read-side calls still go through `file_io_resolve_path()`, which rejects any absolute path outside the game root. So `existsSync` on the save folder fails, `DataManager.saveGame(1)` never creates the slot file under `$XDG_DATA_HOME/<dir_save_name>/`, and the game cannot read a save back (found by M0-T022's run). Make `existsSync`, `isDirSync`, `readdirSync`, `readFileSync` and `readFileBinary` accept an absolute path outside the game root when, and only when, the guard is on and the same path would pass the write guard. Everything else is unchanged: guard off behaves exactly as upstream does today, relative paths and paths inside the root resolve as before, and a path the write guard refuses is still refused for reads.

## Acceptance criteria
- [ ] New cases in `tests/test_file_io.c`, guard on, under a scratch directory outside the game root: `existsSync` is false for a missing file and true after the file is written; `isDirSync` and `readdirSync` see a directory made with `mkdirSync`; `readFileSync` and `readFileBinary` return the bytes written. A missing file logs no error.
- [ ] New cases with the guard on: an absolute path inside the root, `root/../root/x`, and a symlink outside the root that points inside it all resolve the way they do today (not through the new path). With the guard off, an absolute path outside the root is still rejected for reads.
- [ ] New QuickJS case in `tests/test_storage.c` with scratch `HOME`, `XDG_DATA_HOME` and the guard on: the save sequence the game's save code uses (exists check on the folder, `mkdirSync`, `writeFileSync`, exists check on the file, `readFileSync`) creates `file1.rmmzsave` under `$XDG_DATA_HOME/<name>/` of the scratch tree and reads it back; nothing is written in the game root.
- [ ] No file outside the owned files changes, apart from `docs/UPSTREAM.md` (new U15 row set to `ready to cut` on merge) and `CHANGELOG.md`.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
This is an upstream-candidate card (§8), a follow-up to U7 in the same files (DEC-U15-SAVE-READS). Rules: upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the owned files; add tests in upstream's existing test files. Nothing goes upstream automatically; the owner decides whether to cut `up/U15-guarded-save-reads` after talking to the upstream developer. No game run is needed: the tests use scratch directories only. Reads go only where writes may already go, so the guard's protection of the game root is unchanged.

Spec §5.3.
