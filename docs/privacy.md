<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The smzuggler contributors -->
<!-- Privacy boundary for contributors and CI. Implements spec §16 and §14.7;
     keep secrets outside the checkout and scan before sharing any report. -->

# Privacy and commit hygiene

## Scope: code, docs, commits, logs, reports and fixtures

No personal names, email addresses, user names or host names belong in these
materials. The rule also covers PR text, upstream contributions and logs shared
outside the project. Never commit game content or save fixtures.

## Safe replacements

Use “the project owner” or a role such as “the reviewer” for a person, `~`,
`$HOME` or `<home>` for a home directory, and target profiles such as
`dev-laptop` and `steam-deck` for machines. Email examples use the project
noreply identity or the `example.invalid` domain.

## Copyright lines

Use `Copyright (C) 2026 The smzuggler contributors`. Preserve upstream headers;
upstream offers carry the copyright required by spec §8.2.

## Commit identity and messages

Both author and committer must use the project identity defined in spec §16
and `tools/privacy/scan.py`. Set `git config user.name` and `git config
user.email` for this repository only, never globally. Messages and PR text
must follow the privacy rules too. Follow [Commit messages](../CONTRIBUTING.md)
for imperative, specific subjects of at most 72 characters and card trailers.
The CI `commit-subjects` job checks PR base-to-head or push before-to-after
ranges with full history, independently of the privacy job. For new branches,
tags, or force pushes whose old tip is unavailable, it checks the default
branch tip through the pushed tip. If the pushed tip is the default branch
tip itself, it checks the full reachable history. An unavailable default
branch reference fails the job rather than bypassing the check.

## Runtime logs and environment

The logger replaces the home prefix with `~` before writing. Never log the
host name, user name or full environment, including `process.env` even though
it is visible to game code. Apply the same filter to crash and performance
reports. Logger tests must supply a fake home path and host name and verify
that neither survives.

The allocation-free `log_scrub` boundary reads `HOME` (`USERPROFILE` on
Windows), accepts either slash style, and preserves similarly named sibling
paths. Console, file and callback messages, including file-open failures, pass
through it. Keep environment changes serialized with logging. The runtime does
not query user or host identity for diagnostics.
Boundary regression tests also check punctuation, sibling paths and bounded
output, including preservation of bytes beyond the destination capacity.
Punctuation ends a home prefix unless it plausibly continues a sibling filename.
Root-only homes (a single separator or drive root) are ignored to preserve
ordinary diagnostics. Windows home matching ignores ASCII letter case.
Both logger regression fixtures use POSIX temporary-directory and descriptor APIs
and are registered only on non-Windows platforms; Windows builds still compile
the production filter and run the existing engine tests.

## Local hooks

Create the private deny-list first, then run `tools/privacy/install-hooks.sh`.
It installs the pre-commit and commit-msg hooks in the common Git directory
for this clone's worktrees and refuses to overwrite unrelated hooks. The
pre-commit scan checks staged added lines, added file names and image metadata;
the commit-msg hook checks the message and commit subject. Both enforce project
identity. Missing configuration refuses commits rather than disabling checks.

## CI scans and report uploads

[privacy.yml](../.github/workflows/privacy.yml) runs `privacy-scan` on every
push and pull request. It checks the full checked-out head tree with
`tools/privacy/scan.py --tree HEAD`; PRs also use `--range BASE..HEAD` to
check identities, messages and added lines. Full history is fetched, and the
PR head is checked out rather than the synthetic merge commit.

CI writes the `PRIVACY_DENYLIST` repository secret directly to a mode-600
file under `$RUNNER_TEMP`, outside the checkout, passes `--denylist`, removes
the environment copy and deletes the file on exit. It never prints the secret.
An unavailable, empty or comments-only secret causes scanner exit 2 and fails
the job, including fork PRs without access to secrets. There are no bypasses.

Generated reports belong in `reports/` or `test-results/`; performance files
use `perf-report-*`. The workflow scans these files with `--paths`. It does
not upload artifacts. Any future upload step must follow successful scans,
run after report generation in the same job, and scan every uploaded file
with `--paths`, including reports outside these directories. Never upload
unchecked output from another job or from a failed run.

## Built-in generic checks

The scanner rejects absolute home paths on Linux, macOS and Windows,
non-noreply email addresses (except the `example.invalid` domain), shell prompt
patterns containing user and host, and private text in PNG/JPEG metadata.
It rejects save file names ending in `*.rmmzsave` or `*.rpgsave`; keep fixture
saves outside the repository. Tree scans check tracked file names and content;
`--paths` checks report file names and content, including image metadata.

## Commit identity checks

Range scans check the author and committer against the exact project identity
and scan each non-merge commit's message. Select ranges after the upstream
history so inherited identities are not treated as new project commits.

## Private deny-list location and format

Store the local list at `~/.config/smzuggler-rt/privacy-denylist.txt`, never in
the repository. Protect its permissions. For a different local location use
`SMZUGGLER_PRIVACY_DENYLIST` or `--denylist`. CI uses `PRIVACY_DENYLIST`.

Use one entry per line: a literal string, or `re:` followed by a Python regular
expression. Matching is case-insensitive. Blank lines and lines beginning
with `#` are ignored; surrounding whitespace is stripped. At least one active
entry is required. Include the private names, email addresses, user names and
host names that need protection; never paste the real list into issues or logs.

## Scan output

Findings use `privacy: <file>:<line>: <rule>` and a final finding count.
Commit findings use a commit identifier as the location; identity findings
have line zero. The scanner reports location and rule, never matched text.
Exit 0 means clean, 1 means findings, and 2 means configuration or invocation
failure. Do not add context dumps or print offending lines when troubleshooting.

## No overrides and reviewed allow-list entries

There is no skip flag or override: a deny-list hit can never be allowed.
Change false-positive text first. Generic-rule exceptions may be proposed in
`tools/privacy/allowlist.txt` as three tab-separated fields: rule, path glob,
and regex. A reviewer must confirm that the exact generic match is harmless,
that the path and expression are narrowly scoped, and that the entry contains
no private material. Review the change through the normal PR process; never
use an allow-list entry to suppress a deny-list finding.

## History incidents

If a leak reaches `main`, fix it with a new commit and extend scan coverage.
Rewriting public `main` is a last resort requiring the owner's explicit decision
because it breaks existing clones. Do not rewrite history as routine cleanup.

## Report scrubbing and performance CI (§14.7)

Each device run writes `perf-report-<profile>-<scene>.json` and a short Markdown
summary. Include profile names only, never host names, user names or home paths.
Filter crash and performance output before writing and scan both JSON and
Markdown before sharing or uploading, using the same deny-list as commits.

Preserve the `gate` field: `pass`/`fail` for the laptop gating run,
`confirm-pass`/`confirm-fail` for Deck confirmation, `extra` for the AC/`balanced`
laptop run, or `proxy` for `deck-proxy`. Only a `steam-deck` report confirms
Deck testing. CI runs S1–S3 on a fixed Linux runner and fails on a greater than
20% p99 regression. Manual laptop gates occur at M2, M6 and M7, Deck confirmation
at M7; both are rerun before every release. Scrubbing must preserve these
measurements and gate semantics.
