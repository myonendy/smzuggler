+++
id = "M2-T008"
title = "Outside-tester kit: --scrub-report, deck-proxy scripts"
milestone = 2
deps = ["M2-T007"]
agent = "or-deepseek"
owns = ["tools/tester/**", "tools/perf/**", "src/smzuggler/**", "src/main.c", "tests/tools/test_scrub_report.py", "tests/tools/test_deck_proxy.py"]
docs = ["docs/testing.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = true
+++
## Goal
Ship the outside-tester kit (§14.8) and optional `deck-proxy` scripts (§14.9). `--scrub-report` keeps only the fixed report schema, replaces home paths with `~`, refuses to pack if local user/host names appear, writes `SCRUBBED.txt`, reuses `tools/share/make_bundle.py` (M0-T023), and **refuses save files**. `deck-proxy` is clearly labelled `gate: proxy` / `profile: deck-proxy` and is never a pass.

## Acceptance criteria
- [ ] `--scrub-report` on a fixture report directory produces an archive whose contents pass `tools/privacy/scan.py --paths` and contain `SCRUBBED.txt`.
- [ ] New tests `tests/tools/test_scrub_report.py`: scrubbed output has no host/user/home path; packing a tree that contains a `.rmmzsave` (or the project's save suffix) is refused; packing a tree that embeds the local user or host name is refused.
- [ ] `tools/perf/deck-proxy.sh` (and restore) and `tools/perf/deck-proxy.md` exist; docs state the proxy is optional and not a pass (§14.9).
- [ ] `docs/testing.md` (and/or `docs/testing-deck.md` if created under owns) describes the tester steps without personal names.
- [ ] A kit dry-run's outputs are packaged with `tools/share/make_bundle.py` (M0-T023) without a privacy hit; the bundle path and README.txt summary are in the card report. Nothing is posted anywhere by the card.
- [ ] Every new or changed file has the §12 header and every new or changed function / script entry point has a §15.1-style doc comment or header note.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §14.8, §14.9. Never collect tester names, Steam accounts, serials or host names. Caps in `deck-proxy` are first guesses until a Deck-equivalent report exists; do not present proxy numbers as a milestone sign-off.

Owner 2026-11-04/05 agent workflow: run `forge check` once when finished and once after each fix; mid-work run just the one relevant test; read files by line range instead of whole files unless under 100 lines.
