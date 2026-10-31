<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-10-31: confirm section 12 notices for changed upstream files.
Modified 2026-10-31: replace upstream contribution policy for smzuggler.
Contributor workflow for the runtime described in ARCHITECTURE.md; keep fork
licensing, privacy and upstream assignment rules distinct.
-->

# Contributing to smzuggler

smzuggler contributions are GPL-3.0-or-later. There is no CLA and no copyright
assignment for contributions to this fork. Preserve inherited copyright and
SPDX lines; add `Copyright (C) 2026 The smzuggler contributors` and a dated
modified note to inherited files you change. New files use
`SPDX-License-Identifier: GPL-3.0-or-later` and the project copyright line.
Do not add upstream's commercial license option to new fork files.

Read [ARCHITECTURE.md](ARCHITECTURE.md) and [style](docs/style.md) before
changing a subsystem. Keep changes focused, add failing tests first, implement
the fix, and update documentation in the same change. Explain why each public
type and function exists, including ownership, threading and error contracts.

## Identity and privacy setup

Use the project identity for both author and committer, including upstream PRs.
Set it per repository, never globally; remove any environment overrides that
would substitute a personal author or committer:

```sh
git config --local user.name myonendy
git config --local user.email 350390780+myonendy@users.noreply.github.com
tools/privacy/install-hooks.sh
```

Before committing, maintain a nonempty private deny-list at
`~/.config/smzuggler-rt/privacy-denylist.txt`, outside the repository. Each line
is a case-insensitive literal or `re:` regex. Never publish its contents.
The installer adds pre-commit and commit-msg hooks and refuses to overwrite
unrelated hooks. Hooks refuse commits without the deny-list. CI's `privacy-scan`
checks the full tree, PR commits (messages, author and committer), and reports.
Deny-list hits cannot be overridden; diagnostics identify locations without
printing matched text.

Keep personal names, emails, usernames, hostnames and absolute home paths out
of code, docs, commits, PR text, logs and fixtures. Use roles, `~`, `dev-laptop`,
and `example.invalid` instead. Never log the environment. Keep game scripts,
assets, saves and screenshots out of the repository, and never copy proprietary
wrapper code. Tests use external installations through `SMZUGGLER_GAME_DIR` or
`RMMZ_TEST_GAME_DIR`; save fixtures belong only in
`$XDG_DATA_HOME/smzuggler-rt/fixtures/`.

## Build and test

Run from the repository root with the pinned dependency source cache already
available. These commands are offline and never download dependencies:

```sh
tools/forge/build.sh
tools/forge/test.sh
tools/forge/privacy_gate.sh
tools/forge/commit_subjects.sh
tools/forge/game_names.sh
python3 tools/doclint/mdlinks.py CONTRIBUTING.md docs/upstream-CONTRIBUTING.md .github/pull_request_template.md
```

See [the dependency cache instructions](docs/FORGE.md#dependency-cache) for
cache setup. `SMZ_DEPS_DIR` selects another cache; `SMZ_BUILD_DIR` selects the
build directory. The test gate runs CTest and Python tool tests without a game.

## Pull requests and documentation

Use the [PR template](.github/pull_request_template.md) and address every item:

- Why the change is needed: describe the failure and intended behavior.
- Docs updated: change subsystem docs, module README or compat table alongside code.
- Compat row updated: record the API reference, status and deviations.
- Tests added: describe the regression and how it was verified.
- Upstream-eligible?: explain whether the fix should go to Outsider.
- Privacy scan clean (§16): check source, metadata, commits and shared reports.

CI checks file headers and function doc comments, compat-table consistency,
Markdown links and the docs-touched gate. If no documentation change is needed,
request the `no-doc-change` label with a stated justification; the reviewer must
agree. A subsystem's milestone documentation also needs fresh-eyes review by
someone who did not write it; turn every question into a documentation fix.
Use descriptive commit subjects as specified below.

## Upstream patches

U-PRs follow the preserved [upstream contribution rules](docs/upstream-CONTRIBUTING.md),
including upstream copyright assignment, rather than this fork's no-CLA policy.
The owner makes upstream's required assignment statement once under the project
identity. See [upstream workflow and tracking](docs/UPSTREAM.md).
Cut `up/<id>-<slug>` from current `upstream/main`, never smzuggler's main.
Keep one concern per PR, ideally under about 200 changed lines, with its own
test and no unrelated refactoring. Use upstream headers and style, ES5 shims,
and QuickJS CI; assume neither V8 nor real PIXI. Open dependent PRs only after
the prerequisite lands. Explain the observed failure, root cause with file and
line, fix, tests and reference behavior with a public API documentation link.
Mention plugin names and public behavior only, never game files or identities.
Track each PR and its upstream and carried smzuggler commits in docs/UPSTREAM.md.

## Commit messages (smzuggler)

smzuggler's history is read by people who were not there, so a commit
subject must say at a glance what the commit adds or changes:

- Imperative and specific, 72 characters or fewer, naming what changed and
  where: `Add removeAllListeners and once to nw.Window shim`, not `M0-T003`,
  `fix stuff`, `update` or `wip`.
- No bare card IDs and no generic subjects (`wip`, `fix`, `update`,
  `changes`, `misc`); no `forge:` prefix.
- The card ID goes in a trailer at the end of the body: `Card: M0-T003`.
- The body explains why, when that is not obvious from the subject.

`tools/privacy/install-hooks.sh` installs a commit-msg hook that enforces
these rules (`tools/commitmsg/check_subject.py`), and the forge
`commit_subjects` gate checks every new commit. Subjects written by
AgentForge itself (`M0-T003: attempt 1 by codex`) are the one known
exception until forge can take a subject from the card; see docs/FORGE.md.
