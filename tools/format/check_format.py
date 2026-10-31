# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Check changed C/C++ lines without rewriting sources; see docs/style.md.
# Invokes Git and clang-format synchronously and owns only temporary byte strings.
import argparse
import subprocess
import sys
import xml.etree.ElementTree as ET

from changed import changed


def violations(path, lines):
    """Return changed lines touched by replacements; borrow path and line set.

    clang-format may expand ranges to syntax boundaries, so filter its byte edits
    against the original new-side lines. Tool or XML failures propagate to main.
    """
    data = path.read_bytes()
    command = ['clang-format', '--output-replacements-xml']
    for number in sorted(lines):
        command.append('--lines=%d:%d' % (number, number))
    command.append(str(path))
    result = subprocess.check_output(command, stderr=subprocess.PIPE)
    bad = set()
    for edit in ET.fromstring(result).findall('replacement'):
        offset, length = int(edit.attrib['offset']), int(edit.attrib['length'])
        first = data[:offset].count(b'\n') + 1
        last = data[:offset + max(length - 1, 0)].count(b'\n') + 1
        bad.update(lines.intersection(range(first, last + 1)))
    return sorted(bad)


def main():
    """Report file:line diagnostics; return 1 for style and 2 for tool failures."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', required=True)
    args = parser.parse_args()
    failed = False
    try:
        for path, name, lines in changed(args.base, {'.c', '.h', '.cc', '.cpp', '.hpp', '.cxx', '.hh'}):
            for number in violations(path, lines):
                print('%s:%d: clang-format differs' % (name, number))
                failed = True
    except (OSError, subprocess.CalledProcessError, ET.ParseError) as error:
        print('format: tool failure: %s' % error, file=sys.stderr)
        return 2
    return int(failed)


if __name__ == '__main__':
    sys.exit(main())
