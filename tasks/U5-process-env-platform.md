+++
id = "U5"
title = "U5: real process.env, platform, versions, arch, hrtime and nextTick"
milestone = 0
deps = ["M0-T013", "M0-T005"]
agent = "any"
owns = ["src/shims/storage_shim.js", "src/bindings/bind_platform.c", "src/bindings/bind_platform.h", "tests/test_storage.c"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
`process.env` is a snapshot of the real environment taken at start (a plain object of strings); `process.platform` and `process.arch` are the real OS/CPU in Node's spelling (`linux`, `win32`, `darwin`; `x64`, `arm64`) from a native `bind_platform.c` call; `process.versions` has `node`/`nw`/`chromium` values matching the NW.js the game targets, plus the engine; `process.hrtime([prev])` and `hrtime.bigint` if BigInt exists; `process.nextTick(fn, ...args)` runs before the next macrotask. Today `process.env` is `{}`, which sends saves to `undefined/.local/share/...` (§2, §5.3).

## Acceptance criteria
- [ ] New cases in tests/test_storage.c set an env var in the test process and assert it appears in `process.env`; assert platform/arch against the compile target; hrtime monotonic and `[s, ns]` shaped; nextTick ordering relative to setTimeout(0) and Promise jobs as in Node.
- [ ] With `HOME` set, the save path the game's save plugin would compute is under `$XDG_DATA_HOME` or `$HOME/.local/share`, never `undefined/...` (tested with a stand-in snippet, not the game's plugin).
- [ ] `tools/format/es5_check.py --base upstream-main` (from M0-T005) passes on the changed shim files.
- [ ] No file outside `src/shims/storage_shim.js`, `src/bindings/bind_platform.c`, `src/bindings/bind_platform.h` and the listed tests changes, apart from docs/UPSTREAM.md, docs/compat-surface.md and CHANGELOG.md.
- [ ] Changes to existing test files only add tests; no existing assertion is changed or removed.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
This is an upstream PR card (§8). Rules: ES5 only in shims (no let/const, arrow functions, classes, template literals, destructuring, default parameters or `for…of`); upstream's file header and style, **no** smzuggler copyright line on upstream files (DEC-SETUP-005, SI-5); change only the files in its §8.1 row plus their tests; add tests in upstream's existing test files so the patch applies upstream as is. The tests run in QuickJS mode, which `main` still is during M0. When merged, set the row in docs/UPSTREAM.md to `ready to cut`; the owner cuts `up/U5-process-env-platform` with `tools/upstream/cut_up_branch.sh` and opens the PR.

Spec §5.3 (storage_shim.js 344–361).
