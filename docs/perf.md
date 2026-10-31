<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
How to record a CPU profile and split it into the spec section 6.1 buckets
with tools/perf/attribute.py. Repository tooling, not part of the runtime.
Never commit a recorded profile or a report taken from a game run.
-->

# Performance attribution

Spec §6.1 asks one question before any V8 work starts: how much of a QuickJS
frame is interpreter time that V8's JIT removes, and how much is native work
(Canvas2D raster, texture upload, GPU submit, GC) that it would not.
`tools/perf/attribute.py` answers it from a `perf` call-graph profile, using
the rules in `tools/perf/buckets.toml`.

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
