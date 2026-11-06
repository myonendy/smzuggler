<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Optional Deck-proxy recipe for spec §14.9. It describes host caps only and
ships no game file, save or personal identifier.
-->

# Optional Deck proxy on dev-laptop

`tools/perf/deck-proxy.sh` applies Deck-like CPU and GPU caps on dev-laptop so
an owner can get an early warning about Deck-specific limits. It is
**optional** and it is **a proxy, not a pass**: the laptop battery gate is the
operative gate (spec §14), and only a real Steam Deck confirmation run with a
`steam-deck` report satisfies the Deck confirmation before release. Proxy
reports carry
`profile: deck-proxy` and `gate: proxy`, and the analyser never turns them into
a Deck pass.

The numbers below are first guesses, not calibrated. Change them only after a
real Deck-equivalent report exists; record the new values here.

## Caps applied

| Cap | Value | Why |
| --- | --- | --- |
| CPU threads | `taskset -c 0-7` | The Deck has four cores and eight threads; on dev-laptop CPUs 0-7 are the performance cores. |
| CPU maximum clock | 2.8 GHz | The Deck boosts to 3.5 GHz, but its older cores do less work per clock. |
| GPU maximum clock | 1250 MHz | The Intel integrated GPU peaks near the Deck's arithmetic throughput. |
| Display | 1280x800 at 60 Hz | A nested `gamescope` session matches the Deck panel. |
| Power | AC, `balanced` | Measure the caps, not the battery governor. |

`deck-proxy.sh` writes each previous value under
`/run/smzuggler-deck-proxy/` before changing it. `--restore` puts every value
back and removes that state directory. Run both commands with root.

## Run the proxy

```sh
sudo tools/perf/deck-proxy.sh
gamescope -W 1280 -H 800 -r 60 -- \
  taskset -c 0-7 ./build/smzuggler-rt --game "<game folder>" \
  --perf-suite=deck-proxy --frame-stats="$HOME/perf/"
python3 tools/perf/analyse.py --dir "$HOME/perf" --out-dir "$HOME/perf"
sudo tools/perf/deck-proxy.sh --restore
```

`--run -- COMMAND...` applies the caps and then runs `COMMAND` under
`taskset -c 0-7` as the sudo caller, with that user's `HOME`, so the game does
not write its state into root's directories. A direct root invocation has no
caller to drop to and keeps root. The caller still restores afterwards. If a
cpufreq policy, the Intel GPU frequency node or `powerprofilesctl` is missing,
the script says so and continues with the caps it could apply.

The suite writes the same frame streams as any other profile. The analyser
writes `perf-report-deck-proxy-<scene>.json` and `.md` with `gate: proxy`.
That word is evidence, never a milestone sign-off.

## Calibration after the first Deck report

1. Run S1-S3 on `deck-proxy` with the commit the Deck report names.
2. Compare the p50 and p99 work times, scene by scene.
3. Adjust the caps up or down and record the new values in this file.
4. Keep the `profile: deck-proxy` and `gate: proxy` labels unchanged.

Until that comparison is recorded, treat proxy numbers as early warnings only.
