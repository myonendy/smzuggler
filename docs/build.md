<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
How to build the runtime and how the GLES provider is selected.
Spec §4.2 and §11 Q5; the ANGLE pin follows the V8 13.6 (Chrome 136) pin.
Keep commands free of personal paths.
-->

# Building smzuggler

The runtime is configured with CMake 3.20 or newer and built offline from the
dependency cache described in [Working with AgentForge](FORGE.md#dependency-cache).
V8 is the only JavaScript engine; [Building V8](build-v8.md) covers its pinned
install and `SMZ_V8_DIR`.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DRMMZ_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Linux needs the EGL and GLES development files for the default provider
(Arch `libglvnd` plus `mesa`; Debian/Ubuntu `libegl1-mesa-dev` and
`libgles2-mesa-dev`; Fedora `mesa-libEGL-devel` and `mesa-libGLES-devel`).
Windows and macOS take the ANGLE path below.

## The `MZ_GLES` switch

`MZ_GLES` chooses the OpenGL ES provider that the WebGL2 stack links
(spec §4.2). Both choices export the standard EGL and GLES entry points; the
switch changes which libraries those symbols resolve to.

| `-DMZ_GLES=` | Provider | Where it is the intended path |
|---|---|---|
| `system` (default) | The platform EGL/GLES (Mesa on Linux) | Linux, Steam Deck and Android, the §14 targets |
| `angle` | An installed, pinned ANGLE | Windows (D3D11), macOS (Metal), and the Linux CI reference oracle |

`system` is the default on every platform so a missing ANGLE cannot break an
ordinary Linux build. Windows and macOS release builds select `angle`; their
system GLES does not exist, and the configure step says so when `system` finds
nothing there.

The switch does **not** settle spec §11 Q5. "ANGLE everywhere" versus "system
GL on Linux/Deck with ANGLE on Windows/macOS and as the CI oracle" is still the
owner's decision; this page documents only what §4.2 already states.

## ANGLE on Windows and macOS

ANGLE translates GLES 3.0 to D3D11 on Windows and Metal on macOS, which is why
it is the production provider there and why Chrome's precision and blending
behaviour carry over. A build selects it with the staged install:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DMZ_GLES=angle -DSMZ_ANGLE_DIR="$SMZ_DEPS_DIR/angle/136.0.7103.113"
```

`SMZ_ANGLE_DIR` is a prefix with `include/` and `lib/`:

| Path | Content |
|---|---|
| `include/EGL/`, `include/GLES2/`, `include/GLES3/` | ANGLE's Khronos headers, or the system ones when the install ships libraries only |
| `lib/libEGL.*` | The ANGLE EGL library |
| `lib/libGLESv2.*` | The ANGLE GLES library |

On Windows the import libraries are `libEGL.lib` and `libGLESv2.lib` beside
the DLLs. The configure step fails with the variable name when either library
is missing, and the provider libraries are searched only inside
`SMZ_ANGLE_DIR`, so a system Mesa cannot be mistaken for ANGLE.

## The ANGLE pin

The pin is **ANGLE commit `fa40b7c586fd2da9fd7e5c4d893ecb1334553b9e`** on
branch `chromium/7103`, the ANGLE revision shipped by Chrome 136.0.7103.113.
That is the Chromium release of the V8 13.6 pin recorded in
[Building V8](build-v8.md), so the oracle and the engine come from the same
browser generation. ANGLE is BSD-3-Clause (spec §12). Moving either pin is a
one-line change plus a new decision entry.

ANGLE is not fetched by CMake. Its authoritative build is GN/Ninja over a
`gclient` checkout, and the offline rule in `AGENTS.md` keeps source acquisition
out of configure. Stage it once, then configure against the prefix:

```sh
# One-time, with depot_tools on PATH and network access (a human or CI step).
mkdir -p "$SMZ_DEPS_DIR/angle/136.0.7103.113"
cd "$SMZ_DEPS_DIR/angle-src"
gclient config --name . https://chromium.googlesource.com/angle/angle
gclient sync
git checkout fa40b7c586fd2da9fd7e5c4d893ecb1334553b9e
gclient sync
gn gen out/Release --args='is_debug=false'
autoninja -C out/Release libEGL libGLESv2
cp -r include "$SMZ_DEPS_DIR/angle/136.0.7103.113/"
cp out/Release/libEGL.* out/Release/libGLESv2.* \
   "$SMZ_DEPS_DIR/angle/136.0.7103.113/lib/"
```

Every later configure is offline and reads only that prefix.

## Linux CI reference oracle

The oracle is a second Linux build of the same tree with `-DMZ_GLES=angle`.
It compiles, links and runs the `angle_switch` provider probe on a runner
without a GPU; a full game boot runs only on a runner that has a GL device.
This is a documented job fragment, not yet part of `.github/workflows/ci.yml`
(that file is outside this card's scope):

```yaml
  linux-angle-oracle:
    runs-on: ubuntu-22.04
    env:
      SMZ_DEPS_DIR: ${{ runner.temp }}/smz-deps
    steps:
      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1
        with:
          persist-credentials: false
      - name: Stage pinned ANGLE
        run: |
          git clone --depth 1 \
            https://chromium.googlesource.com/chromium/tools/depot_tools.git \
            "$SMZ_DEPS_DIR/depot_tools"
          export PATH="$SMZ_DEPS_DIR/depot_tools:$PATH"
          mkdir -p "$SMZ_DEPS_DIR/angle-src"
          cd "$SMZ_DEPS_DIR/angle-src"
          gclient config --name . https://chromium.googlesource.com/angle/angle
          gclient sync
          git checkout fa40b7c586fd2da9fd7e5c4d893ecb1334553b9e
          gclient sync
          gn gen out/Release --args='is_debug=false'
          autoninja -C out/Release libEGL libGLESv2
          prefix="$SMZ_DEPS_DIR/angle/136.0.7103.113"
          mkdir -p "$prefix/lib"
          cp -r include "$prefix/"
          cp out/Release/libEGL.* out/Release/libGLESv2.* "$prefix/lib/"
      - name: Configure the ANGLE oracle
        run: cmake -S . -B build-angle -G Ninja -DCMAKE_BUILD_TYPE=Release \
               -DRMMZ_BUILD_TESTS=ON -DMZ_GLES=angle \
               -DSMZ_ANGLE_DIR="$SMZ_DEPS_DIR/angle/136.0.7103.113"
      - name: Build
        run: cmake --build build-angle --parallel
      - name: Provider smoke test
        run: ctest --test-dir build-angle -R '^angle_switch$' --output-on-failure
```

That staging block is the same command sequence shown above, run by CI with
network access and a cache for `$SMZ_DEPS_DIR`. The depot_tools checkout floats
at its current default branch; ANGLE itself remains pinned to the commit above.
On a GPU runner the same job
adds the ordinary game-boot ctest; on a runner without one the smoke test is
the delivered evidence, and the game boot is recorded as left out rather than
faked.

The default Linux build is not linked against ANGLE: it links the system
EGL/GLES libraries, and its `angle_switch` test runs against them.
The probe exits nonzero for missing extensions or a provider mismatch even in
Release builds with `NDEBUG` defined.
