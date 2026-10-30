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
- [ ] `headers.py --base upstream-main` passes on the tree at the end of this card.
- [ ] `LICENSES/README.md` is linked from README.md.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.

## Notes
Spec §12. File types without comments (JSON, PNG) are skipped and listed in the tool's docstring.
