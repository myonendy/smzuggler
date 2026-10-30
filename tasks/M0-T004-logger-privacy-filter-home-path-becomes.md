+++
id = "M0-T004"
title = "Logger privacy filter: home path becomes ~, never log user or host"
milestone = 0
deps = ["M0-T003"]
agent = "any"
owns = ["src/smzuggler/log_scrub.c", "src/smzuggler/log_scrub.h", "src/engine/error_handler.c", "src/engine/error_handler.h", "CMakeLists.txt", "tests/smz/test_log_scrub.c", "tests/smz/log_scrub.cmake"]
docs = ["docs/privacy.md", "CHANGELOG.md"]
tests_required = true
may_edit_tests = false
+++
## Goal
Add `src/smzuggler/log_scrub.c/.h`: `log_scrub(const char *in, char *out, size_t n)` replaces every occurrence of the user's home directory (from `HOME`, or `USERPROFILE` on Windows, compared as a path prefix, also with the other slash style) with `~`. Route every line written by `error_handler_log` (file and console) through it. The logger never writes the user name or host name: remove or scrub any existing log line that does.

## Acceptance criteria
- [ ] New unit test `tests/smz/test_log_scrub.c` (registered via `tests/smz/log_scrub.cmake`) sets HOME to a fresh temp directory (never a literal home path in the test source), logs messages containing that path several times, inside a longer path and as a bare prefix, and asserts the log file contains `~/…` and never the fake home path.
- [ ] The test also asserts that a path that merely starts with the same characters (the temp dir name with an extra character appended) is not rewritten.
- [ ] `git grep -nE 'gethostname|getlogin|getpwuid|GetUserName|GetComputerName' -- src` is empty, or each hit is shown (by a test) not to reach a log line.
- [ ] Existing `error_handler` tests pass unchanged.
- [ ] Every new or changed file has the §12 header and every new or changed function has a §15.1 doc comment.
- [ ] Commit subject describes the change per CONTRIBUTING.md (proposed in the final message's `COMMIT SUBJECT:` line; `tools/commitmsg/check_subject.py` accepts it).

## Notes
Spec §16 (logger rule). Keep the function short and free of allocation in the log path.
