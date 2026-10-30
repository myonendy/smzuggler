+++
id = "M0-T007"
title = "Docs-touched gate tool"
milestone = 0
deps = ["M0-T006"]
agent = "any"
owns = ["tools/doclint/docs_touched.py", "tests/tools/test_docs_touched.py"]
docs = ["docs/docs-map.toml", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `tools/doclint/docs_touched.py --base REV`: maps source areas to the docs that must change with them using `docs/docs-map.toml` (e.g. `src/shims/**` → `docs/shims.md` and `docs/compat-surface.md`, `src/bindings/**` → `docs/js-binding.md`, new top-level `src/` dirs → `ARCHITECTURE.md`). Fails when a mapped source path changed and none of its docs did, unless the `NO_DOC_CHANGE` env var is set by CI from a `no-doc-change` PR label; with the label, the PR template's justification line must be non-empty (checked by reading `PR_BODY` when present).

## Acceptance criteria
- [ ] `tests/tools/test_docs_touched.py` covers: mapped change with doc change passes; without fails naming the source path and expected docs; unmapped paths pass; `NO_DOC_CHANGE=1` with an empty justification fails, with a justification passes.
- [ ] `docs/docs-map.toml` exists, covers every directory under `src/` and `tools/`, and the tool reports unmapped new directories.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.

## Notes
Spec §15.3, §15.7. Docs that do not exist yet may be listed; the tool reports them as "to be created".
