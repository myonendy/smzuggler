<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
M0 binary-size baseline (spec section 9); measured by tools/perf/binsize.py.
See docs/perf.md. Repository documentation, not a game-run report.
-->

# Binary size: M0 baseline

This is the size of a smzuggler-rt build in QuickJS mode, before the V8 swap.
M1 (V8, ICU) and M2 (ANGLE) are measured against it, and open questions Q5
(ANGLE everywhere or not) and Q8 (ICU data size) are decided against it.
[docs/perf.md](../perf.md) explains how the tool measures and how to refresh
this page.

## Build used

The release build is what `tools/forge/build.sh` configures. The cache entries
`binsize.py` reads are:

| Setting | Value |
|---|---|
| `CMAKE_BUILD_TYPE` | `Release` (CMake default `-O3 -DNDEBUG`, no `-g`) |
| `CMAKE_C_FLAGS` | `-Wno-error=discarded-qualifiers` (DEC-SETUP-003) |
| `CMAKE_CXX_FLAGS` | none |
| `RMMZ_RENDER_BACKEND` | `gpu` (default), so `RMMZ_RUNTIME_SHADERS=ON` links glslang, SPIRV-Cross and SDL_shadercross |
| `RMMZ_USE_EFFEKSEER` | off (default), so no Effekseer archive is built |
| SDL3 | static (`SDL_STATIC=ON`; shared only on Android) |
| Dependencies | the pinned sources in the local dependency cache ([docs/FORGE.md](../FORGE.md)) |

Command:

```sh
tools/forge/build.sh
python3 tools/perf/binsize.py build
```

## Shipped size

Measured on 2026-10-31 on Linux x86-64 (dev-laptop) from the build above.

| Shipped part | Bytes | KiB |
|---|---:|---:|
| `smzuggler-rt` stripped | 13946390 | 13619.5 |
| shims (13 files, source tree) | 432577 | 422.4 |
| **total** | **14378967** | **14042.0** |

There is no shared-library row because this build links no shared library of
its own: SDL3 and every other dependency are static. System libraries
(libc, libm, libGL, pthread) are outside the build tree and are not counted.
The shim total is the 13 `src/shims/*.js` files that the `runtime` install
component ships.

Spec §3.2 guessed "a few MB to ~15 MB" for this QuickJS build. The measured
stripped executable is 13.6 MiB, inside that range, of which the shader
toolchain is the largest contributor (see below). V8 (+25–35 MB) and ICU
(+1–10 MB, Q8) are not in this binary. ANGLE (+6–12 MB, Q5) is not either.

## Per-dependency breakdown

The numbers are the ELF text, data and bss of every member of each dependency's
static archives. That is an upper bound on what the dependency adds: the linker
drops unreferenced sections, which is why the sum is larger than the binary.

| Dependency | Archives | Archive bytes | text | data | bss | non-ELF |
|---|---:|---:|---:|---:|---:|---:|
| shader tools | 15 | 18399104 | 7068290 | 1521333 | 7422304 | 0 |
| QuickJS | 1 | 1674628 | 1044972 | 194109 | 76 | 0 |
| SDL3 | 1 | 7691686 | 3146911 | 846440 | 27306 | 0 |
| SoLoud | 1 | 666252 | 357830 | 56330 | 1053 | 0 |
| Effekseer | 0 | not built as a static archive | | | | |

"Shader tools" are glslang, SPIRV-Cross and SDL_shadercross, which the default
`gpu` backend links for runtime shader compilation. They are the largest
dependency, ahead of SDL3. QuickJS, which M1 replaces with V8, adds at most
about 1.2 MB of code and data.
