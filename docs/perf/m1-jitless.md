<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Map 92 JIT-less comparison preflight and owner rerun recipe (spec sections 6.1, 9 M1).
Repository documentation; keep game names, home paths and host names out.
-->

# Map 92 JIT-less comparison (M1)

Spec §6.1 step 5 and the §9 M1 exit need the same Map 92 drive measured twice:
with V8's JIT and with `--js-flags=--jitless`. The build, route, warm-up and
scored window are identical; only that one flag may differ. The card asks for
JS time (mean, p95 and p99) and memory for both modes.

**Result: BLOCKED: needs dev-laptop GUI run.** The worker session built the
runtime and validated the flag path, but it has no GPU device, is on AC and
has a full swap. No scored run exists, so this file records no timings and
invents no numbers. The owner rerun below fills the measurement table.

## Preflight (worker, 2026-11-05)

| Check | Reading | Result |
| --- | --- | --- |
| Build (`tools/forge/build.sh`) | exit 0; commit `a496d79`; V8 `13.6.233.8` | pass |
| `--js-flags` validation | unknown flag exits 2; `--jitless` accepted and reached graphics init | pass |
| Display | `DISPLAY=:0`; a Wayland socket is present | pass |
| GPU device | no `/dev/dri`; `vulkaninfo` finds no physical device | **fail** |
| Window/GPU smoke | both mode commands fail at `SDL_CreateGPUDevice`: `No supported SDL_GPU backend found`; exit 1; logs identical apart from timestamps | **fail** |
| Game copy | test copy outside the store folder; `js/main.js` and `data/Map092.json` present | pass |
| Clean copy | contains a dangling `.smz-runtime` symlink and an earlier `debug.log` | **fail** (the owner makes a fresh copy) |
| `powerprofilesctl get` | `power-saver` | pass |
| AC adapter online | `1` | **fail** |
| Battery status | `Charging` | **fail** |
| Battery capacity | 92 % at preflight start, 95 % at preflight end (above the 50 % floor) | pass |
| EPP | `power` | recorded |
| Free RAM | 6.0 GiB at preflight end | pass |
| Swap | 4.0 GiB used of 4.0 GiB (0 B free) | **fail** |
| Scored run | not started | **blocked** |
| Bundle | not made: no scored run outputs exist | **blocked** |

The session has no GPU device: `/dev/dri` is absent and Vulkan reports no
physical device, so `platform_init()` cannot open a window. The two scored
command lines differ only by `--js-flags=--jitless` and produced the same log
before the GPU check, so the flag path itself is live. That GPU failure is an
environment limitation, not a runtime or V8 failure, and it is not
measurement evidence. The adapter is online and the battery is charging, so
the laptop is on AC and has not been unplugged for the required two minutes;
the agent cannot unplug it. The swap is full, which §14.4 also forbids.

## §14.4 machine state

```
power_source: AC (AC adapter online=1)
profile: power-saver
battery_start_pct: 92 (Charging)
battery_end_pct: 95 (Charging; no scored window)
epp: power
free_ram_gib: 6.0
swap_used_gib: 4.0 of 4.0 (0 B free)
other_apps_closed: not verified
gating_state: FAIL (on AC, never unplugged; swap full; no GPU device)
```

## Measurement table

Both rows must come from one build and one runtime command line that differ
only by `--js-flags=--jitless`. JS time is the per-frame
`timers_ms + jobs_ms + raf_ms` from the scored frame-time CSV (the
GameLoopStats definition in DEC-M0-T018b). Memory is the peak resident set
size reported by `/usr/bin/time -v`, with the same wrapper on both runs.

| Mode | Scored frames | JS mean (ms) | JS p95 (ms) | JS p99 (ms) | Peak RSS (MiB) |
| --- | ---: | ---: | ---: | ---: | ---: |
| JIT (default) | not measured | not measured | not measured | not measured | not measured |
| JIT-less (`--js-flags=--jitless`) | not measured | not measured | not measured | not measured | not measured |

The worker could not fill the table, so it stays empty rather than carrying
invented or software-renderer numbers. The owner fills both rows after the
gating run below.

## Owner rerun (dev-laptop, battery)

Use the normal desktop session, not the worker sandbox: it must expose a GPU
device (`/dev/dri` present and `vulkaninfo` naming a physical device). Do this
after unplugging, with at least two minutes off AC, the battery over 50 % and
the swap not full.

1. Confirm and record the §14.4 state: `powerprofilesctl get` must be
   `power-saver`; `AC*/online` must be `0`; `BAT*/status` must be
   `Discharging`; `BAT*/capacity` must be at least 50; record the EPP.
2. Make a fresh test copy outside the store folder and point
   `SMZUGGLER_GAME_DIR` at it. Never point it at the store or a Proton tree.
3. Build once with `tools/forge/build.sh` and use that one
   `build/smzuggler-rt` for both runs. Use a scratch `HOME` and XDG roots as
   in the [Map 92 profile recipe](m0-map92-profile-recipe.md).
4. Run the M0-T018 route twice, with the same 10 s warm-up and 60 s scored
   window. The only difference is the flag:

   ```sh
   # JIT
   /usr/bin/time -v -o run/jit/time.txt \
       ./build/smzuggler-rt "$SMZUGGLER_GAME_DIR" --control --perf 1 \
       --shims src/shims

   # JIT-less
   /usr/bin/time -v -o run/jitless/time.txt \
       ./build/smzuggler-rt "$SMZUGGLER_GAME_DIR" --control --perf 1 \
       --shims src/shims --js-flags=--jitless
   ```

   Drive title -> new game -> `$gameMap.mapId() === 92`, warm up 10 s, then
   score 60 s, exactly as in the recipe's Python driver. Write each run's
   `runtime.log` and `frame-times.csv` under its own directory. The
   `--js-flags` value changes the V8 code-cache key, so a run can never
   consume a cache compiled under the other mode.
5. Compute the table with the nearest-rank percentile used by
   `tools/perf/map92_report.py`:

   ```python
   import csv, math
   rows = list(csv.DictReader(open("run/jit/frame-times.csv")))
   js = [float(r["timers_ms"]) + float(r["jobs_ms"]) + float(r["raf_ms"])
         for r in rows]
   js.sort()
   def pct(p):
       rank = max(1, math.ceil(p / 100.0 * len(js)))
       return js[min(rank, len(js)) - 1]
   print(len(js), sum(js) / len(js), pct(95), pct(99))
   ```

   Take peak RSS from `Maximum resident set size (kbytes)` in each
   `time.txt` and convert to MiB. If a V8 heap figure is wanted too, run each
   mode once more with `--trace-gc` added to `--js-flags`; keep that extra run
   out of the timing table because tracing changes the measurement.
6. Pack the two run directories with `tools/share/make_bundle.py`, record the
   bundle path and its `README.txt` summary here, and run
   `python3 tools/privacy/scan.py --paths docs/perf/m1-jitless.md` before
   committing the filled table. Nothing is posted.

If a step fails, record the observed output in this file and leave the table
empty. Do not switch to AC or a software renderer to make it pass.
