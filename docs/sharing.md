<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
How to pack a run for another developer without leaking identity or game files.
The packager never uploads; posting is always a human step. See spec §14.7, §16.
-->

# Sharing a run

`tools/share/make_bundle.py` builds a zip of one run so another developer
(for example on Discord) can read logs and perf reports. It never uploads or
posts. `smzuggler-rt --share-bundle[=FILE]` is a thin wrapper that packs the
newest directory under `$XDG_STATE_HOME/smzuggler-rt/runs/` (see
[DEC-M0-T023](DECISIONS.md#dec-m0-t023--share-bundle-last-run-path-and-fail-closed-binaries)).

```sh
python3 tools/share/make_bundle.py ~/path/to/run -o run.zip
python3 tools/share/make_bundle.py --last-run -o run.zip
./build/smzuggler-rt --share-bundle=run.zip
```

`--denylist` selects a list; the default is
`~/.config/smzuggler-rt/privacy-denylist.txt`, the same file `scan.py` uses.
A missing or empty list exits 2 and writes no zip.

## What is collected

From each `RUN_DIR`:

- runtime logs (`*.log`)
- `perf-report-*.json` and `perf-report-*.md`
- frame-time `*.csv` and `*.jsonl`
- screenshots (`*.png`, `*.jpg`, `*.jpeg`) only with `--include-screenshots`

Each run is stored as `run-1/`, `run-2/`, … A `README.txt` names the collector
commit, the smzuggler version, the run's §14.4 machine-state block, and one
line per packed file.

## What is scrubbed

In text files the packager rewrites:

- the process `HOME` (or `USERPROFILE`) prefix to `~`
- the user name and host name to `[redacted]`
- every deny-list entry to `[redacted]` (literals and `re:` patterns,
  case-insensitive)

PNG `tEXt` / `zTXt` / `iTXt` / `eXIf` / `tIME` chunks and JPEG EXIF/XMP (APP1)
and COM segments are dropped. Pixel bytes are copied, not re-encoded.

The deny-list is loaded only to scrub and scan. It is never printed, logged or
copied into the zip.

## What is excluded

Each exclusion is listed on stdout (and in README.txt) by a path relative to
that run directory:

- saves: `*.rmmzsave`, `*.rpgsave`, `global*`
- anything under `--game-dir`
- game asset types `*.ogg *.m4a *.webm *.mp4 *.rpgmvp *.png_`, and `*.json`
  under `data/`, `js/`, `img/`, `audio/` or `movies/`
- screenshots, unless `--include-screenshots` is set
- symlinks (files or directories); they are not followed

## Screenshots are game content

Game screenshots are game content (§1). The packager includes them only with
`--include-screenshots`. Before posting a bundle that contains them, read the
game's terms. This is the default until SI-14 is ruled; see
[SPEC_ISSUES](SPEC_ISSUES.md).

## Review before posting

Posting is always a human step. The packager does not send the zip anywhere.

1. Open the zip. Read `README.txt` and confirm the file list matches what you
   meant to share.
2. Search the text files for `~` (expected) and for anything that still looks
   like a home path, a user name, a host name or an email.
3. Open screenshots if you included them and check they do not show account UI
   or other personal data.
4. If anything is wrong, discard the zip; do not post it.

The packager already ran `tools/privacy/scan.py --paths` on every staged file
(images after stripping) and refused to write the zip on a hit. A planted
deny-list term inside a binary file also refuses (exit 1) without creating the
zip, and diagnostics name only the file and `chunk`. Run the scan yourself on
the extracted zip if you want a second look:

```sh
python3 tools/privacy/scan.py --denylist ~/.config/smzuggler-rt/privacy-denylist.txt \
    --paths extracted/README.txt extracted/run-1/run.log
```
