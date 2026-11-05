<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-11-01: document the Map 92 report tool and owner-run recipe.
Modified 2026-11-05: document the runtime's --cpu-prof recording.
How to record a CPU profile (tools/perf/attribute.py), turn a Map 92 run into
the M0 report (tools/perf/map92_report.py), and measure the shipped binary
(tools/perf/binsize.py). Repository tooling, not part of the runtime.
Never commit a recorded profile or a report taken from a game run except the
owner's after-merge Map 92 report named in docs/FORGE.md.
-->

# Performance attribution

Spec §6.1 asks one question before any V8 work starts: how much of a QuickJS
frame is interpreter time that V8's JIT removes, and how much is native work
(Canvas2D raster, texture upload, GPU submit, GC) that it would not.
`tools/perf/attribute.py` answers it from a `perf` call-graph profile, using
the rules in `tools/perf/buckets.toml`. Spec §9 also asks for a binary-size
baseline before V8, ICU (Q8) and ANGLE (Q5) land; that is
`tools/perf/binsize.py`, documented under [Binary size](#binary-size).

## Recording

No root is needed. Record user-space cycles only; kernel samples appear only
if `/proc/sys/kernel/perf_event_paranoid` allows them (1 or lower).

```sh
perf record -g --call-graph dwarf -e cycles:u -F 499 -o run.perf.data -- \
    ./build/smzuggler <game-dir> <control-mode options>
perf script -i run.perf.data > run.perf.txt
```

- Build with symbols (`RelWithDebInfo`) so Outsider's functions resolve;
  stripped binaries show up as `[unknown]` frames.
- Pin the frame count in control mode, so runs are comparable, and cross-check
  against `GameLoopStats` per phase (§6.1).
- DWARF unwinding writes large files; keep runs short (a few hundred frames).
- For V8 builds (M1 on), add `--perf-prof` (or `--perf-basic-prof`) so JIT
  frames resolve through `/tmp/perf-<pid>.map`; only then are shim JS and game
  JS separable. Under QuickJS, all JS time lands in the interpreter bucket.

## Attributing

```sh
python3 tools/perf/attribute.py run.perf.txt --json run.json --markdown run.md
python3 tools/privacy/scan.py --paths run.json run.md
```

Without `--json` or `--markdown` the Markdown table goes to stdout; with no
input file the tool reads stdin. `--rules FILE` uses another rules file. The
exit status is 0 on success and 2 on an unreadable input or a bad rule.

Each report has one row per bucket:

- **Self %**: the share of sample weight whose innermost *claimed* frame is in
  the bucket (see the rules below). Self percentages sum to 100.
- **Total %**: the share of samples with at least one frame in the bucket
  anywhere on the stack, counted once per sample. Totals overlap.
- Weight is the sample period perf printed; without one, each sample weighs 1.

The diagnostics line counts samples whose innermost frame is in the kernel,
`[unknown]` frames, and *unclaimed* samples (no non-folded rule matched any
frame). A high unclaimed count means the rules or the symbols need work. The
second table lists the 20 heaviest innermost frames with their self bucket.

All DSOs become basenames and every absolute path or `file://` URL inside a
symbol becomes its basename before it is written. Still run the privacy
`--paths` scan before sharing a report, and never commit one from a game run.

## CPU profiles from the runtime (`--cpu-prof`)

V8's own sampler answers the other half of the JS question: which game or
shim function a frame spends its time in, without `perf`, JIT symbol maps or
root. `--cpu-prof` starts the profiler before the first shim or game script
runs and writes a Chrome DevTools `.cpuprofile` when the runtime exits.

```sh
./build/smzuggler-rt --game <game-dir> --cpu-prof
./build/smzuggler-rt --game <game-dir> --cpu-prof=run/my-run.cpuprofile
```

Without `=FILE` the file is
`$XDG_STATE_HOME/smzuggler-rt/profiles/cpu-<pid>-<unix time>.cpuprofile`
(`$HOME/.local/state/smzuggler-rt/profiles/` when `XDG_STATE_HOME` is unset),
or under the `--data-dir` directory when one is passed; parent directories
are created. An explicit path is used as given. A relative path is relative
to the working directory.

Open the file with the DevTools **Performance** panel's *Load profile*
button. It is the standard JSON object with `nodes`, `samples` and
`timeDeltas`; every node names its function and source. The `--game`
directory is resolved to an absolute path first, then stripped, so script
URLs are relative to the game root (`js/main.js`, `data/...`) even when
`--game` was relative. This is how NW.js shows them, and it means a profile
can be shared without revealing the host directory layout.

The sampling interval is 100 µs. This records JavaScript only: native frames
(Canvas2D, texture upload, GPU submit, GC) still need the `perf` recording
above, so use both when a frame is split between JS and native work. Profiles
are runtime output, never commit one from a game run. The share packager
currently omits `.cpuprofile` files: adding them to its allowlist requires
a scope correction for M1-T028. Keep the profile separately when collecting
a run bundle until that change is authorized by the scope gate.

## Bucket rules

`tools/perf/buckets.toml` declares the buckets in report order, the fallback
bucket and an ordered list of rules. Each frame takes the **first** rule that
matches:

- `dso`: a list of DSO basenames, compared exactly or as `fnmatch` globs;
- `symbol`: a Python regex searched in the symbol without its `+0x` offset
  (paths in JIT symbols are still present at this point);
- a rule with both needs both; a rule with neither is an error;
- `fold = true`: the frame counts towards its bucket's total, but self time
  passes to its caller. With `bucket = ""` it counts towards no bucket.

| Bucket | Claimed by (summary) |
| --- | --- |
| QuickJS interpreter | `libquickjs*`, `JS_*`, `js_*`, `lre_*`; V8 `v8::`, `Builtins_*` and JIT builtins/stubs |
| GC | `JS_RunGC`, `gc_decref`, `gc_scan`, `free_cycles`, ...; V8 `Heap`, `Scavenger`, `MarkCompact`, ... |
| shim JS | JIT frames (`perf-*.map`, `jitted-*.so`) from `*_shim.js` or a `/shims/` path |
| game JS | every other JIT frame |
| Canvas2D | `canvas2d_*`, `font_manager_*`, `stbtt_*`, FreeType, HarfBuzz |
| renderer/GPU submit | `renderer_*`, `sprite_batch_*`, `gpu_*`, `filter_*`, `tilemap_*`, `image_upload*`, `gl*`; GL/EGL/Vulkan/DRI/NVIDIA driver DSOs |
| audio | `audio_*`, SoLoud, Vorbis, Ogg, Opus; ALSA, PulseAudio, PipeWire |
| video | `video_*`, `vpx_*`, `nestegg_*`, `libvpx*` |
| libc/other | libc, libm, libstdc++, libgcc, the loader (folded), and the fallback |

Kernel frames (`[kernel.kallsyms]`, `[vdso]`, or an address in the kernel
half when the DSO is unknown) and remaining `[unknown]` frames are folded with
no bucket. So a syscall made by the GPU driver counts as renderer time, an
unresolved helper inside Canvas2D counts as Canvas2D, and a stack that is all
kernel (an idle CPU) goes to the fallback, libc/other.

Static helpers that match no rule (`draw_text`, `blend_pixel`) are unclaimed,
so their self time also goes to the nearest claimed caller. Rules match
symbol prefixes, not source files, so check the top-frames table after a
run and add a rule when a heavy frame lands in the wrong bucket. Keep game
names out of the rules (AGENTS.md section 4).

The tests in `tests/tools/test_attribute.py` pin the exact split of the
hand-written profile in `tests/tools/fixtures/perf/`.

## Map 92 report (QuickJS, M0)

Spec §6.1 and the M0 exit ("profile report written") need one Map 92 run in
QuickJS mode turned into `docs/perf/m0-map92-profile.md`: bucket table,
mean/p95 frame time, JS update time, the §14.4 machine-state block, and a
conclusions section for Gate G1 planning. Forge workers cannot open the game,
so `tools/perf/map92_report.py` is tested on synthetic inputs. The owner runs
the real capture after merge ([DEC-M0-T018-OWNER-RUN](DECISIONS.md#dec-m0-t018-owner-run--m0-t018-builds-the-report-tool-the-owner-runs-it)).

Exact commands: [Map 92 profile recipe](perf/m0-map92-profile-recipe.md).

```sh
python3 tools/perf/map92_report.py \
    --attribution run/perf-report-map92.json \
    --stats run/runtime.log \
    --frames run/frame-times.csv \
    --machine-state run/machine.json \
    -o docs/perf/m0-map92-profile.md
python3 tools/privacy/scan.py --paths docs/perf/m0-map92-profile.md
```

Inputs:

- **attribution**: JSON from `tools/perf/attribute.py`.
- **stats**: runtime log containing GameLoopStats `perf:` lines.
- **frames**: CSV with `total_ms`, `timers_ms`, `jobs_ms`, `raf_ms`.
- **machine-state**: JSON with `power_source`, `profile`,
  `battery_start_pct`, `battery_end_pct`, `epp`.

JS update time is the mean of timers + jobs + rAF on the CSV. Attributed JS
time is the QuickJS + GC + shim JS + game JS self share of mean frame time.
A gap of more than 15% of GameLoopStats JS time is a `FLAG` in the report,
not a nonzero exit (DEC-M0-T018b). The tool copies none of the raw input
text, so a host name, user name or absolute path in the inputs cannot reach
the Markdown. This card does not commit the generated report and posts
nothing.

## Binary size

Spec §9 (M0) asks for a size baseline before V8, ICU (Q8) and ANGLE (Q5)
make the binary larger. `tools/perf/binsize.py` measures one CMake build
directory:

```sh
python3 tools/perf/binsize.py BUILD_DIR             # Markdown tables
python3 tools/perf/binsize.py BUILD_DIR --json      # the same data as JSON
python3 tools/perf/binsize.py BUILD_DIR --install-dir PREFIX
```

It reports:

- **The executable** (`SMZUGGLER_BIN` from `CMakeCache.txt`, default
  `smzuggler-rt`), stripped.
- **Every shared library** in the build tree, stripped. Symlinks and anything
  under `BUILD_DIR/tests/` are skipped. System libraries (libc, libGL) are
  outside the tree and are not counted.
- **The shims.** By default these are the `src/shims/**/*.js` files of the
  configured source tree, which is what the `runtime` install component copies.
  With `--install-dir`, the tool measures `PREFIX/shims/` of a
  `cmake --install BUILD_DIR --component runtime --prefix PREFIX` instead.
- **Per-dependency archive totals.** Every `*.a`/`*.lib` outside `tests/` is
  grouped as QuickJS, SDL3, SoLoud, Effekseer or "shader tools" (glslang,
  SPIRV, SPIRV-Cross, SDL_shadercross). Any other archive is listed under its
  own name. For each group the tool sums the archive file bytes and the ELF
  text, data and bss of the members.

The build flags (`CMAKE_BUILD_TYPE`, `CMAKE_C_FLAGS`, `CMAKE_CXX_FLAGS`) are
printed with the numbers, so a baseline always says how it was built.

How it measures:

- Everything is parsed in Python. The tool calls no `strip`, `ar` or `size`,
  so its numbers do not depend on the host's binutils.
- "Stripped" means the file size minus the sections `strip --strip-all`
  removes (`.symtab`, `.strtab`, `.debug*`) and their section headers. This
  can differ from real `strip` by a few bytes of alignment.
- PE and Mach-O files are reported at their raw size and marked
  "not ELF, unstripped". Non-ELF archive members count in the "non-ELF"
  column.
- Archive totals are an upper bound on what a dependency costs, because the
  linker drops unreferenced sections.
- The tool only reads. It exits 1 with the offending file's basename when the
  directory is not configured, the executable is missing or an ELF file is
  truncated. Exit 2 is a usage error.

Tests: `tests/tools/test_binsize.py` builds a fake build directory from ELF
objects and ar archives that it generates itself, then checks every total.

Results: [M0 baseline](perf/binary-size.md).
