<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-11-01: document isolated evaluator scenarios, the storage-preserving
prelude, step summaries and local NW.js references.
Modified 2026-11-01: document scratch HOME/XDG isolation for the evaluator game process.

Local and CI validation of the QuickJS baseline, part of build infrastructure.
Keep the game-free suite independent of any installed game; do not bypass skips.
-->

# Testing

Run the local equivalent of CI from the repository root:

```sh
python3 -m pip install PyYAML==6.0.2
tools/forge/build.sh
tools/forge/test.sh
```

The build gate configures Release with `RMMZ_BUILD_TESTS=ON` and builds it.
It requires the prepopulated offline dependency cache described in
[Working with AgentForge](FORGE.md#dependency-cache); it never downloads sources.
The test gate runs CTest, followed by unittest discovery in `tools/privacy`,
`tools/commitmsg` and `tests/tools`. PyYAML parses the CI workflow in the tool tests.

[CI](../.github/workflows/ci.yml) runs on pushes and pull requests, with separate
Linux, Windows and macOS jobs. Each configures CMake, builds, runs CTest with
failure output, then runs the same three Python test directories. Windows uses
Visual Studio 2022 and the Release configuration; Linux and macOS use Ninja.
QuickJS is the current CMake baseline, so no engine selection flag is needed.

Dependencies and the Linux Xvfb/Openbox display setup follow upstream's
[release workflow](../.github/workflows/release.yml). That workflow has no cache
step to reuse. Hosted CI fetches the pinned CMake dependencies; local forge
builds instead use the offline cache. All CI actions are pinned to full commit
SHAs. No game files or game path variables are supplied. Game-dependent tests
retain upstream's existing `RMMZ_TEST_GAME_DIR` skip mechanism. The local test
gate clears game path variables before running CTest.

Linux window tests need a display and window manager. In a headless local shell,
start Xvfb and Openbox as in CI before running the test gate. GPU tests use the
available backend or report upstream's existing skips when no device is usable.
Actual hosted runs require the owner to push the workflow (SI-6 in
[specification issues](SPEC_ISSUES.md)); local workflow tests verify its wiring.

## Read-only game smoke

Run `tools/smoke/game_smoke.sh 120` with `SMZUGGLER_GAME_DIR` naming a local
**test copy**, and `smzuggler-rt` on PATH. `SMZUGGLER_RUNTIME` may select another
executable. Steam/Proton locations, missing game configuration and invalid frame
counts return 2 with a message naming the failed check. Steam directory names
(`steam`, `.steam`, `steamapps`, `SteamLibrary`, `compatdata`, `pfx`) match
exactly; any component containing `proton` is refused. The default is 120
frames. Symlinks and special files in the game tree are refused to avoid
following data outside the copy.

The driver launches control mode, sends an initial evaluation command and keeps
stdin open until the frame-limited runtime exits. The wall timeout is 60 s of
startup allowance plus four times the frames' 60 fps duration; expiry fails the
run with exit code 124. Because `XDG_RUNTIME_DIR` moves to scratch, a relative
`WAYLAND_DISPLAY` is passed as the absolute socket path in the host runtime dir
(and `PULSE_SERVER` points at the host PulseAudio/PipeWire socket when present),
so Wayland sessions work without an XWayland fallback. It creates private HOME, temporary and XDG directories, starts the
runtime in scratch with this checkout's shims, and uses bubblewrap to bind the host and game read-only with
scratch as the only writable filesystem mount. Network and PID namespaces and
dropped capabilities constrain the child. Bubblewrap is required: missing or
unusable containment fails closed rather than executing an unprotected runtime.
Display sockets and device access still depend on the local GUI environment;
this is a filesystem smoke harness, not a hostile-code security boundary.

Before/after SHA-256 manifests cover every regular file and include directory
entries. The report prints both listings with JSON-escaped relative paths,
requested frame count, runtime exit code and `game dir unchanged` or
`game dir changed`. Any tree difference or nonzero child status fails the run.
Runtime output is private scratch data and is deleted with the manifests on
exit, so absolute paths from runtime diagnostics cannot leak into the report.
Diagnostics stay inside scratch on failures too; the driver never persists
logs under the caller's XDG directories, even when they point into the game.
A denied write that the child ignores cannot land outside scratch; a runtime
that propagates a denied write fails through its exit status.

The game-free tests use a tiny synthetic tree, shell runtime stubs and a
bubblewrap command double to verify mount arguments, control input, tree
mutation detection and exit handling. They do not certify kernel confinement.
Run them with `python3 -m unittest discover -s tests/tools -p test_game_smoke.py`.

The real M0 report must record frame count, exit code and `game dir unchanged`
on dev-laptop after at least two minutes unplugged, with battery at least 50%
and `powerprofilesctl get` reporting `power-saver`. If the game copy or GUI is
unavailable, report `BLOCKED: needs dev-laptop GUI run`; stub runs are not real
game evidence. A human wires the gate after that run (DEC-SETUP-006).

## Local title, map, menu and save scenarios

M0-T015 adds `tools/evaluator/scenarios/smz-basics.json`, loaded by the
upstream evaluator's scenario schema. Run its guarded host entry point:

```sh
python3 -m tools.evaluator.smzuggler --dry-run
python3 -m tools.evaluator.smzuggler --battery-gate-confirmed
```

Set `SMZUGGLER_GAME_DIR` to a separate local test copy, never a store install.
The dry run prints resolved input, runtime and output paths without writing.
Both lexical and resolved paths containing `steamapps` or `compatdata` are
refused, including symlinks through those components. State and game roots
must not overlap. `--runtime` selects a locally built host if needed.
Before the real command, the owner verifies on dev-laptop that the battery is
at least 50%, AC has been disconnected for at least two minutes, and
`powerprofilesctl get` reports `power-saver`. The confirmation flag records
that manual check; it does not measure or replace it.

Each session creates a fresh directory under
`$XDG_STATE_HOME/smzuggler-rt/evaluator/` (default
`~/.local/state/smzuggler-rt/evaluator/`). The host runs against a disposable
copy of the inputs under this run's output directory. Copies dereference
symlinks and exclude top-level owner save directories, save files and runtime
logs. Reserve enough space for a complete test copy per session. The game
process is started with scratch `HOME`, `XDG_DATA_HOME`, `XDG_CONFIG_HOME`,
`XDG_STATE_HOME` and `XDG_CACHE_HOME` directories created under that run
(never the caller's), so the save-location plugin, which reads `process.env`
(U5), resolves inside the scratch tree. Display and `PATH` stay inherited so
the window can open. Nothing is written into the original game directory.
Unlike the read-only smoke above, this route needs a writable save directory,
so it uses a scratch copy instead of a read-only bubblewrap mount.

The route waits for title, starts a new game, checks `$gameMap.mapId() === 92`
directly (not the configured start map), captures 60/300/600 additional
logical frames after map readiness (the last includes the HUD), opens the main
menu, saves slot 1, checks that the slot exists, reloads it (requiring a
replacement game-system object) and requests `nw.Window.get().close()`. The
process must exit with status zero; forced cleanup does not count as a clean
close. After the steps, the entry point also checks that a slot-1 file exists
in the scratch copy's `save/` directory or under the child's
`$XDG_DATA_HOME/<save-dir>/` (both `file1.rmmzsave` and `forage_*file1`
names). The save-dir name is taken from the tree the plugin wrote; it is not
hard-coded. Readiness can advance frames, so these capture offsets are
relative to readiness, not engine boot.

The upstream `prelude.js` stubs `StorageManager.saveObject`, `loadObject` and
`exists` so its own scenarios never touch player data. That would make save
and reload no-ops, so the entry point loads the prelude without that section
(`--print-prelude` shows exactly what is loaded). The cut keeps
`var storagePatched = false` so prelude status reporting stays valid. This is
safe only because the host sees a scratch copy without owner saves and because
its HOME and XDG bases are under the run directory.

`summary.json` records logical frame counts and status per step without game
text or screenshots. `runtime.log`, native phase averages in `perf.json`,
`frame-times.csv`, `steps.csv` and captures stay in the run directory. Native
`--perf 1` reports one-frame timings rounded to 0.1 ms; control-mode samples
are diagnostic evidence, not the later device performance acceptance gate.
The share packager runs locally with explicit screenshot inclusion, scans
staged output and writes `bundle.zip` only on success. The archive names
the JSON reports `perf-report-evaluator.json` and `perf-report-steps.json`
(the step list wrapped in an object) to match the packager's report
allowlist. Scratch inputs and saves are excluded. Its `README.txt` lists the
scrubbed contents, and the entry point then appends a pass/fail line per step
(step indices and fixed status words only). No upload or posting occurs;
screenshots and reference captures must never be committed.
`tests/tools/test_scenario_bundle.py` packs a synthetic run in this layout
with the real packager.

For NW.js reference captures, the owner uses another disposable local test
copy with no personal saves, on the same battery configuration. Make the
assets read-only and keep only that copy's scratch save directory writable.
Write the storage-preserving prelude with
`python3 -m tools.evaluator.smzuggler --print-prelude > <ref-dir>/prelude.js`,
then run the inherited evaluator (`python3 -m tools.evaluator.cli`) against
the copy with `--scenario tools/evaluator/scenarios/smz-basics.json`,
`--prelude <ref-dir>/prelude.js`, `--skip-outsider` and `--out <ref-dir>`,
where `<ref-dir>` is a separate directory under the evaluator state root. Its
NW.js profile is created under that output directory. Never run this recipe on
the original store directory. Inspect and compare the captures locally using
the inherited comparison/report workflow; do not add any reference image to git.

If the game copy or GUI is unavailable, the entry point reports
**BLOCKED: needs dev-laptop GUI run** and exits non-zero. The real run and its
bundle are the owner's checkpoint after merge (docs/FORGE.md,
DEC-M0-T015-OWNER-RUN); the owner keeps the real `summary.json`, bundle path
and archive README summary locally and attaches them to that checkpoint.
