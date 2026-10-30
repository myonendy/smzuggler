# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Token-based shared-shim syntax guard; policy and limitations: docs/style.md.
# Runs synchronously, borrows source text and owns token lists; no runtime APIs.
import argparse
import fnmatch
import re
import subprocess
import sys

from changed import changed

TOKEN = re.compile(r'''\s+|//[^\n]*|/\*[\s\S]*?\*/|'(?:\\[\s\S]|[^'\\])*'|"(?:\\[\s\S]|[^"\\])*"|[A-Za-z_$][\w$]*|\d+(?:\.\d+)?|=>|\.\.\.|===|!==|==|!=|[^\s]''')
REGEX = re.compile(r'/((?:\\.|\[(?:\\.|[^\]\\])*\]|[^/\n\\])*)/[a-z]*')


def tokens(source):
    """Yield (token, one-based line) pairs, discarding comments and literals.

    A preceding expression-start token distinguishes regex from division. This
    lightweight lexer is a syntax guard, not a complete JavaScript validator.
    Input is borrowed; errors in general JS syntax belong to runtime validation.
    """
    offset, previous, line = 0, '', 1
    while offset < len(source):
        match = TOKEN.match(source, offset)
        word = match.group()
        token_line = line
        if word == '/' and previous in ('', '=', '(', '[', ',', ':', 'return', '!', '&&', '||'):
            literal = REGEX.match(source, offset)
            if literal:
                line += literal.group().count('\n')
                offset = literal.end()
                previous = 'literal'
                yield 'literal', token_line
                continue
        line += word.count('\n')
        offset = match.end()
        if word.isspace() or word.startswith(('//', '/*')):
            continue
        if word.startswith(('"', "'")):
            word = 'literal'
        yield word, token_line
        previous = word


def check(source):
    """Yield line/reason pairs for forbidden syntax in borrowed source.

    Scan whole changed files so multiline constructs cannot escape the guard.
    No retained inputs or external resources; callers choose diagnostic policy.
    """
    items = list(tokens(source))
    words = [item[0] for item in items]
    for index, (word, line) in enumerate(items):
        before = words[index - 1] if index else ''
        after = words[index + 1] if index + 1 < len(words) else ''
        property_name = before == '.' or after == ':'
        if word in ('let', 'const', 'class') and not property_name:
            yield line, word + ' syntax'
        if word in ('=>', '`', '...'):
            yield line, 'arrow, template or spread/rest syntax'
        if word in ('var', 'let', 'const') and after in ('{', '['):
            yield line, 'destructuring declaration'
        if word == 'of' and not property_name:
            start = index - 1
            while start >= 0 and words[start] not in ('(', ';', '{', '}'):
                start -= 1
            if start > 0 and words[start - 1] == 'for':
                yield line, 'for-of loop'
        if word in ('{', '[') and before in ('', ';', '{', '}', '(', ',', '=', 'return'):
            closing = '}' if word == '{' else ']'
            depth, end = 1, index + 1
            while end < len(words) and depth:
                depth += (words[end] == word) - (words[end] == closing)
                end += 1
            if end < len(words) and words[end] == '=':
                yield line, 'destructuring assignment'
        if word == 'function':
            yield from parameters(items, index)


def parameters(items, index):
    """Reject modern function parameters; borrow tokens and return diagnostics.

    Track nested parentheses so defaults containing calls remain visible; the
    caller handles rest syntax globally. No allocations escape except results.
    """
    index += 1
    while index < len(items) and items[index][0] != '(':
        index += 1
    depth = 1
    index += 1
    while index < len(items) and depth:
        word, line = items[index]
        if depth == 1 and word in ('=', '{', '['):
            yield line, 'default or destructured parameter'
        depth += (word == '(') - (word == ')')
        index += 1


def main():
    """Return 1 for ES5 violations, 2 for Git/I/O errors; print file:line context."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', required=True)
    parser.add_argument('--paths', nargs='+', default=['src/shims/*.js'])
    args = parser.parse_args()
    failed = False
    try:
        for path, name, lines in changed(args.base, {'.js'}):
            if not any(fnmatch.fnmatchcase(name, pattern) for pattern in args.paths):
                continue
            for line, reason in check(path.read_text(encoding='utf-8')):
                print('%s:%d: %s' % (name, line, reason))
                failed = True
    except (OSError, UnicodeError, subprocess.CalledProcessError) as error:
        print('es5: tool failure: %s' % error, file=sys.stderr)
        return 2
    return int(failed)


if __name__ == '__main__':
    sys.exit(main())
