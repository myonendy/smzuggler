<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Exact commands for the owner's Map 92 QuickJS profile run (spec section 6.1).
No measured numbers live here; the generated report is written after merge.
See docs/perf.md and docs/FORGE.md (DEC-M0-T018-OWNER-RUN).
-->

# Map 92 profile recipe (QuickJS, M0)

This is the owner checkpoint after M0-T018 merges
([DEC-M0-T018-OWNER-RUN](../DECISIONS.md#dec-m0-t018-owner-run--m0-t018-builds-the-report-tool-the-owner-runs-it)).
Forge workers do not run it and must not invent profile numbers.

Do this on **dev-laptop**, on battery, with `power-saver`, unplugged at least
2 minutes, battery at least 50% at the start, other apps closed, at least 4 GB
of RAM free. The game is the test copy in `SMZUGGLER_GAME_DIR`, never a Steam
or Proton folder. A new game lands on Map 92. Warm up **10 s**, then record
**60 s**. Commands below are from the repository root.

The generated report is `docs/perf/m0-map92-profile.md` (the owner commits it;
this card does not). Nothing is posted by these commands;
[sharing](../sharing.md) is a later human step.

## 1. Machine prep

Confirm the §14.4 gating state. smzuggler must not change the profile, EPP or
CPU limits.

```sh
powerprofilesctl get
# expected: power-saver

# Power source and battery (BAT* name varies; read, do not write):
cat /sys/class/power_supply/BAT*/status
cat /sys/class/power_supply/BAT*/capacity
cat /sys/class/power_supply/A*/online
# Discharging / online 0 => battery. Capacity is battery start (need >= 50).

# EPP (intel_pstate); first CPU is enough if they match:
cat /sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference
```

Record the start values, then write the snapshot. Replace the zeros with this
run's sysfs readings after the scored window (battery end):

```sh
mkdir -p run/scratch
cat > run/machine.json <<'JSON'
{
  "power_source": "battery",
  "profile": "power-saver",
  "battery_start_pct": 0,
  "battery_end_pct": 0,
  "epp": "power"
}
JSON
```

Build with symbols so `perf` can name Outsider frames. `tools/forge/build.sh`
is Release; reconfigure the same cache as RelWithDebInfo (see
[Recording](../perf.md#recording)):

```sh
tools/forge/build.sh
# For this profile, rebuild RelWithDebInfo into build/ if the Release binary
# has no usable frames in perf script output.
```

## 2. Run

Scratch HOME/XDG so U5 `process.env` cannot see the owner's save folder.
Keep `DISPLAY` / `WAYLAND_DISPLAY` / `PATH`. `--perf 1` prints one
GameLoopStats line per frame.

```sh
export SMZUGGLER_GAME_DIR  # already set to the test copy
export HOME="$PWD/run/scratch"
export XDG_DATA_HOME="$HOME/.local/share"
export XDG_CONFIG_HOME="$HOME/.config"
export XDG_STATE_HOME="$HOME/.local/state"
export XDG_CACHE_HOME="$HOME/.cache"
mkdir -p "$XDG_DATA_HOME" "$XDG_CONFIG_HOME" "$XDG_STATE_HOME" "$XDG_CACHE_HOME"
```

Drive title → new game → Map 92, warm up 10 s, then sample 60 s. `perf record`
attaches after warm-up so attribution covers the scored window only. The
Python driver writes scored `run/runtime.log` and `run/frame-times.csv`.

```sh
python3 - <<'PY'
import csv, json, os, re, select, subprocess, time
from pathlib import Path

root = Path('.').resolve()
run = root / 'run'
host = ['stdbuf', '-oL', str(root / 'build' / 'smzuggler-rt'),
        os.environ['SMZUGGLER_GAME_DIR'], '--control', '--perf', '1',
        '--shims', str(root / 'src' / 'shims')]
log = (run / 'host.log').open('wb')
proc = subprocess.Popen(host, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT)
next_id, buf, scored = 1, b'', []

def pump(timeout_s, want_id=None):
    """Read host stdout for up to timeout_s seconds; return the matching @@ctl line."""
    global buf
    deadline = time.time() + timeout_s
    found = None
    while time.time() < deadline:
        ready, _, _ = select.select([proc.stdout], [], [], 0.25)
        if not ready:
            continue
        chunk = os.read(proc.stdout.fileno(), 4096)
        if not chunk:
            break
        log.write(chunk)
        buf += chunk
        while b'\n' in buf:
            line, buf = buf.split(b'\n', 1)
            text = line.decode('utf-8', 'replace')
            if 'perf: 1 frames avg' in text:
                scored.append(text)
            if (want_id is not None and text.startswith('@@ctl ')
                    and text.split()[1] == str(want_id)):
                found = text
                deadline = time.time()
    return found

def eval_js(js, timeout_s=30):
    """Send one control-mode eval and wait for its @@ctl reply."""
    global next_id
    ident = next_id
    next_id += 1
    proc.stdin.write(f'{ident} eval {json.dumps(js)}\n'.encode())
    proc.stdin.flush()
    reply = pump(timeout_s, ident)
    if reply is None:
        raise SystemExit('no control reply')
    return reply

eval_js('typeof SceneManager')
for _ in range(600):
    if 'Scene_Title' in eval_js('(SceneManager._scene && SceneManager._scene.constructor && SceneManager._scene.constructor.name) || ""'):
        break
eval_js('DataManager.setupNewGame(); SceneManager.goto(Scene_Map); true')
for _ in range(600):
    if eval_js('($gameMap && $gameMap.mapId && $gameMap.mapId()) || 0').rstrip().endswith('92'):
        break
else:
    raise SystemExit('did not reach map 92')
scored.clear()
pump(10)  # warm-up, not scored
perf = subprocess.Popen(
    ['perf', 'record', '-g', '--call-graph', 'dwarf', '-e', 'cycles:u',
     '-F', '499', '-p', str(proc.pid), '-o', str(run / 'run.perf.data'),
     '--', 'sleep', '60'])
scored.clear()
pump(70)
perf.wait()
ident = next_id
proc.stdin.write(f'{ident} quit\n'.encode())
proc.stdin.flush()
pump(5, ident)
proc.wait()
log.close()
(run / 'runtime.log').write_text('\n'.join(scored) + '\n', encoding='utf-8')
pat = re.compile(
    r'perf: 1 frames avg ([\d.]+) ms \(events ([\d.]+), timers ([\d.]+), '
    r'jobs ([\d.]+), rAF ([\d.]+), swap ([\d.]+)\)')
with (run / 'frame-times.csv').open('w', newline='') as handle:
    writer = csv.writer(handle)
    writer.writerow(['frame', 'total_ms', 'events_ms', 'timers_ms', 'jobs_ms',
                     'raf_ms', 'swap_ms'])
    for index, line in enumerate(scored, 1):
        match = pat.search(line)
        if match:
            writer.writerow([index, *match.groups()])
print('scored frames', len(scored))
PY
```

Re-read battery capacity into `battery_end_pct` in `run/machine.json`.

## 3. Attribute

```sh
perf script -i run/run.perf.data > run/run.perf.txt
python3 tools/perf/attribute.py run/run.perf.txt \
    --json run/perf-report-map92.json --markdown run/perf-report-map92.md
```

## 4. Report

```sh
python3 tools/perf/map92_report.py \
    --attribution run/perf-report-map92.json \
    --stats run/runtime.log \
    --frames run/frame-times.csv \
    --machine-state run/machine.json \
    -o docs/perf/m0-map92-profile.md
python3 tools/privacy/scan.py --paths docs/perf/m0-map92-profile.md \
    run/perf-report-map92.json run/perf-report-map92.md run/frame-times.csv
```

If the report FLAGs a gap of more than 15% between attributed JS time and
GameLoopStats JS time, keep the FLAG; do not edit numbers to hide it.

## 5. Share bundle

Wrap the snapshot so the packager can copy the §14.4 block into README.txt,
then pack. The packager never uploads.

```sh
python3 -c 'import json,pathlib; p=pathlib.Path("run"); m=json.loads(p.joinpath("machine.json").read_text()); p.joinpath("perf-report-machine.json").write_text(json.dumps({"machine_state": m})+"\n")'
python3 tools/share/make_bundle.py run -o run/map92-bundle.zip
```

Open the zip, read README.txt, and only then consider posting.
