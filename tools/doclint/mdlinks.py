#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-10-31: handle indented code, multiline links and heading entities.
"""Check relative links and #anchors in Markdown files, offline.

Why: spec section 15.7 makes a Markdown link check part of the docs gate, so a
renamed file or heading cannot silently break README, ARCHITECTURE, docs/ or a
module README. Anchors follow GitHub's heading slugs (github-slugger): lower
case, punctuation dropped, spaces become hyphens, repeats get -1, -2, ...
External links (any URL scheme, or //host) are never fetched. Fenced and
indented code blocks and inline code spans are ignored. Anchors are only
checked when the target is a Markdown file. See docs/style.md for gate usage.
With no paths it checks every git-tracked *.md file of the repository.
Reads files only; exit 0 when clean, 1 with file:line per broken link, 2 on
usage errors.
"""

import argparse
from html import unescape
from pathlib import Path
import re
import subprocess
import sys
from urllib.parse import unquote

FENCE = re.compile(r'^ {0,3}(`{3,}|~{3,})')
ATX = re.compile(r'^ {0,3}(#{1,6})(?:[ \t]+(.*?))?(?:[ \t]+#+)?[ \t]*$')
SETEXT = re.compile(r'^ {0,3}(=+|-+)[ \t]*$')
CODE_SPAN = re.compile(r'(`+)(.+?)\1')
INLINE_LINK = re.compile(r'!?\[(?:[^\[\]]|\[[^\[\]]*\])*\]\(')
REFERENCE = re.compile(r'^ {0,3}\[(?!\^)[^\]]+\]:[ \t]*<?([^\s>]+)>?')
HTML_REF = re.compile(r'<(?:a|img)\b[^>]*?\b(?:href|src)=["\']([^"\']*)["\']',
                      re.I)
HTML_ID = re.compile(r'<[a-z][^>]*?\b(?:id|name)=["\']([^"\']+)["\']', re.I)
SCHEME = re.compile(r'^[A-Za-z][A-Za-z0-9+.-]*:')


def code_lines(lines):
    """Return line indexes inside fenced or indented code blocks.

    Why: links and headings shown as examples in code must not be checked or
    turned into anchors. An unclosed fence runs to the end of the file, as on
    GitHub. Indentation uses four-column tab stops; indented code cannot
    interrupt a paragraph, so continuation lines remain available to links.
    """
    inside, opener, result = False, '', set()
    paragraph, indented = False, False
    for index, line in enumerate(lines):
        match = FENCE.match(line)
        if not inside and match:
            inside, opener = True, match.group(1)
            paragraph, indented = False, False
            result.add(index)
        elif inside:
            result.add(index)
            if (match and match.group(1)[0] == opener[0]
                    and len(match.group(1)) >= len(opener)
                    and not line.strip()[len(match.group(1)):].strip()):
                inside = False
        elif line.strip():
            expanded = line.expandtabs(4)
            if expanded.startswith('    ') and (indented or not paragraph):
                result.add(index)
                indented = True
            else:
                indented = False
                paragraph = not (ATX.match(line) or SETEXT.match(line))
        else:
            paragraph = False
    return result


def mask_code_spans(line):
    """Blank inline code spans so `[x](y)` shown as code is not checked."""
    return CODE_SPAN.sub(lambda m: ' ' * len(m.group()), line)


def heading_text(raw):
    """Return the rendered text of a heading, the input to GitHub's slugger.

    Why: GitHub slugs the rendered text, so link and image syntax collapse to
    their label, HTML tags vanish and emphasis markers are dropped, while code
    spans keep their content verbatim.
    """
    parts = re.split(r'(`+.+?`+)', raw)
    out = []
    for part in parts:
        if part.startswith('`'):
            out.append(part.strip('`').strip())
            continue
        part = re.sub(r'!?\[([^\]]*)\]\([^)]*\)', r'\1', part)
        part = re.sub(r'!?\[([^\]]*)\]\[[^\]]*\]', r'\1', part)
        part = re.sub(r'<[^>]+>', '', part)
        part = re.sub(r'\*+|~~', '', part)
        part = re.sub(r'(?<!\w)_+|_+(?!\w)', '', part)
        out.append(unescape(part))
    return ''.join(out).strip()


def slug(text):
    """Apply github-slugger's rule: lower case, keep word chars, - and space."""
    return re.sub(r'[^\w\- ]', '', text.lower()).replace(' ', '-')


class Slugger:
    """Assign unique slugs in document order, as GitHub does for repeats."""

    def __init__(self):
        """Start with no slugs seen."""
        self.occurrences = {}

    def add(self, text):
        """Return the unique slug for heading `text`; repeats get -1, -2, ..."""
        original = result = slug(text)
        while result in self.occurrences:
            self.occurrences[original] += 1
            result = '%s-%d' % (original, self.occurrences[original])
        self.occurrences[result] = 0
        return result


def anchors(text):
    """Return every fragment a Markdown document defines.

    Why: a link to file.md#x is valid if x is a heading slug or an explicit
    HTML id/name; both are collected so either form can be targeted.
    """
    lines = text.splitlines()
    code = code_lines(lines)
    slugger, result = Slugger(), set()
    for index, line in enumerate(lines):
        if index in code:
            continue
        result.update(HTML_ID.findall(line))
        match = ATX.match(line)
        if match:
            result.add(slugger.add(heading_text(match.group(2) or '')))
            continue
        prev = index - 1
        if (SETEXT.match(line) and prev >= 0 and prev not in code
                and lines[prev].strip() and not ATX.match(lines[prev])
                and not SETEXT.match(lines[prev])
                and not lines[prev].lstrip().startswith(('|', '-', '*', '>'))):
            result.add(slugger.add(heading_text(lines[prev].strip())))
    return result


def destination(line, start):
    """Parse an inline link destination beginning at offset `start`.

    Returns the destination string (without <> or title), or None when the
    destination has no terminator. Input may span lines: Markdown permits
    whitespace around a destination. Balanced parentheses inside the
    destination are kept, as CommonMark allows.
    """
    rest = line[start:].lstrip()
    if rest.startswith('<'):
        end = rest.find('>')
        return rest[1:end] if end >= 0 else None
    depth, out = 0, []
    for char in rest:
        if char.isspace() or (char == ')' and depth == 0):
            return ''.join(out)
        depth += {'(': 1, ')': -1}.get(char, 0)
        out.append(char)
    return None


def links(text):
    """Yield (line number, target) for each link outside code in `text`.

    Covers inline links and images, reference definitions, and HTML href/src
    attributes. Line numbers start at 1.
    """
    lines = text.splitlines()
    code = code_lines(lines)
    visible = '\n'.join(' ' * len(raw) if index in code else mask_code_spans(raw)
                        for index, raw in enumerate(lines))
    for match in INLINE_LINK.finditer(visible):
        target = destination(visible, match.end())
        if target is not None:
            yield visible.count('\n', 0, match.start()) + 1, target
    for index, raw in enumerate(lines):
        if index in code:
            continue
        line = mask_code_spans(raw)
        match = REFERENCE.match(line)
        if match:
            yield index + 1, match.group(1)
        for target in HTML_REF.findall(line):
            yield index + 1, target


def check_target(source, target, root, cache):
    """Return an error message for a broken `target` in `source`, else None.

    `root` is the repository root, used for /-rooted links as GitHub does.
    `cache` maps resolved Markdown paths to their anchor sets and is filled
    lazily so each file is parsed once per run.
    """
    if not target or SCHEME.match(target) or target.startswith('//'):
        return None
    path_part, _, fragment = target.partition('#')
    path_part = unquote(path_part.split('?', 1)[0])
    if not path_part:
        resolved = source
    elif path_part.startswith('/'):
        resolved = root / path_part.lstrip('/')
    else:
        resolved = source.parent / path_part
    if not resolved.exists():
        return 'missing file'
    if not fragment or resolved.is_dir() or resolved.suffix.lower() != '.md':
        return None
    key = resolved.resolve()
    if key not in cache:
        cache[key] = anchors(resolved.read_text(encoding='utf-8'))
    if unquote(fragment) not in cache[key]:
        return 'missing anchor'
    return None


def tracked_markdown(root):
    """List git-tracked *.md files under `root`; exit 2 loudly outside git.

    Files deleted in the working tree but still in the index are skipped,
    because links into them are already reported as missing.
    """
    result = subprocess.run(['git', '-C', str(root), 'ls-files', '-z', '--',
                             '*.md'], capture_output=True, text=True)
    if result.returncode != 0:
        print('mdlinks: git ls-files failed in %s: %s'
              % (root, result.stderr.strip()), file=sys.stderr)
        sys.exit(2)
    paths = [root / name for name in result.stdout.split('\0') if name]
    return [path for path in paths if path.is_file()]


def check_files(files, root):
    """Return 'file:line: message' strings for every broken link in `files`."""
    cache, findings = {}, []
    for path in files:
        text = path.read_text(encoding='utf-8')
        for number, target in links(text):
            error = check_target(path, target, root, cache)
            if error:
                shown = path.relative_to(root) if path.is_relative_to(root) else path
                findings.append('%s:%d: %s: %s' % (shown, number, error, target))
    return findings


def main(argv=None):
    """Parse arguments, check the files and print findings; return exit code."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--root', type=Path,
                        help='repository root (default: git top level of cwd)')
    parser.add_argument('paths', nargs='*', type=Path,
                        help='Markdown files to check (default: all tracked)')
    args = parser.parse_args(argv)
    root = args.root
    if root is None:
        top = subprocess.run(['git', 'rev-parse', '--show-toplevel'],
                             capture_output=True, text=True)
        root = Path(top.stdout.strip()) if top.returncode == 0 else Path.cwd()
    root = root.resolve()
    files = [p.resolve() for p in args.paths] or tracked_markdown(root)
    missing = [str(p) for p in files if not p.is_file()]
    if missing:
        print('mdlinks: no such file: ' + ', '.join(missing), file=sys.stderr)
        return 2
    findings = check_files(files, root)
    for finding in findings:
        print(finding)
    return 1 if findings else 0


if __name__ == '__main__':
    sys.exit(main())
