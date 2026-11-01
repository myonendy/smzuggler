<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Owner recipe for the spec section 9 M0 exit run on dev-laptop.
Repository documentation; keep game names, home paths and host names out.
-->

# M0 exit run (owner, dev-laptop)

Why this exists: workers cannot run on battery with `power-saver` or with
real graphics, so the owner does the §9 M0 exit run after merge
(DEC-M0-T022-OWNER-RUN in [docs/DECISIONS.md](../DECISIONS.md)) and records
the result in [M0.md](M0.md). Every command below exists in the repo; flags
match each tool's `--help` as of 2026-11-01.

Rules for the whole run:

- Never point `SMZUGGLER_GAME_DIR` at the Steam folder. The evaluator and the
  smoke script refuse paths containing `steamapps`, `compatdata` or `proton`,
  but do not rely on that.
- Keep every output (logs, hashes, screenshots, saves) under the scratch tree.
  Nothing is posted; posting a bundle is a separate human decision.
- If a step fails, record `fail` with the observed output. Do not switch to a
  software renderer or AC power to make it pass.

## 1. Build

From the repo root, on a `main` that contains U15:

```sh
git log --oneline -1 --grep='^U15:'      # must print a commit
tools/forge/build.sh                     # produces build/smzuggler-rt
```

## 2. Battery and `power-saver`

Unplug the charger, wait **at least 2 minutes**, then check (read only;
smzuggler never changes these):

```sh
date                                       # note the time; unplugged >= 2 min before it
powerprofilesctl get                       # must print: power-saver
cat /sys/class/power_supply/A*/online      # must print: 0
cat /sys/class/power_supply/BAT*/status    # must print: Discharging
cat /sys/class/power_supply/BAT*/capacity  # must be >= 50
cat /sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference
```

Write the readings down; they go into M0.md (E1) and the bundle. If any check
fails, stop here.

## 3. Test copy and tree hash before

Make a scratch tree and a test copy of the game outside the Steam folder.
`<steam game folder>` is only read, once, by `cp`.

```sh
export SCRATCH=~/smz-m0-exit             # any new empty directory
mkdir -p "$SCRATCH"/game "$SCRATCH"/state
cp -a "<steam game folder>"/. "$SCRATCH"/game/
export SMZUGGLER_GAME_DIR="$SCRATCH/game"
export SMZUGGLER_RUNTIME="$PWD/build/smzuggler-rt"
export XDG_STATE_HOME="$SCRATCH/state"   # evaluator output lands here

tree_hash() {
  (cd "$SMZUGGLER_GAME_DIR" &&
   find . -printf '%y %m %s %p\n' | LC_ALL=C sort &&
   find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum)
}
tree_hash > "$SCRATCH/tree-before.txt"
sha256sum "$SCRATCH/tree-before.txt"
```

## 4. Contained smoke

```sh
tools/smoke/game_smoke.sh 600
```

The only argument is an optional positive frame count (default 120). It needs
`bwrap`, runs the runtime with the game tree read-only and scratch `HOME`/XDG,
and exits 0 only if it prints `game dir unchanged` and the runtime exited 0.
Keep the last lines (`frames: ...; exit code: ...` and the verdict). If `bwrap`
cannot create a user namespace the child never starts; record that as `fail`
for this step and rely on step 8, do not count the verdict line.

## 5. Evaluator scenarios (scratch `HOME` / `XDG_*`)

```sh
python3 -m tools.evaluator.smzuggler --dry-run
python3 -m tools.evaluator.smzuggler --battery-gate-confirmed --runtime build/smzuggler-rt
```

`--dry-run` prints the game, output, scratch env and scenario without writing.
The real run copies the game into `<run>/input`, gives the child
`HOME=<run>/home` and `XDG_{DATA,CONFIG,STATE,CACHE}_HOME=<run>/xdg-*`, and
drives `tools/evaluator/scenarios/smz-basics.json` (title, new game, Map 92,
map waits, menu, save slot 1, reload, close). `<run>` is
`$XDG_STATE_HOME/smzuggler-rt/evaluator/run-*`:

```sh
export RUN=$(ls -d "$XDG_STATE_HOME"/smzuggler-rt/evaluator/run-* | tail -1)
cat "$RUN/summary.json"
```

Boot evidence (E1): steps 0, 1 and 2 have status `ok` (step 2 throws unless
`$gameMap.mapId() === 92`). The run writes `$RUN/bundle.zip` with
screenshots; it is game content, keep it local and do not share it.

## 6. One save from the map, in the scratch XDG tree

If `summary.json` shows step 9 (`DataManager.saveGame(1)`) `ok`, use that
save. Otherwise save once by hand from the map, in a scratch environment:

```sh
mkdir -p "$SCRATCH"/manual/home "$SCRATCH"/manual/xdg-data \
         "$SCRATCH"/manual/xdg-config "$SCRATCH"/manual/xdg-state "$SCRATCH"/manual/xdg-cache
env HOME="$SCRATCH/manual/home" XDG_DATA_HOME="$SCRATCH/manual/xdg-data" \
    XDG_CONFIG_HOME="$SCRATCH/manual/xdg-config" XDG_STATE_HOME="$SCRATCH/manual/xdg-state" \
    XDG_CACHE_HOME="$SCRATCH/manual/xdg-cache" \
    build/smzuggler-rt --game "$SMZUGGLER_GAME_DIR" --shims src/shims --no-audio \
    > "$SCRATCH/manual/runtime.log" 2>&1
# New Game -> on the map open the menu -> Save -> slot 1 -> quit.
export RUN="$SCRATCH/manual"   # only if you used the manual save
```

Check where slot 1 went (`<dir_save_name>` is the game's own save folder name;
do not write it into the repo):

```sh
find "$RUN"/xdg-data -mindepth 2 -maxdepth 2 \
     \( -name 'file1.rmmzsave' -o -name 'file1.rpgsave' -o -name 'forage_*file1' \) -printf '%P\n'
ls "$RUN"/input/save "$SMZUGGLER_GAME_DIR"/save 2>&1
```

Pass (E4): `find` prints exactly one `<dir_save_name>/file1.*` path, and
there is no slot file under `input/save` or the game copy's `save`.

## 7. No compat or diagnostic script

The runtime loads only the stock shims in `src/shims` (the evaluator adds
`control_shim.js` and its prelude through `--control`; that is the harness,
not a game compat script). Show it:

```sh
git status --short -- src/shims          # must print nothing
ls src/shims                             # 13 stock files, no game-specific file
grep -iE 'compat|diagnostic' "$RUN"/runtime.log || echo 'no extra script lines'
```

Pass (E2): `src/shims` is clean, no file outside the stock list, the
evaluator command line in step 5 was used as is (no extra `--shims` folder or
injected script), and the log shows no extra script.

## 8. Tree hash after

```sh
tree_hash > "$SCRATCH/tree-after.txt"
sha256sum "$SCRATCH/tree-before.txt" "$SCRATCH/tree-after.txt"
cmp "$SCRATCH/tree-before.txt" "$SCRATCH/tree-after.txt" && echo 'game dir unchanged'
```

Pass (E3): both digests are equal and `cmp` prints nothing.

## 9. Map 92 profile report

Follow [m0-map92-profile-recipe.md](../perf/m0-map92-profile-recipe.md)
with the same test copy and machine state. It ends with
`docs/perf/m0-map92-profile.md` written by `tools/perf/map92_report.py` and
checked with `python3 tools/privacy/scan.py --paths docs/perf/m0-map92-profile.md`.

## 10. Share bundle (kept, not posted)

Record battery end (`cat /sys/class/power_supply/BAT*/capacity`) and write
the step 2 readings as a machine-state report (same keys as the profile
recipe; replace the zeros and EPP with your readings):

```sh
mkdir -p "$SCRATCH/machine"
cat > "$SCRATCH/machine/perf-report-machine.json" <<'JSON'
{"machine_state": {"power_source": "battery", "profile": "power-saver",
 "battery_start_pct": 0, "battery_end_pct": 0, "epp": "power"}}
JSON
```

Pack without screenshots. For an evaluator run pack `$RUN/artifacts`; for a
manual save pack `$RUN` (only `runtime.log` qualifies; the packager skips
saves, assets and screenshots):

```sh
python3 tools/share/make_bundle.py "$RUN/artifacts" "$SCRATCH/machine" \
    --game-dir "$SMZUGGLER_GAME_DIR" -o "$SCRATCH/m0-exit-bundle.zip"
```

The packager scans for privacy findings and never uploads. Keep
`$SCRATCH/m0-exit-bundle.zip`; write its path with `~` in M0.md. If it
refuses because of a finding, fix the cause and pack again.

## 11. Update M0.md

Change only these lines of [M0.md](M0.md), from `pending owner run` to `pass`
or `fail`, adding the date, the step 2 readings (profile, power source,
battery start/end, EPP) once on E1, and this evidence:

- **E1** — `summary.json` steps 0–2 status (step 5); runtime commit (`git rev-parse --short HEAD`).
- **E2** — output of step 7 (`git status` empty, shim list, log grep result).
- **E3** — the two digests from step 8 and the step 4 verdict line and exit code.
- **E4** — the relative path `xdg-data/<dir_save_name>/file1.*` from step 6
  (use the placeholder, not the real folder name) and whether it came from
  evaluator step 9 or the manual save.
- **E5** — `docs/perf/m0-map92-profile.md` exists and passes the privacy scan (step 9).
- **E6** — the bundle path (`~/...`) and the packager's exit status (step 10).

Leave E7 and E8 as they are; E8 changes only after hosted CI runs. Finally
check the file:

```sh
python3 tools/privacy/scan.py --paths docs/milestones/M0.md
```
