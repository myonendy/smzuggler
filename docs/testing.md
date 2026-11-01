<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors

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
