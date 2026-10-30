+++
id = "M0-T013"
title = "docs/UPSTREAM.md and tools/upstream/cut_up_branch.sh"
milestone = 0
deps = []
agent = "any"
owns = ["tools/upstream/**", "tests/tools/test_cut_up_branch.py"]
docs = ["docs/UPSTREAM.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Write `docs/UPSTREAM.md`: the §8.1 table as a tracker (PR, branch, status: planned / ready to cut / opened / merged / rejected-carried / withdrawn, upstream link, notes), the §8.2 build rules, the §8.3 sync procedure (merge, not rebase; `cherry-pick -x`; `/* smzuggler: why */` comments) and the Q12 assignment note. Add `tools/upstream/cut_up_branch.sh ID SLUG COMMIT...`: creates `up/<ID>-<SLUG>` from `upstream-main` in a temporary worktree, cherry-picks the commits keeping only the paths listed for that row in docs/UPSTREAM.md, refuses if any other path or the smzuggler copyright line would land, optionally builds and runs ctest (`--build`), and never pushes.

## Acceptance criteria
- [ ] `tests/tools/test_cut_up_branch.py` runs the script against a temp git repo with fake `upstream-main`/`main` branches and asserts: the branch is created from upstream-main; only listed paths are taken; a commit touching an unlisted path is refused; a smzuggler header line is refused; no remote is contacted (no remotes configured, and the script contains no `git push`).
- [ ] The script uses a temporary worktree and leaves the caller's worktree and branch unchanged.
- [ ] `docs/UPSTREAM.md` lists U1–U14 and the UltraMode7 issue row with status `planned`.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §8. Upstream's default branch is `master`; `upstream-main` tracks it (DEC-SETUP-001). Cutting and opening PRs is the owner's step (DEC-SETUP-005).
