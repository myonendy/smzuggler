+++
id = "M0-T012"
title = "CONTRIBUTING.md and PR template for smzuggler"
milestone = 0
deps = ["M0-T008", "M0-T013"]
agent = "any"
owns = ["CONTRIBUTING.md", ".github/pull_request_template.md", "tests/tools/test_pr_template.py"]
docs = ["CONTRIBUTING.md", "docs/upstream-CONTRIBUTING.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Replace CONTRIBUTING.md with smzuggler's: GPL-3.0-or-later, no CLA; the §16 commit identity and privacy hook install; build and test commands; the §15.7 PR checklist; how upstream patches differ (§8.2, link docs/UPSTREAM.md, which M0-T013 creates; do not create or edit docs/UPSTREAM.md here). Keep upstream's text as `docs/upstream-CONTRIBUTING.md` unchanged except its header note, because U-PRs follow it. Add `.github/pull_request_template.md` with the §15.7 checklist and the `no-doc-change` justification line.

## Acceptance criteria
- [ ] `docs/upstream-CONTRIBUTING.md` is byte-identical to upstream's CONTRIBUTING.md below an added note line.
- [ ] The PR template contains every §15.7 checklist item (checked by `tests/tools/test_pr_template.py` against a list in the test).
- [ ] `tools/doclint/mdlinks.py` passes on both files.
- [ ] The new CONTRIBUTING.md keeps the "Commit messages" section (subject rules, `Card:` trailer, hook and `commit_subjects` gate) word for word, and the PR template asks for descriptive commit subjects.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §9 M0, §15.7, §16. See docs/SPEC_ISSUES.md SI-7, DEC-SETUP-013 and DEC-SETUP-014 (upstream contact address). The upstream file contains Outsider's public contact address; it is allow-listed for `docs/upstream-CONTRIBUTING.md` in tools/privacy/allowlist.txt, so keep it byte-identical. Do not obfuscate it and do not normalise it in the test.
