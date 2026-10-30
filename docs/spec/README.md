# docs/spec: the project specification

This folder holds the specification that forge's planner, workers and reviewers read.
It is a protected path: agents never edit it. Changes come from the project owner.

| File | What it is |
|---|---|
| `SPEC.md` | **The current spec, v0.5.1** (2026-10-30), verbatim. Upstream name: `V8_MZ_HOST_SPEC.md`. This is the file `forge.toml` points at. |
| `V8_MZ_HOST_SPEC_v0.2.md` | v0.2, kept because v0.5.1 carries module internals forward from it "by reference" (see SPEC.md Appendix B). |
| `CHANGES.md` | What changed in each spec version. |

v0.3, v0.4 and v0.5 are superseded and are not kept here; SPEC.md lists what changed.

**Reading rule for cards.** A card cites the current spec as `§N.M` (for example `§5.2`).
It cites v0.2 in words ("v0.2 section 9.2"), never with the section sign, because forge's
context pack resolves every `§N` against SPEC.md only.

**Spec issues** found while setting up forge are listed in `docs/SPEC_ISSUES.md`.
