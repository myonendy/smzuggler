+++
id = "M0-T022"
title = "M0 exit check"
milestone = 0
deps = ["M0-T002", "M0-T003", "M0-T004", "M0-T005", "M0-T006", "M0-T007", "M0-T010", "M0-T011", "M0-T012", "M0-T013", "M0-T014", "M0-T016", "M0-T018", "M0-T019", "M0-T020", "M0-T021", "M0-T023", "U1", "U2", "U3", "U4", "U5", "U6", "U7", "U8", "U9", "U10", "U11", "U12", "U15"]
agent = "any"
owns = ["docs/milestones/**"]
docs = ["docs/milestones/M0.md", "CHANGELOG.md"]
tests_required = false
may_edit_tests = false
+++
## Goal
Prepare and record the §9 M0 exit so the owner can confirm it with one dev-laptop run (DEC-M0-T022-OWNER-RUN). The exit asks that smzuggler (QuickJS mode) boots the game to the map with no compat script, no game-dir writes and a correct save dir; that the profile report exists; and that all local gates pass. The card writes `docs/milestones/M0.md` (the checklist, filled in for everything that does not need the game) and `docs/milestones/m0-exit-recipe.md` (the exact owner run). It does not run the game and claims no game-run result.

## Acceptance criteria
- [ ] `docs/milestones/M0.md` has one line per §9 M0 exit criterion with a status and evidence, and passes `tools/privacy/scan.py --paths`. Items that need the real game run (boot to map, no compat script, game-dir tree hash, save location, Map 92 profile report, share bundle) say `pending owner run` and point to the recipe step; none of them says pass.
- [ ] Items the card can check without the game are filled in from real output: the local gates in `forge.toml` and the merged cards they rely on (U15 for save reads, M0-T015 evaluator, M0-T018 report tool, M0-T023 bundle). CI items that need a push are listed as pending, not as passed (SI-6).
- [ ] `docs/milestones/m0-exit-recipe.md` lists the owner run step by step: the battery and `power-saver` checks (unplugged ≥ 2 min, `powerprofilesctl get` = power-saver, battery ≥ 50%); the test copy in `SMZUGGLER_GAME_DIR`, never the Steam folder; the game-dir tree hash before and after; `tools/smoke/game_smoke.sh`; the evaluator scenarios with scratch `HOME`/`XDG_*`; one save from the map and the check that the slot file is in `$XDG_DATA_HOME/<dir_save_name>/` of the scratch tree; the run with no compat or diagnostic script and how to show that; and `tools/share/make_bundle.py` with the bundle path kept. Every command and flag it names exists in the repo (match each tool's `--help`).
- [ ] The recipe ends with exactly which lines of `docs/milestones/M0.md` the owner changes from `pending owner run` to pass or fail, and with what evidence.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §9. The real exit run happens after merge on dev-laptop on battery with `power-saver`, done by the owner (docs/FORGE.md human checkpoint), because the worker cannot meet those conditions. Keep useful text from earlier attempts on this branch, but drop any game-run result that came from an AC or llvmpipe session. Nothing is posted anywhere by the card.
