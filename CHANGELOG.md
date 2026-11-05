<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-10-31: add docs-touched gate release entry.
Modified 2026-10-31: record contribution guide and PR checklist.
Modified 2026-10-30: document runtime naming, CI coverage and format checks.
Modified 2026-10-31: record Markdown parser corrections.
Modified 2026-10-31: add the binary-size tool and M0 baseline.
Modified 2026-10-31: add the perf attribution tool entry.
Modified 2026-10-31: record the API-members-only surface dump.
Modified 2026-10-31: record the smaller-shim compatibility inventory.
Modified 2026-10-31: record the dom_shim.js compatibility inventory.
Modified 2026-10-31: add the share-bundle packager.
Modified 2026-10-31: record PIXI.Circle, Ellipse and Polygon (U8).
Modified 2026-10-31: record PIXI missing-member [PROF] warnings (U14).
Modified 2026-10-31: record the nw.Window EventEmitter (U3).
Modified 2026-10-31: record the real process env and platform (U5).
Modified 2026-10-31: record process.argv/cwd/exit, throwing require, plugin scan.
Modified 2026-10-31: record opt-in --data-dir log path (U9).
Modified 2026-11-01: record local evaluator scenarios.
Modified 2026-11-01: record evaluator game-process XDG isolation.
Modified 2026-11-01: record NW.js reference-recipe scratch HOME/XDG exports.
Modified 2026-11-01: record the .jsc cache-dir option (U10).
Modified 2026-11-01: default logs, cache and config to smzuggler-rt data dirs.
Modified 2026-11-01: record the Map 92 profile report tool and owner-run recipe.
Modified 2026-11-01: record the M0 exit observation checklist.
Modified 2026-11-01: record guarded save reads (U15).
Modified 2026-11-01: record the V8 build script and proposed pin.
Modified 2026-11-01: record jsb persistent callbacks.
Modified 2026-11-01: record the V8 script watchdog.
Modified 2026-11-01: record the game loop off QuickJS.
Modified 2026-11-01: record microtask checkpoints after every macrotask.
Modified 2026-11-04: record the renderer binding port.
Project documentation; identity decisions are in docs/DECISIONS.md.
-->
<!-- Records project changes for release review; keep entries free of private data. -->

# Changelog

All notable changes to smzuggler. Outsider's own history is in its git log.

## Unreleased
- Port the `__native_renderer` binding to the jsb seam with unchanged names, function arities, results and errors. (M1-T023)
- Port the native video binding to jsb with API parity tests. (M1-T020)
- `__native_canvas2d` is defined through the jsb seam by `bind_canvas2d_define`, with its JS API unchanged. (M1-T021)
- Port the `__native_effekseer` binding to the jsb seam with unchanged names, function arities, results and errors. (M1-T022)
- Port native file I/O bindings to jsb with API parity coverage. (M1-T016)
- Port native audio bindings to jsb while preserving function arities, coercion, results and errors. (M1-T019)
- Port native image loading, cache access and APNG decoding through jsb with API contract checks. (M1-T017)
- Port native font loading and measurement to jsb while preserving the JavaScript API. (M1-T018)
- V8 builds set --stack-size from the main thread's stack at startup (minus 25%, at least 256 KiB) and print the value; the /STACK link flag and the runtime thread are gone from v8 mode. (M1-T013)
- `__native_input` (bind_input.c) is defined through the jsb seam by `bind_input_define`, with its JS API unchanged. (M1-T015)
- bind_platform (`__native_platform`, `__native_process`) runs on the jsb seam through `bind_platform_define`, with its JS API unchanged and a both-modes ctest (M1-T014)
- jsb can adopt the engine's QuickJS context (`js_engine_get_jsb()` works in quickjs mode) and has value, coercion, array, error and function-length calls for binding ports. (M1-T007b)
- Store V8 code caches by source, version and flags in the host cache directory. (M1-T012)
- Preserve nextTick priority after callback exceptions while reporting failures. (M1-T010)
- Run a microtask checkpoint after every macrotask: after each input-phase dispatch, and
  in v8 mode after each timer and rAF callback through the new
  `__smz_microtaskCheckpoint()`; `process.nextTick` jobs now run before promise jobs
  (DEC-M1-T010, `tests/smz/test_microtasks.c`).
- Move `error_handler.c`'s window.onerror dispatch and crash auto-save off QuickJS onto
  `js_engine.h`, add the engine-neutral `error_handler_report_js_error(engine, message,
  stack, fatal)` and `error_handler_on_jsb_error`, a `jsb_error_fn` for jsb callback
  exceptions; `error_handler_report_js_exception` (QuickJS's pending exception) is a
  quickjs-mode inline in `error_handler.h`. Test `error_handler_jsb` (both modes).
  Card M1-T011.
- Take QuickJS out of `src/engine/game_loop.c`: every call into JS (timers, rAF, input,
  window close, control mode, exit probe) goes through `js_engine.h` to one JS helper,
  with phase order and timing unchanged, so the loop runs on both engines. Test
  `game_loop_js` (`docs/js-binding.md`, "Game loop"; DEC-M1-T009).
- Stop runaway scripts on V8: a watchdog thread (`src/engine/js_watchdog.cc`), present
  only while `js_engine_set_watchdog` has a limit, interrupts the script, reports its
  `file:line:col` and terminates it with `Isolate::TerminateExecution`; the engine then
  cancels the termination so the next eval runs. Test `js_watchdog`
  (`docs/js-binding.md`, "Watchdog"; DEC-M1-T008).
- Implement `js_engine.h` on V8 (`src/engine/js_engine_v8.cc`, library
  `smz_js_engine_v8` in v8 mode) with `ScriptOrigin` file names, console,
  error stacks and a microtask checkpoint; add `js_engine_get_jsb()` and the
  `js_engine_stack` and `js_engine_on_v8` tests (`docs/js-binding.md`,
  "js_engine on V8").
- Add persistent callbacks to the `jsb` seam in both backends:
  `jsb_callback_new`, `jsb_callback_call` (C-built arguments and result, own
  handle scope), `jsb_callback_free` (generation-checked handles, so freed
  and stale copies are refused) and `jsb_set_error_handler`, which receives
  every callback exception instead of C; with the `jsb_callbacks` test
  (`docs/js-binding.md`, "Persistent callbacks"; DEC-M1-T006).
- Add zero-copy buffers to the `jsb` seam in both backends: `jsb_arg_bytes`
  (bytes of an ArrayBuffer, TypedArray or DataView argument in place),
  `jsb_new_external_buffer` (native memory as an ArrayBuffer, freed once by a
  callback), `jsb_adjust_external_memory` and `jsb_set_flags`, with the
  `jsb_buffers` test (`docs/js-binding.md`, "Buffers"; DEC-M1-T005).
- Add the transitional QuickJS backend of the `jsb` seam,
  `src/jsb/jsb_quickjs.c`, built as `smz_jsb` in quickjs mode so `jsb_core`
  runs in both engine modes, plus `jsb_quickjs` tests for its handle chunks
  and per-runtime function tables; removed by M1-T027 (DEC-SETUP-008).
- Add the `jsb` binding seam: `src/jsb/jsb.h` (plain C11) and its V8 backend
  `src/jsb/jsb_v8.cc`, built as `smz_jsb` in v8 mode, with the `jsb_core` test
  and a C11 header check; API and handle rules in `docs/js-binding.md`
  (DEC-M1-T003).
- Add the `SMZ_JS_ENGINE` CMake option (`quickjs` default, `v8`) and
  `cmake/V8.cmake`, which imports the pinned prebuilt V8 from `SMZ_V8_DIR` as
  `smz::v8` and fails at configure time if it is missing or another version;
  v8 mode builds the `v8_smoke` test (`docs/build-v8.md`, DEC-M1-T002).
- Add `tools/v8/build_v8.sh`, which builds the proposed V8 pin 13.6.233.8 as a
  static monolith into `$SMZ_DEPS_DIR/v8/<version>/` with a `v8-config.cmake`
  manifest; `--print-args` shows the GN args (`docs/build-v8.md`, DEC-M1-T001).
- Add the §9 M0 exit checklist `docs/milestones/M0.md` (local gates pass;
  game-run items `pending owner run`, hosted CI `pending push`) and the owner
  run recipe `docs/milestones/m0-exit-recipe.md` (battery and `power-saver`
  checks, test copy, tree hash, smoke, evaluator, scratch save, bundle).
- With the save-path guard on, `existsSync`, `isDirSync`, `readdirSync`,
  `readFileSync` and `readFileBinary` also accept an absolute path outside the
  game root that the write guard would accept, so a save written under
  `$XDG_DATA_HOME/<dir_save_name>/` can be found and read back (U15).
- Add `tools/perf/map92_report.py`, which turns a Map 92 attribution run,
  GameLoopStats log, frame-time CSV and §14.4 machine-state snapshot into the
  M0 QuickJS profile report, plus the owner-run recipe
  `docs/perf/m0-map92-profile-recipe.md`.
- Add guarded local title/map/menu/save evaluator scenarios (`tools/evaluator/smzuggler.py`)
  with scratch storage, native timing exports and privacy-scanned local bundles.
- Load the evaluator prelude without StorageManager stubs so scenario save and reload hit the
  scratch save directory; check Map 92 directly and add per-step results to bundle READMEs.
- Start the evaluator game process with scratch HOME and XDG directories under the run so
  the save-location plugin cannot write the caller's save folder; the slot check searches
  that XDG tree as well as the staged `save/`.
- Export scratch HOME and XDG directories under the NW.js reference output before launching
  the inherited evaluator CLI, so the child inherits isolated save paths (U5); the slot
  file is under `$XDG_DATA_HOME/<save-dir>/` of that tree.
- Default logs, the `.jsc` cache and config to `$XDG_STATE_HOME` / `$XDG_CACHE_HOME` / `$XDG_CONFIG_HOME` `/smzuggler-rt` (HOME fallbacks when unset) so the game folder stays read-only; startup resolves all three, and `--data-dir=PATH` still overrides the log directory.
- Add `js_engine_set_cache_dir()` so the `.jsc` bytecode cache can live under the platform cache dir (`$XDG_CACHE_HOME/outsider/jsc`), keyed by the game-relative path, instead of beside each script (U10).
- Add opt-in `--data-dir` / `--data-dir=PATH` so `debug.log` and `console.log` go to the platform state directory (or PATH) instead of the game directory (U9).
- Log one `[PROF] missing: <Class>.<member>` warning per distinct missing renderer or plugin member a game reads, only when profiling is on (U14).
- Give `process.argv` Chrome/NW.js-shaped `[chrome, index.html]` values, keep `cwd()` and `mainModule.filename` on the game root, route `process.exit(code)` through the clean quit path, and throw `MODULE_NOT_FOUND` from unknown `require()`; add `tools/compat/scan_requires.py` to count plugin `require`/`process` uses.
- Make `process.env` a snapshot of the real environment and `process.platform`/`arch` the real OS and CPU, and add `versions`, `hrtime` and `nextTick`, so saves no longer go to `undefined/.local/share` (U5).
- Add PIXI.Circle, Ellipse and Polygon with PIXI 5.3.12 clone, contains, getBounds and type (U8).
- Add an opt-in save-path guard: with it on, file writes, mkdir, unlink and rename refuse relative paths and paths that resolve inside the game root, logging one error (U7).
- Report navigator.platform and userAgent for the real OS in Chrome's format, following process.platform/arch, instead of always Windows (U6).
- Deliver the `close` event on SDL quit and window close; a listener keeps the app running until `close(true)` or `nw.App.quit()` (U4).
- Answer canPlayType from the decoders the build has (MPEG-1, Ogg Vorbis, WAV); webm, MP4 and MP3 now answer '' (U11).
- Make `nw.Window.get()` a Node-style EventEmitter with the §5.2 window methods, so CleanExit-style `removeAllListeners("close")` works (U3).
- Add subtree tag lookup and simple tag/id/class selector queries with syntax errors for unsupported selectors (U2).
- Pin SoLoud and SDL_shadercross to the revisions in the offline dependency cache (U12).
- Include head, body and html in document tag lookups, including wildcard matches (U1).
- Add a read-only game smoke runner with SHA-256 tree checks and scratch XDG storage.
- Keep failed smoke diagnostics inside scratch, including when caller XDG paths name the game.
- Verify literal `name` compatibility rows through the checker CLI, including stale-row rejection.
- Recognize only the complete compatibility-table header, preserving the DOM `name` member.
- Add compatibility-surface rows for every dump member credited to `dom_shim.js`.
- Add compatibility-surface rows for every `pixi_shim.js` member; the full table
  now passes `compat_check.py` (M0-T010).
- Add `tools/share/make_bundle.py` and `smzuggler-rt --share-bundle` to pack a
  scrubbed zip of a run's logs, perf reports and frame times for sharing.
- Add `tools/perf/binsize.py`, which reports stripped `smzuggler-rt`, shared-library
  and shim sizes and a per-dependency archive breakdown, and record the M0 size baseline.
- Add the offline compatibility-surface checker and inventory for smaller shims.
- Add `tools/perf/attribute.py`, which splits `perf script` samples into the §6.1 buckets.
- Add §12 copyright lines and dated Modified notes to `.gitignore`,
  `tests/CMakeLists.txt` and `tests/integration/test_acceptance.c`; put the
  `docs/upstream-CONTRIBUTING.md` header items on separate lines.
- Skip verbatim licence texts (`LICENSE`, `LICENSES/*.txt`) by path in `headers.py`
  and list the remaining out-of-scope header violations for M0-T014b.
- Require the canonical General Arcade copyright and complete SPDX expression on
  added upstream-offer files; reject unrelated holders and substituted licences.
- Require licensing headers on prose and configuration and reject invalid UTF-8
  in checked files instead of silently treating source files as binary.
- Check renamed upstream files in `headers.py` against their source header so a
  moved Outsider file keeps upstream's licence lines.
- Add `tools/doclint/headers.py --base REV` to check spec section 12 licence headers
  and dated `Modified` notes; restore Outsider's header lines in `CMakeLists.txt`.
- Add smzuggler contribution rules, preserved upstream guidance and a tested PR checklist.
- Add the upstream offer tracker and isolated, offline branch extraction helper.
- Document upstream extraction ordering and verify atomic multi-commit replay.
- Add the architecture guide and checks for source references and directory coverage.
- Add headless `--dump-surface[=FILE]` shim inventory with sorted source attribution.
- Limit `--dump-surface` to API members: drop intrinsic function `length`,
  `name` and `prototype` rows and members of DOM elements under `document`.
- Ignore indented Markdown examples, check multiline link destinations, and
  decode heading entities while preserving literal code-span text in mdlinks.
- Add an offline Markdown link and anchor check (`tools/doclint/mdlinks.py`)
  and fix the broken architecture link in `docs/style.md`.
- Add a docs-touched gate with subsystem mappings and justified CI exemptions.
- Check documentation on compact C/C++ definitions independently in doclint.
- Add source documentation lint with changed-line checks and a legacy baseline.
- Recognize ES5 regex literals after logical operators and unary keywords.
- Added code style guidance, upstream-aligned clang-format and changed-file ES5 checks.
- Harden logger scrubbing for punctuation, root homes and Windows path casing;
  register POSIX logger fixtures only on supported platforms.
- Add explicit assertions for logger privacy path and buffer boundaries.
- Scrub home path prefixes from logger console, file and callback output.
- Added SHA-pinned Linux, Windows and macOS QuickJS CI and local testing guidance.
- Added configurable smzuggler identity and the smzuggler-rt executable name.
- Handle new branches, tags and unavailable old push tips in CI subject checks.
- Added CI privacy and commit-subject checks and contributor privacy guidance.
- Forked from Outsider f789f46 as smzuggler (QuickJS mode, unchanged behaviour).
- Added the v0.5.1 spec, the privacy scan and hooks (§16), and the AgentForge setup.
- Added commit subject rules (CONTRIBUTING.md), a commit-msg hook and the `commit_subjects` gate.
