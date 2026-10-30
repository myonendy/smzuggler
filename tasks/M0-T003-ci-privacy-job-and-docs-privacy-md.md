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
Add `.github/workflows/privacy.yml` running the §16 scan in CI: `tools/privacy/scan.py --tree HEAD` on every push, `--range <base>..<head>` on pull requests (identity, messages, added lines) and `--paths` on any uploaded report. The deny-list comes from the `PRIVACY_DENYLIST` secret written to a temp file outside the checkout; a missing secret fails the job (scan exit 2), it is never skipped. Write `docs/privacy.md`: what is scanned, the hook install (`tools/privacy/install-hooks.sh`), the deny-list location `~/.config/smzuggler-rt/privacy-denylist.txt` and format, why there are no overrides, how allowlist entries are reviewed, and the report-scrub rules of §14.7.

## Acceptance criteria
- [ ] `tests/tools/test_privacy_workflow.py` asserts: the job runs on push and pull_request; it writes the secret to a file under `$RUNNER_TEMP`, never echoes it, and passes it with `--denylist`; there is no `continue-on-error` and no `if:` that skips the scan when the secret is absent.
- [ ] `docs/privacy.md` covers every §16 rule (each rule has a heading) and the scan's output format (file and line, never the matched text).
- [ ] The tool's own tests (`tools/privacy/test_scan.py`) still pass unchanged.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.

## Notes
Spec §16, §14.7. `tools/privacy/**` is protected; this card documents and wires it, it does not change it. If the scan needs a change, stop and report it.
