+++
id = "M0-T012"
title = "CONTRIBUTING.md and PR template for smzuggler"
milestone = 0
deps = ["M0-T008"]
agent = "any"
owns = ["CONTRIBUTING.md", ".github/pull_request_template.md", "tests/tools/test_pr_template.py"]
docs = ["CONTRIBUTING.md", "docs/upstream-CONTRIBUTING.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Replace CONTRIBUTING.md with smzuggler's: GPL-3.0-or-later, no CLA; the §16 commit identity and privacy hook install; build and test commands; the §15.7 PR checklist; how upstream patches differ (§8.2, link docs/UPSTREAM.md). Keep upstream's text as `docs/upstream-CONTRIBUTING.md` unchanged except its header note, because U-PRs follow it. Add `.github/pull_request_template.md` with the §15.7 checklist and the `no-doc-change` justification line.

## Acceptance criteria
- [ ] `docs/upstream-CONTRIBUTING.md` is byte-identical to upstream's CONTRIBUTING.md below an added note line.
- [ ] The PR template contains every §15.7 checklist item (checked by `tests/tools/test_pr_template.py` against a list in the test).
- [ ] `tools/doclint/mdlinks.py` passes on both files.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.

## Notes
Spec §9 M0, §15.7, §16. See docs/SPEC_ISSUES.md SI-7.
