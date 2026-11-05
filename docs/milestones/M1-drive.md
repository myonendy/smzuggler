<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
M1 V8 test-drive preflight and owner rerun recipe (spec sections 2 and 9).
Repository documentation; keep game names, home paths and host names out.
-->

# M1 test drive on V8 (spec §2, §9)

Why this exists: the §9 M1 exit repeats the §2 drive on V8 with Outsider's
renderer. The card requires dev-laptop on battery with `power-saver`,
unplugged for at least two minutes, and a test copy of the game in
`SMZUGGLER_GAME_DIR`. A worker session built and preflighted the runtime but
could not open a GPU window, and could not truthfully satisfy the battery
condition, so the guarded entry point refused the run. This file records that
preflight, gives every §2 row a status, and leaves the measured run and its
bundle to the owner on dev-laptop.

**Result: BLOCKED: needs dev-laptop GUI run (no GPU device in the worker
session; battery gate not confirmed).** The X display and a clean test copy
were available, but the session has no GPU device and the machine is on AC.

## Preflight (worker, 2026-11-05)

| Check | Reading | Result |
| --- | --- | --- |
| V8 build (`tools/forge/build.sh`) | exit 0; commit `810ddd7`; V8 `13.6.233.8` | pass |
| Display | `DISPLAY=:0` reachable; a Wayland socket is present | pass |
| GPU device | no `/dev/dri`; `vulkaninfo` finds no physical device | **fail** |
| Window/GPU smoke | `SDL_CreateGPUDevice` fails: `No supported SDL_GPU backend found` | **fail** |
| Clean test copy | staged outside the store folder; no symlinks | pass |
| `powerprofilesctl get` | `power-saver` | pass |
| AC adapter online | `1` | **fail** |
| Battery status | `Charging` | **fail** |
| Battery capacity | 69 % (over the 50 % floor) | pass |
| EPP | `power` | recorded |
| M0-T015 entry point | exit 2, `BLOCKED: needs dev-laptop GUI run (battery gate or runtime unavailable)` | **blocked** |

The session has no GPU device: `/dev/dri` is absent and Vulkan reports no
physical device, so `platform_init()` fails and the runtime cannot open its
window. The same failure appears in the `platform` CTest and in the contained
read-only smoke (`tools/smoke/game_smoke.sh 120`), which reaches the runtime
and then prints `No supported SDL_GPU backend found`. That is an environment
limitation, not a V8 crash, and it is not drive evidence. The `--dry-run`
resolved the game, runtime, scenario and scratch paths before the graphics
check.

The adapter is online and the battery is charging, so the laptop is on AC and
has not been unplugged for the required two minutes. The agent cannot unplug
it. Passing `--battery-gate-confirmed` would assert a state that was not true,
so the drive was not started even if the GPU were available.

The pre-existing local test copy is not directly usable by the smoke driver:
it contains a dangling `.smz-runtime` symlink and artifacts from earlier runs.
The preflight used a clean dereferenced copy. The owner should make a fresh
copy from the store folder (step 2 below).

## §2 rows: QuickJS baseline versus V8

The QuickJS column is the §2 evidence. The V8 column is deliberately not
filled where the drive did not run; no number is invented.

| # | §2 observation (QuickJS) | V8 on this attempt | Reason |
| --- | --- | --- | --- |
| 1 | Title runs at ~59.8 fps; JS 1.4–2 ms per frame | not measured | No GPU device in the worker session; no drive |
| 2 | Map 92 runs at 6.8 fps; `dt_avg` 112–166 ms, `js_update_avg` 88–156 ms, max 1,045 ms and 3,601 ms | not measured | No GPU device in the worker session; no drive |
| 3 | Mode 7 floor is black | not measured | Drive did not run (no GPU); M2 replaces the shim and owns Mode 7 |
| 4 | `renderer` lacks `renderTexture`, `filter`, `mask`, `context`, `buffer`; plugins `extract`, `rpgtilemap`, `um7tilemap` missing | not measured | Drive did not run (no GPU); M1 changes the JS engine, not Outsider's renderer |
| 5 | Crash in `Mimosa_MouseCursor` (`styleParent` undefined) | not exercised | Drive did not run (no GPU); §5.1 fix was pre-existing |
| 6 | `CleanExit` throws on `removeAllListeners` | not exercised | Drive did not run (no GPU); §5.2 fix was pre-existing |
| 7 | `CGMZ_Core` throws: `PIXI.Circle` undefined | not exercised | Drive did not run (no GPU); §5.4 fix is outside M1 |
| 8 | Save went to `undefined/.local/share/<dir_save_name>/file19.rmmzsave`; save and load worked | not exercised | Drive did not run (no GPU); the scenario's save and reload steps are the test |
| 9 | HUD panels drew without text; message-window text drew | not measured | Drive did not run (no GPU); §5.5's root cause is M3 |
| 10 | `Scene_Menu` reached; 18.9 ms average at f=2880 (53 fps), 56.9 ms at f=2820 | not measured | No GPU device in the worker session; no drive |
| 11 | Driver commands later timed out ("no reply to command … within 30s") | not measured | No GPU device in the worker session; no driver session |

Rows 1, 2, 5–7 and 11 are the categories the card names explicitly (fps,
`dt_avg`, `js_update_avg`, max frame, crashes, driver timeouts). Rows 3, 4 and
8–10 are kept because they are also §2 rows; omitting them would hide part of
the comparison.

## Owner rerun (dev-laptop, battery)

The build is already green. Use the normal desktop session, not the worker
sandbox: it must expose a GPU device (`/dev/dri` present and `vulkaninfo`
naming a physical device; the worker session had neither). Do this after
unplugging, with at least two minutes off AC and the battery over 50 %.

1. Confirm the machine state and write it down: `powerprofilesctl get` must be
   `power-saver`; `AC*/online` must be `0`; `BAT*/status` must be
   `Discharging`; `BAT*/capacity` must be at least 50; record the EPP.
2. Make a fresh test copy outside the store folder and point
   `SMZUGGLER_GAME_DIR` at it. Never point it at the store or a Proton tree.
3. Use a scratch state root:
   `export XDG_STATE_HOME=<scratch>/state`.
4. Run the guarded entry point:

   ```sh
   python3 -m tools.evaluator.smzuggler \
       --battery-gate-confirmed --runtime build/smzuggler-rt
   ```

   This drives `tools/evaluator/scenarios/smz-basics.json`: title, new game,
   `$gameMap.mapId() === 92`, 60/300/600 map frames, menu, save slot 1,
   reload and a clean `nw.Window.get().close()`. It copies the game under
   the run, uses scratch `HOME`/XDG and runs the runtime with `--perf 1`.
5. Fill the V8 column from the run:
   - fps = 1000 / mean `total_ms` in `frame-times.csv`;
   - `dt_avg` = mean `total_ms`;
   - `js_update_avg` = mean of `timers_ms + jobs_ms + raf_ms`;
   - max frame = maximum `total_ms`;
   - crashes = any non-`ok` row in `summary.json` (the run stops at the first
     failed step);
   - driver timeouts = a `DriverError`/timeout in the runtime or driver log.
   Window the means by the frame ranges in `steps.csv` so frozen frames
   between steps do not dilute the result.
6. The entry point writes `<run>/bundle.zip` with the logs, CSV/JSON reports
   and screenshots, all passed through `tools/share/make_bundle.py`. Record
   the bundle path (write `~` for the home directory) and the `README.txt`
   summary, then run
   `python3 tools/privacy/scan.py --paths docs/milestones/M1-drive.md`
   before committing the filled table.

Nothing is posted. If a step fails, record the observed output and do not
switch to AC or a software renderer to make it pass.
