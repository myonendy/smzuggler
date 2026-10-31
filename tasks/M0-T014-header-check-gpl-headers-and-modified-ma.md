+++
id = "M0-T014"
title = "Header check: GPL headers and modified marks (§12)"
milestone = 0
deps = ["M0-T001"]
agent = "any"
owns = ["tools/doclint/headers.py", "tests/tools/test_headers.py", "README.md", "src/**", "CMakeLists.txt"]
docs = ["CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `tools/doclint/headers.py --base REV`: for files added since REV, require `SPDX-License-Identifier: GPL-3.0-or-later` and `Copyright (C) 2026 The smzuggler contributors`; for upstream files changed since REV, require the original copyright and SPDX lines unchanged, plus the smzuggler copyright line and a dated "modified" note; never allow `LicenseRef-GeneralArcade-Commercial` on a new file. `--upstream` mode (used for up/ branches) instead requires upstream's header untouched and no smzuggler line. Fix the headers of every file smzuggler has changed so far.

## Acceptance criteria
- [ ] `tests/tools/test_headers.py` covers: new file good/bad; changed upstream file with and without the modified mark; removed upstream copyright line fails; commercial LicenseRef on a new file fails; `--upstream` mode rejects the smzuggler line.
- [ ] `headers.py --base upstream-main` reports no violation in any file this card adds or changes, or in any file matching its `owns`. The full-tree output is in the card's report, and every remaining violation is in a file outside `owns` (protected paths, upstream tests, CONTRIBUTING.md, .gitignore); DEC-M0-T014 lists them for draft card M0-T014b. Do not add an exemption list for them and do not weaken a check to make them pass (DEC-M0-T014-SCOPE).
- [ ] Verbatim licence texts (`LICENSE`, `LICENSES/*.txt`) are skipped by explicit path, listed in the module docstring with the reason (the text must stay verbatim), and covered by a test. Project-written prose under `LICENSES/` is still checked.
- [ ] `LICENSES/README.md` is linked from README.md.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §12, DEC-M0-T014-SCOPE. File types without comments (JSON, PNG) are skipped and listed in the tool's docstring. Fixing headers in files outside `owns`, protected files included, is M0-T014b's job (a human does the protected ones); this card must not edit them.
