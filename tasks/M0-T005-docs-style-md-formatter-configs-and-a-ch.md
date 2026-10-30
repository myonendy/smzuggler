+++
id = "M0-T005"
title = "docs/style.md, formatter configs and a changed-lines format check"
milestone = 0
deps = []
agent = "any"
owns = [".clang-format", "tools/format/**", "tests/tools/test_check_format.py", "tests/tools/test_es5_check.py"]
docs = ["docs/style.md", "CHANGELOG.md", "docs/DECISIONS.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Write `docs/style.md` from §15.6 (naming, ≤ 60-line functions, no clever macros, comments say why, ES5 for shims sent upstream, error handling, header and doc-comment format with an example). Add `.clang-format` that matches upstream's existing C style as closely as possible (measure: formatting an unchanged upstream file should produce a small diff; record the residual in the DEC). Add `tools/format/check_format.py --base REV`, which runs clang-format only over lines changed since REV in C/C++ files and fails listing file:line, and `tools/format/es5_check.py --base REV [--paths ...]`, a small tokenizer-based checker that rejects ES2015+ syntax (let/const, arrow functions, classes, template literals, destructuring, default/rest parameters, spread, `for…of`) in shim files changed since REV.

## Acceptance criteria
- [ ] `tests/tools/test_check_format.py` builds a temp git repo, commits a well-formatted file, then a badly formatted change, and asserts the tool passes on the first and fails on the second with file:line output; lines not touched by the change are never reported.
- [ ] `tests/tools/test_es5_check.py` has one failing sample per forbidden construct, ES5 look-alikes that must pass (`'=>'` inside a string or comment, a property named `class`), and runs the checker over `src/shims/*.js` at `upstream-main` to show upstream's shims pass (or lists the files that already use ES2015+ in the card report and DEC).
- [ ] Running the tool against `upstream-main` on the current tree passes.
- [ ] `docs/style.md` covers every bullet of §15.6 and links `docs/privacy.md` and `ARCHITECTURE.md` (link may be added by M0-T011 if it lands later).
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §15.6, §15.7. clang-format is available on dev-laptop; node/eslint are not assumed in the gates. JS formatting is limited to the ES5 rules for now; record DEC-M0-T005 for the JS tooling choice. A human wires `check_format.py` into `[gates]` after merge (DEC-SETUP-006).
