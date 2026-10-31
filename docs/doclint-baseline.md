<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Record the legacy documentation debt and the linter's operating contract.
This tool supports spec section 15.1 without adding compiler dependencies.
-->

# Documentation lint baseline

On 2026-10-31, `python3 tools/doclint/doclint.py --all` scanned `src/` in
0.867 seconds and reported **1,586 findings**: 72 `header.purpose` and 1,514
`function.doc`. Exit status 1 is expected for this legacy baseline. Milestone D
will reduce this debt; this count is a record, not an exemption or threshold.

`python3 tools/doclint/doclint.py --changed upstream-main` passes on the
M0-T006 tree. This mode checks headers in changed C/C++/JS files and comments
only for functions whose signatures or bodies intersect changed lines. It
compares the base directly with the working tree, includes staged and untracked
sources, and ignores deleted files. Deletions select their position between surviving
lines so removing code within a body still requires its documentation, without
selecting an untouched function beside a fully deleted definition.

Use `--all [paths ...]` to scan explicit files or directories; the default is
`src/`. Recognized extensions are `.c`, `.cc`, `.cpp`, `.h`, and `.js`.
Diagnostics have the form `file:line: rule`; rules are `header.missing`,
`header.spdx`, `header.copyright`, `header.purpose`, and `function.doc`.
Exit codes are 0 for clean input, 1 for findings, and 2 for invocation, Git,
encoding or filesystem errors. No files are modified and no dependencies are
needed beyond Python and Git (Git is needed only for changed mode).

The parser masks comments and string literals and balances parentheses/braces.
It recognizes ordinary C/C++ definitions, non-static header prototypes, JS
function declarations and expressions (including prototype assignments), and
common class/object methods and block-bodied arrows. Plain block and line
comments qualify, but a licensing block alone does not document a function.
Separate C/C++ definitions on the same line each require their own comment;
documentation preceding the first definition does not cover later definitions.
Headers need license metadata and a separate nonempty prose line. Human review
must assess the paragraph's meaning and the function's contract; this tool
cannot establish that text explains ownership, threading or API deviations.

As detailed in the module docstring, macro-generated functions, complicated
C++ declarators/templates/operators and JavaScript regular expression literals
are outside this heuristic parser's guarantees. Expression-bodied arrows are
also outside its coverage. Preprocessor branches are scanned as written.
There is no compiler or Node dependency. The human-owned gate is added after
merge; this card does not change CI or gate infrastructure.
