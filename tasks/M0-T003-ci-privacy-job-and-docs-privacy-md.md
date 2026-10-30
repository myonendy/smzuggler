+++
id = "M0-T003"
title = "CI privacy job and docs/privacy.md"
milestone = 0
deps = []
agent = "any"
owns = [".github/workflows/privacy.yml", "tests/tools/test_privacy_workflow.py"]
docs = ["docs/privacy.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `.github/workflows/privacy.yml` running the §16 scan in CI: `tools/privacy/scan.py --tree HEAD` on every push, `--range <base>..<head>` on pull requests (identity, messages, added lines) and `--paths` on any uploaded report. The deny-list comes from the `PRIVACY_DENYLIST` secret written to a temp file outside the checkout; a missing secret fails the job (scan exit 2), it is never skipped. Write `docs/privacy.md`: what is scanned, the hook install (`tools/privacy/install-hooks.sh`), the deny-list location `~/.config/smzuggler-rt/privacy-denylist.txt` and format, why there are no overrides, how allowlist entries are reviewed, and the report-scrub rules of §14.7. The same workflow runs a commit-subject job: `tools/commitmsg/check_subject.py --range <base>..<head>` on pull requests and `--range <before>..<after>` on pushes (CONTRIBUTING.md, "Commit messages").

## Acceptance criteria
- [ ] `tests/tools/test_privacy_workflow.py` asserts: the job runs on push and pull_request; it writes the secret to a file under `$RUNNER_TEMP`, never echoes it, and passes it with `--denylist`; there is no `continue-on-error` and no `if:` that skips the scan when the secret is absent.
- [ ] The same test asserts a commit-subject job runs `tools/commitmsg/check_subject.py --range` over the pushed or PR commits with full history (`fetch-depth: 0`) and no `continue-on-error`.
- [ ] `docs/privacy.md` covers every §16 rule (each rule has a heading) and the scan's output format (file and line, never the matched text).
- [ ] The tools' own tests (`tools/privacy/test_scan.py`, `tools/commitmsg/test_check_subject.py`) still pass unchanged.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §16, §14.7. `tools/privacy/**` and `tools/commitmsg/**` are protected; this card documents and wires them, it does not change them. If the scan needs a change, stop and report it.
