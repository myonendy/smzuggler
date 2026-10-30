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
