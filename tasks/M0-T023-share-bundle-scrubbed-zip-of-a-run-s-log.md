+++
id = "M0-T023"
title = "Share bundle: scrubbed zip of a run's logs, perf reports, CSVs and screenshots"
milestone = 0
deps = ["M0-T001", "M0-T003"]
agent = "any"
owns = ["tools/share/**", "tests/tools/test_make_bundle.py", "src/main.c", "ARCHITECTURE.md"]
docs = ["docs/sharing.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `tools/share/make_bundle.py RUN_DIR... -o FILE.zip` (also reachable as `smzuggler-rt --share-bundle[=FILE]`, which calls the script for the last run's state dir) to package a run for other developers (e.g. Outsider devs on Discord). It collects logs, `perf-report-*.json`/`.md`, frame-time CSV/JSONL files and screenshots, then:
- rewrites the user's home directory to `~` and replaces the user name, host name and every entry of the external deny-list (`~/.config/smzuggler-rt/privacy-denylist.txt`, or `--denylist`) with `[redacted]` in text files;
- strips image metadata (EXIF/XMP from JPEG, `tEXt`/`zTXt`/`iTXt`/`eXIf`/`tIME` chunks from PNG) and re-encodes nothing else;
- excludes saves (`*.rmmzsave`, `*.rpgsave`, `global*`), anything under the game dir and any game asset types (`*.ogg *.m4a *.webm *.mp4 *.rpgmvp *.png_ *.json` from `data/`, `js/`, `img/`, `audio/`, `movies/`), listing each exclusion by relative name only;
- runs `tools/privacy/scan.py --paths` over every staged file (images after stripping) and refuses to write the zip on any hit (exit 1, file and line or chunk only, never the matched text);
- writes a `README.txt` into the zip: commit hash, smzuggler version, the run's §14.4 machine-state block, and one line per file saying what it is.
The tool never uploads or posts anything.

## Acceptance criteria
- [ ] `ARCHITECTURE.md` has a directory-map row for `tools/share/` (required by `tests/tools/test_architecture_refs.py`); no other ARCHITECTURE.md change.
- [ ] `tests/tools/test_make_bundle.py`, using temp dirs and a temp deny-list with made-up terms: a log containing the temp HOME, a fake user name, a fake host name and a deny-list term produces a zip whose files contain `~` and `[redacted]` and none of the originals.
- [ ] A PNG with a `tEXt` chunk and a JPEG with EXIF (both generated in the test) come out without metadata and with identical pixel data.
- [ ] A planted deny term the scrubber cannot rewrite (e.g. inside a binary file) makes the tool exit 1 without creating the zip, and neither stdout nor stderr contains the term.
- [ ] `*.rmmzsave`, a file under a fake game dir and a fake game asset are excluded and listed; a missing deny-list exits 2 (as scan.py does), never silently skips.
- [ ] README.txt lists the commit, version, machine-state block and one line per included file; the zip passes `scan.py --paths` after extraction.
- [ ] `docs/sharing.md` explains what is collected, scrubbed and excluded, how to review a bundle before posting, and that posting is always a human step.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §14.7, §14.8 (`--scrub-report` will reuse this in M2), §16. The tool loads the deny-list at run time only to scrub and scan; it never prints, logs or copies it (AGENTS.md §3). Game screenshots are game content (§1): the tool includes them only with `--include-screenshots`, and docs/sharing.md says to check the game's terms before posting them (docs/SPEC_ISSUES.md SI-14). `--share-bundle` in main.c is a thin wrapper; keep the logic in Python.
