<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Fixture-save recipe for the §14 perf suite (spec §14.10).
Repository documentation; it ships no save file and names no personal path.
-->

# Perf fixture saves

The §14 suite starts S2, S3, S8 and S9 from one save and later scenes from two more. No fixture ships with smzuggler: each person makes their own saves in normal play and imports each one once. Import only reads the source slot, and the suite never writes a fixture.

| Fixture | Used by | Where it must be | Why |
| --- | --- | --- | --- |
| `fx-map92` | S2, S3, S8, S9 | Map 92, standing, no message open | fixed walk-loop start |
| `fx-trade` | S4 | Map 92 or 93, near the store door, with money and cargo | the trade screen cannot be saved into |
| `fx-combat` | S5 | Map 55, at sea beside an encounter event | the duel cannot be saved into |

S1-S3 need only `fx-map92`. Fixtures live in `$XDG_DATA_HOME/smzuggler-rt/fixtures/` (fallback `~/.local/share/smzuggler-rt/fixtures/`) under the exact names above.

## Import a save slot

Make each save in any slot during normal play, point `SMZUGGLER_SAVE_DIR` at the game's own save folder, and run one import per fixture:

```sh
export SMZUGGLER_SAVE_DIR="<game save folder>"
./build/smzuggler-rt --import-fixture 12 fx-map92
./build/smzuggler-rt --import-fixture 4  fx-trade
./build/smzuggler-rt --import-fixture 6  fx-combat
```

The slot numbers are examples. Import reads `file<slot>.rmmzsave`, inflates its zlib-deflated MZ JSON (plain JSON is also accepted for diagnostics), checks its map id, refuses a missing slot or a save on a map the fixture does not accept, and copies the bytes to the fixture name. Encoded and decoded saves are limited to 64 MiB; malformed compressed saves are refused. The source is never written. A refusal is a non-zero exit with one line naming the fixture and both maps.

## Run and analyse

```sh
./build/smzuggler-rt --game "<game folder>" \
    --perf-suite=dev-laptop --frame-stats="$HOME/perf/"
python3 tools/perf/analyse.py --dir "$HOME/perf" --out-dir "$HOME/perf"
```

`--frame-stats` must name a directory (end it with `/` or create it first). The suite writes one `frame-stats-<profile>-<scene>-run<N>.jsonl` per scene and run, runs three times, and the analyser uses the worst run. `--perf-suite` refuses `--control`, `--frames` and `--import-fixture`, because each would make the fixed windows meaningless. Reports are `perf-report-<profile>-<scene>.json` and `.md` with the §14.7 `gate` word; a `deck-proxy` or `extra` report is never a gating pass.

A fixture is the person's own save and stays outside the repository. Never commit a save, a fixture, or a listing with a home path. The runtime reports profile and scene names only; see [sharing.md](sharing.md) before packing a run.
