#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Check source documentation without a compiler or Node dependency.

This repository tool implements the mechanical part of spec section 15.1;
reviewers still assess purpose, ownership and API contracts. It reads files only
and owns no runtime resources (see docs/doclint-baseline.md). Comments and strings
are masked before recognizing ordinary C/C++ signatures and JS function syntax.
It does not expand macros, resolve conditional compilation, parse C++ templates,
operators or function-pointer declarators, or interpret JavaScript regex literals.
ES class/object shorthand methods and block-bodied arrows are recognized
heuristically; expression-bodied arrows are outside coverage.
Comment presence is checked, not prose quality; plain block and line comments
are accepted. Changed mode compares BASE with the working tree, including staged
and untracked sources; any changed line in a function's full span selects it.
"""

import argparse
from pathlib import Path
import re
import subprocess
import sys

EXTENSIONS = {'.c', '.cc', '.cpp', '.h', '.js'}
LEXEMES = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'
                     r"'(?:\\.|[^'\\])*'|`(?:\\.|[^`\\])*`", re.S)
C_SIGNATURE = re.compile(
    r'^[ \t]*(?:(?:[\w:]+|\*|&)[ \t\n]+)+'
    r'[*& \t]*([\w:~]+)[ \t]*\(', re.M)
JS_SIGNATURE = re.compile(r'\bfunction\s*\*?\s*[\w$]*\s*\(')
JS_METHOD = re.compile(r'^[ \t]*(?:(?:async|static|get|set)\s+)*'
                       r'[\w$]+\s*\([^;{}]*?\)\s*\{', re.M)
JS_ARROW = re.compile(r'(?:\([^;{}]*?\)|[\w$]+)\s*=>\s*\{')
CONTROL = {'if', 'for', 'while', 'switch', 'catch', 'return', 'sizeof',
           'else', 'do', 'throw', 'new', 'delete'}


def masked(source):
    """Preserve offsets and newlines while hiding comments and string contents."""
    return LEXEMES.sub(lambda match: re.sub(r'[^\n]', ' ', match.group()), source)


def closing(text, start, left, right):
    """Return a balanced delimiter's offset, or None for incomplete source."""
    depth = 0
    for index in range(start, len(text)):
        if text[index] == left:
            depth += 1
        elif text[index] == right:
            depth -= 1
            if depth == 0:
                return index
    return None


def functions(source, suffix):
    """Yield half-open source spans for definitions and public header prototypes."""
    code = masked(source)
    patterns = [JS_SIGNATURE, JS_METHOD, JS_ARROW] if suffix == '.js' else [C_SIGNATURE]
    seen = set()
    for pattern in patterns:
        for match in pattern.finditer(code):
            start = match.start()
            prefix = code[start:match.end()]
            if any(word in CONTROL for word in re.findall(r'\b\w+\b', prefix)):
                continue
            paren = code.find('(', start, match.end())
            if paren < 0:  # Single-parameter arrow.
                tail = match.end() - 1
            else:
                end = closing(code, paren, '(', ')')
                if end is None:
                    continue
                tail = end + 1
            following = re.match(r'\s*(?:(?:const|noexcept|override|final)\s*)*'
                                 r'(?:=>\s*)?([{;])', code[tail:])
            if not following:
                continue
            brace = tail + following.end() - 1
            prototype = following[1] == ';'
            if prototype and (suffix != '.h' or re.search(r'\bstatic\b', prefix)):
                continue
            finish = brace if prototype else closing(code, brace, '{', '}')
            if finish is not None and brace not in seen:
                seen.add(brace)
                # Include the assignment preceding a JS function expression.
                line_start = source.rfind('\n', 0, start) + 1
                yield line_start, finish + 1


def documented(source, start):
    """Accept a nonempty comment immediately before the signature's first line."""
    before = source[:start].rstrip()
    if before.endswith('*/'):
        opening = before.rfind('/*')
        comment = before[opening + 2:-2] if opening >= 0 else ''
        return bool(comment.strip('* \t\n')) and 'SPDX-License-Identifier:' not in comment
    return bool(re.search(r'//\s*\S[^\n]*$', before))


def header_rules(source):
    """Require leading license metadata and purpose prose before any source code."""
    header = re.match(r'\s*(?:(?:/\*.*?\*/|//[^\n]*)(?:\s*))+', source, re.S)
    if not header:
        return ['header.missing']
    text = header.group()
    rules = []
    if not re.search(r'SPDX-License-Identifier:\s*\S+', text):
        rules.append('header.spdx')
    if not re.search(r'Copyright\s+(?:\([Cc]\)\s*)?\d{4}\s+\S', text):
        rules.append('header.copyright')
    prose = [line.strip(' /*\t\r') for line in text.splitlines()
             if not re.search(r'SPDX-License-Identifier:|Copyright|Modified', line)]
    if not any(re.search(r'[A-Za-z]{2,}', line) for line in prose):
        rules.append('header.purpose')
    return rules


def lint(path, changed=None):
    """Return line/rule findings; changed contains new-side lines or deletion boundaries."""
    source = path.read_text(encoding='utf-8')
    findings = [(1, rule) for rule in header_rules(source)]
    for start, end in functions(source, path.suffix):
        first = source.count('\n', 0, start) + 1
        last = source.count('\n', 0, end) + 1
        if changed is not None and not any(first <= line <= last for line in changed):
            continue
        if not documented(source, start):
            findings.append((first, 'function.doc'))
    return findings


def git(root, *args):
    """Read Git state without mutating it; propagate command failures to the CLI."""
    return subprocess.run(['git', '-C', str(root), *args], check=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout


def changed_files(base):
    """Map working-tree paths to changed new-side lines, including untracked files."""
    root = Path(git(Path.cwd(), 'rev-parse', '--show-toplevel').decode().strip())
    names = git(root, 'diff', '--name-only', '-z', '--diff-filter=ACMR', base, '--')
    untracked = git(root, 'ls-files', '--others', '--exclude-standard', '-z')
    paths = set(names.split(b'\0')) | set(untracked.split(b'\0'))
    result = {}
    for name in sorted(paths - {b''}):
        path = root / name.decode('utf-8')
        if path.suffix not in EXTENSIONS or not path.is_file():
            continue
        if name in untracked.split(b'\0'):
            result[path] = set(range(1, len(path.read_text().splitlines()) + 1))
            continue
        diff = git(root, 'diff', '--no-ext-diff', '--no-textconv', '--unified=0',
                   base, '--', str(path)).decode('utf-8')
        lines = set()
        for first, count in re.findall(r'^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@',
                                       diff, re.M):
            start, length = int(first), int(count) if count else 1
            # Half-line boundaries select enclosing bodies, not a preceding
            # function when an entire following definition was deleted.
            if length:
                lines.update(range(start, start + length))
            else:
                lines.add(start + 0.5)
        result[path] = lines
    return root, result


def main():
    """Emit stable file:line rule diagnostics; return 2 for input or Git errors."""
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--all', action='store_true')
    mode.add_argument('--changed', metavar='BASE')
    parser.add_argument('paths', nargs='*', help='--all roots (default: src)')
    args = parser.parse_args()
    if args.changed and args.paths:
        parser.error('paths are only supported with --all')
    try:
        root = Path.cwd()
        if args.changed:
            root, selected = changed_files(args.changed)
        else:
            selected = {}
            for name in args.paths or ['src']:
                path = Path(name)
                if not path.exists():
                    raise ValueError('source root does not exist: ' + name)
                files = path.rglob('*') if path.is_dir() else [path]
                selected.update({p: None for p in files if p.suffix in EXTENSIONS
                                 and p.is_file()})
        count = 0
        for path, lines in sorted(selected.items()):
            for line, rule in lint(path, lines):
                display = path.relative_to(root) if path.is_relative_to(root) else path
                print(f'{display}:{line}: {rule}')
                count += 1
        return int(count > 0)
    except (OSError, UnicodeError, ValueError, subprocess.CalledProcessError) as error:
        message = error.stderr.decode().strip() if isinstance(
            error, subprocess.CalledProcessError) else str(error)
        print('doclint: ' + message, file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
