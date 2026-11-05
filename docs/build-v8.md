<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
How the pinned V8 static monolith is built, installed and shipped to CI.
Spec §4.1, §9 M1 and §11 Q8/Q9; the pin is recorded in docs/DECISIONS.md.
-->

# Building V8

smzuggler embeds V8 directly (spec §4.1). It is built from source with gn and
ninja as one static library, `libv8_monolith.a` (v0.2 §11.2 option A), by
`tools/v8/build_v8.sh`. Contributors normally never run it: they use the CI
tarball described below.

## The pin

**Proposed pin: V8 `13.6.233.8`** (spec §11 Q9, DEC-M1-T001). It is a proposal
until the owner approves it at checkpoint H1 (docs/FORGE.md); M1-T002 does not
merge before that.

Why this version:

* 13.6 is the V8 branch of Chrome 136 and of the Node.js 24 LTS line, so it is
  a branch that a large, long-lived embedder (Node) builds against without
  Chromium, with `use_custom_libcxx=false`. Embedding problems on it are
  already known and searchable.
* It is new enough for everything the game's NW.js-era JavaScript and
  PIXI 5.3.12 need, plus the Fast API calls of §4.2 and the code cache of
  §4.1 item 4.
* A fixed tag (not a branch head) makes builds reproducible: `gclient sync`
  checks out exactly the dependencies listed in that tag's `DEPS`.

The tag was chosen offline; the worker that proposed it could not query the V8
repository. The script verifies the checkout against
`include/v8-version.h` and stops if the tag does not exist or does not match.
Moving the pin is a one-line change of `V8_VERSION` plus a new decision entry.

## Usage

```sh
tools/v8/build_v8.sh --print-args   # print the GN args; fetches and writes nothing
tools/v8/build_v8.sh                # fetch, build and install (hours)
```

`SMZ_DEPS_DIR` (default `~/.cache/smzuggler-dev/deps`, the same cache as
`tools/forge/build.sh`) receives everything:

| Path under `$SMZ_DEPS_DIR` | Content |
|---|---|
| `depot_tools/` | Shallow clone of depot_tools; self-update is turned off and its commit is recorded in the install |
| `v8-cache/` | depot_tools' CIPD, vpython and XDG caches, redirected here so nothing lands in `~` |
| `v8-src/` | `.gclient` and the V8 checkout at the pinned tag; build output in `v8-src/v8/out/smz-<version>-<cpu>/` |
| `v8/<version>/` | The install: `include/`, `lib/libv8_monolith.a`, `share/icudtl.dat`, `args.gn`, `depot_tools.rev`, `v8-config.cmake` |

The install is staged in `v8/.<version>.tmp` and renamed at the end, so an
interrupted run never leaves a half-installed version. The host build uses
`include("$SMZ_DEPS_DIR/v8/<version>/v8-config.cmake")` and links
`V8::v8_monolith`; the manifest also sets `V8_VERSION` and `V8_ICU_DATA`.

## Using V8 from CMake (`SMZ_JS_ENGINE`)

The host build chooses its JavaScript engine with the cache option
`SMZ_JS_ENGINE`: `quickjs` (default, Outsider's engine, unchanged) or `v8`.

```sh
cmake -S . -B build-v8 -DSMZ_JS_ENGINE=v8 \
      -DSMZ_V8_DIR="$SMZ_DEPS_DIR/v8/13.6.233.8" ...   # plus the usual dep flags
ctest --test-dir build-v8 -R v8_smoke --output-on-failure
```

In v8 mode the top-level `CMakeLists.txt` includes `cmake/V8.cmake`, which:

* checks `SMZ_V8_DIR` before any dependency is configured, and stops with a
  message naming `SMZ_V8_DIR` if it is unset, not a directory, its
  `include/v8-version.h` is not the pin (`SMZ_V8_VERSION_PIN`, equal to
  `V8_VERSION` in `build_v8.sh`), or `lib/libv8_monolith.a` or
  `share/icudtl.dat` is missing;
* defines the imported static target `smz::v8`: the monolith, its headers,
  `V8_COMPRESS_POINTERS` and `V8_31BIT_SMIS_ON_64BIT_ARCH`, C++20, threads and
  libdl;
* sets `SMZ_V8_ICU_DATA`, the `icudtl.dat` path V8 needs at startup.

The version is read from the headers, not from `v8-config.cmake`
(DEC-M1-T002). The test `v8_smoke` (`tests/smz/test_v8_smoke.cc`) is built
only in v8 mode: it creates an isolate and a context, runs `1+1`, and checks
that `V8::GetVersion()` equals the pin, printing both. Until the engine port
lands, v8 mode still runs the game on QuickJS; it only adds `smz::v8` and the
smoke test. The forge gates build quickjs mode; adding a v8-mode build to them
is human checkpoint H2 (DEC-SETUP-006).

## GN args

One line of why for each (`--print-args` prints exactly these):

* `is_debug = false`: release build; debug V8 is several times slower and the
  §14 frame budget is measured on release.
* `target_cpu`: the host's `x64` or `arm64`, the two 64-bit targets of spec §3.
* `v8_monolithic = true`: one static `libv8_monolith.a`, so the host links one
  archive and ships one binary.
* `is_component_build = false`: required by the monolith; no shared V8 libraries.
* `use_custom_libcxx = false`: build against the system C++ library that the host
  links, so `std::` types at the seam have one ABI.
* `v8_use_external_startup_data = false`: the startup snapshot is linked in, so
  there is no `snapshot_blob.bin` to find at run time.
* `v8_enable_i18n_support = true`: `Intl` and `toLocaleString` match NW.js
  (v0.2 recommendation for Q8; the data-size choice is revisited in M6).
* `icu_use_data_file = true`: ICU data stays a separate `icudtl.dat`, which is
  what v0.2 §11.2's tarball ships and lets M6 swap in a filtered file (Q8).
* `v8_enable_pointer_compression = true`: what Chrome and NW.js ship, so heap
  layout and memory use match them; it roughly halves pointer-heavy heap size,
  which matters on the Deck. The 4 GB per-isolate heap cage is far above the
  game's needs. Headers must then be compiled with `V8_COMPRESS_POINTERS` and
  `V8_31BIT_SMIS_ON_64BIT_ARCH`, which `v8-config.cmake` sets.
* `v8_enable_sandbox = false`: the sandbox forbids ArrayBuffers over memory
  outside its cage, which would break the no-copy buffer views of `jsb`
  (§4.1 item 1). The game runs trusted local code, so the hardening buys little.
* `v8_enable_webassembly = true`: the game's Effekseer runtime is wasm (Q10).
* `symbol_level = 0`: no debug symbols; keeps the archive and link time down.
* `treat_warnings_as_errors = false`: a newer host compiler must not fail a
  pinned third-party build on new warnings; this changes no generated code.

## Disk and time cost

Estimates [G], to be replaced by the first measured run at H1:

* checkout with `--no-history`, including the clang toolchain and sysroots:
  about 5–8 GB;
* build output: about 2–4 GB; the install itself: about 60–120 MB (the
  archive dominates);
* build time: about 30–90 minutes on 8 cores (v0.2 §11.2), longer on a laptop
  on battery. The owner runs it once on dev-laptop; workers never do.

## How CI gets V8 (§9 M1 "V8 CI tarball")

CI never runs gn. A CI job on each target runs `build_v8.sh` once per pin and
packs `$SMZ_DEPS_DIR/v8/<version>/` as `v8-<version>-<os>-<cpu>.tar.xz` with
its SHA-256. The tarball is published as a release asset, and the host's CMake
fetches it by that hash into `$SMZ_DEPS_DIR/v8/<version>/` (v0.2 §11.2), so
CI and contributors use byte-identical V8 builds. Rebuilding is needed only
when the pin or the args change; `args.gn` inside the tarball says which.
The publishing job and the CMake fetch are later M1 cards; this page fixes
the layout they share.

The Linux x64 build runs by hand as `.github/workflows/v8-build.yml`
(`workflow_dispatch` only). It uploads `v8-<version>-linux-x64.tar.xz` and its
`.sha256` as a workflow artifact; publishing it as a release asset is still a
later card.

A free runner job stops at 6 hours, so the build is resumable: the build step
is capped at 320 minutes, and the ccache directory is saved with
`actions/cache/save` even when the step fails or times out, then restored at
the start of the next run. `SMZ_V8_CC_WRAPPER=ccache` makes `build_v8.sh` add
`cc_wrapper = "ccache"` to `args.gn`; it changes no generated code. Rerunning
the workflow after a timeout recompiles only what the cache lacks.
