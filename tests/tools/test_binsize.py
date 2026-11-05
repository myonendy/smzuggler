# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Run tools/perf/binsize.py on a fake build dir of generated ELF objects.

Why: the tool's numbers become the M0 size baseline (docs/perf/binary-size.md),
so its stripping, archive parsing and grouping rules are checked here against
totals computed by hand. The objects are written by this file in pure Python,
so the test needs no compiler and runs the same on every CI host.
"""

import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

TOOL = Path(__file__).resolve().parents[2] / 'tools/perf/binsize.py'
PROGBITS, SYMTAB, STRTAB, NOBITS = 1, 2, 3, 8
ALLOC, EXEC = 0x2, 0x4
TEXT = ('.text', PROGBITS, ALLOC | EXEC)
DATA = ('.data', PROGBITS, ALLOC | 0x1)
BSS = ('.bss', NOBITS, ALLOC | 0x1)


def make_elf(sections):
    """Return a little-endian ELF64 image with the given (name, type, flags, size).

    Why: lets the test state section sizes exactly, so expected totals are
    plain arithmetic. A null section and .shstrtab are added as real files have.
    """
    names = b'\0' + b''.join(n.encode() + b'\0' for n, *_ in sections) + b'.shstrtab\0'
    body, headers, offset = b'', [struct.pack('<IIQQQQIIQQ', *[0] * 10)], 64
    name_off = 1
    for name, sh_type, flags, size in sections:
        blob = b'' if sh_type == NOBITS else b'\x90' * size
        headers.append(struct.pack('<IIQQQQIIQQ', name_off, sh_type, flags, 0,
                                   offset + len(body), size, 0, 0, 1, 0))
        body += blob
        name_off += len(name) + 1
    headers.append(struct.pack('<IIQQQQIIQQ', name_off, STRTAB, 0, 0,
                               offset + len(body), len(names), 0, 0, 1, 0))
    body += names
    shoff = offset + len(body)
    ehdr = (b'\x7fELF\x02\x01\x01' + b'\0' * 9 +
            struct.pack('<HHIQQQIHHHHHH', 1, 62, 1, 0, 0, shoff, 0, 64, 0, 0,
                        64, len(headers), len(headers) - 1))
    return ehdr + body + b''.join(headers)


def make_archive(members, bsd=False):
    """Return a Unix ar archive of (name, bytes); GNU long names unless bsd.

    Why: covers the "/" index, "//" long-name table and BSD "#1/N" variants
    the tool must read.
    """
    out = [b'!<arch>\n']

    def add(name, body):
        """Append one member header, its body and the even-byte pad."""
        out.append(f'{name:<16}{0:<12}{0:<6}{0:<6}{644:<8}{len(body):<10}`\n'.encode())
        out.append(body + (b'\n' if len(body) & 1 else b''))

    add('/', b'\0\0\0\0')
    table = b''.join(n.encode() + b'/\n' for n, _ in members if len(n) > 15)
    if table and not bsd:
        add('//', table)
    for name, body in members:
        if bsd:
            add(f'#1/{len(name)}', name.encode() + body)
        elif len(name) > 15:
            add(f'/{table.index(name.encode() + b"/")}', body)
        else:
            add(name + '/', body)
    return b''.join(out)


def write(path, data):
    """Create parent directories and write bytes or text to path."""
    path.parent.mkdir(parents=True, exist_ok=True)
    if isinstance(data, str):
        path.write_text(data)
    else:
        path.write_bytes(data)


def stripped(sections):
    """Return the expected stripped size of make_elf(sections)."""
    image = make_elf(sections)
    removed = sum(64 + size for name, _t, _f, size in sections
                  if name in ('.symtab', '.strtab') or name.startswith('.debug'))
    return len(image) - removed


def obj(*sections):
    """Return an ELF object image holding the given sections."""
    return make_elf(list(sections))


EXE = [TEXT + (1000,), DATA + (64,), ('.symtab', SYMTAB, 0, 240),
       ('.strtab', STRTAB, 0, 50), ('.debug_info', PROGBITS, 0, 500),
       ('.comment', PROGBITS, 0, 20)]
LIB = [TEXT + (300,), ('.symtab', SYMTAB, 0, 48)]
DLL = b'MZ' + b'\0' * 98


def build_tree(root):
    """Write a fake configured build dir and source tree under root."""
    src, build = root / 'src-tree', root / 'build'
    write(build / 'CMakeCache.txt',
          f'CMAKE_BUILD_TYPE:STRING=Release\nCMAKE_C_FLAGS:STRING=-Wfake\n'
          f'SMZUGGLER_BIN:STRING=smzuggler-rt\n'
          f'CMAKE_HOME_DIRECTORY:INTERNAL={src.as_posix()}\n')
    write(build / 'smzuggler-rt', make_elf(EXE))
    write(build / '_deps/foo-build/libfoo.so.1', make_elf(LIB))
    write(build / 'bin/bar.dll', DLL)
    write(build / 'tests/libhelper.so', make_elf(LIB))
    write(build / 'tests/libhelper.a', make_archive([('h.o', make_elf([TEXT + (9,)]))]))
    write(build / '_deps/qjs-build/libqjs.a', make_archive([
        ('qjs.c.o', obj(TEXT + (100,), DATA + (10,), BSS + (8,))),
        ('libregexp_long_name.c.o', obj(TEXT + (200,), ('.symtab', SYMTAB, 0, 99)))]))
    write(build / '_deps/sdl3-build/libSDL3.a', make_archive([('a.o', obj(TEXT + (50,)))]))
    write(build / '_deps/glslang-build/libglslang.a',
          make_archive([('g.o', obj(TEXT + (70,)))]))
    write(build / '_deps/spirv-build/libspirv-cross-core.a',
          make_archive([('s.o', obj(DATA + (30,)))]))
    write(build / '_deps/sc-build/libSDL3_shadercross.a',
          make_archive([('x.o', obj(TEXT + (5,)))]))
    write(build / '_deps/soloud-build/libsoloud.a', make_archive([
        ('soloud_core_long_name.o', obj(TEXT + (40,), BSS + (4,))),
        ('notes.txt', b'abc')], bsd=True))
    write(build / 'third_party/libzzz.a', make_archive([('z.o', obj(DATA + (3,)))]))
    write(src / 'src/shims/a.js', '0123456789')
    write(src / 'src/shims/sub/b.js', 'abcde')
    write(src / 'src/shims/README.md', 'not shipped')
    return build


class BinsizeTests(unittest.TestCase):
    """Check totals, grouping and failure modes on generated build dirs."""

    def run_tool(self, *args):
        """Run binsize.py with args and return the completed process."""
        return subprocess.run([sys.executable, str(TOOL), *map(str, args)],
                              capture_output=True, text=True)

    def setUp(self):
        """Create a fresh fake build tree for each test."""
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.build = build_tree(self.root)

    def tearDown(self):
        """Remove the temporary tree."""
        self.temp.cleanup()

    def report(self, *extra):
        """Run the tool in JSON mode and return the parsed report."""
        result = self.run_tool(self.build, '--json', *extra)
        self.assertEqual(result.returncode, 0, result.stderr)
        return json.loads(result.stdout)

    def test_shipped_totals(self):
        """Stripped executable, libraries and shims add up as computed by hand."""
        report = self.report()
        exe, lib = stripped(EXE), stripped(LIB)
        self.assertLess(exe, len(make_elf(EXE)))
        self.assertEqual(report['executable']['stripped_bytes'], exe)
        self.assertEqual(report['shared_libraries'], [
            {'path': '_deps/foo-build/libfoo.so.1', 'stripped_bytes': lib, 'elf': True},
            {'path': 'bin/bar.dll', 'stripped_bytes': len(DLL), 'elf': False}])
        self.assertEqual(report['shims'], {'source': 'source tree', 'files': 2, 'bytes': 15})
        self.assertEqual(report['shipped_total'], exe + lib + len(DLL) + 15)
        self.assertEqual(report['build_type'], 'Release')
        self.assertEqual(report['c_flags'], '-Wfake')

    def test_dependency_breakdown(self):
        """Archives are grouped per dependency with allocated-section totals."""
        deps = self.report()['dependencies']

        def pick(name):
            """Return the section totals of one dependency."""
            return {k: deps[name][k] for k in ('text', 'data', 'bss', 'other')}

        self.assertEqual(pick('QuickJS'), {'text': 300, 'data': 10, 'bss': 8, 'other': 0})
        self.assertEqual(pick('SDL3'), {'text': 50, 'data': 0, 'bss': 0, 'other': 0})
        self.assertEqual(pick('shader tools'), {'text': 75, 'data': 30, 'bss': 0, 'other': 0})
        self.assertEqual(len(deps['shader tools']['archives']), 3)
        self.assertEqual(pick('SoLoud'), {'text': 40, 'data': 0, 'bss': 4, 'other': 3})
        self.assertEqual(pick('zzz'), {'text': 0, 'data': 3, 'bss': 0, 'other': 0})
        self.assertNotIn('Effekseer', deps)
        self.assertNotIn('helper', deps)
        qjs = self.build / '_deps/qjs-build/libqjs.a'
        self.assertEqual(deps['QuickJS']['archive_bytes'], qjs.stat().st_size)

    def test_markdown_lists_flags_totals_and_missing_dependency(self):
        """The Markdown form names the flags, the total and absent dependencies."""
        result = self.run_tool(self.build)
        self.assertEqual(result.returncode, 0, result.stderr)
        total = stripped(EXE) + stripped(LIB) + len(DLL) + 15
        self.assertIn('Build type: `Release`; CMAKE_C_FLAGS: `-Wfake`', result.stdout)
        self.assertIn(f'| **total** | **{total}** |', result.stdout)
        self.assertIn('| Effekseer | 0 | not built as a static archive |', result.stdout)
        self.assertIn('(not ELF, unstripped)', result.stdout)

    def test_install_dir_shims(self):
        """With --install-dir the installed shims/ directory is measured."""
        write(self.root / 'prefix/shims/c.js', 'x' * 7)
        shims = self.report('--install-dir', self.root / 'prefix')['shims']
        self.assertEqual(shims, {'source': 'install', 'files': 1, 'bytes': 7})

    def test_missing_build_fails_loudly(self):
        """An unconfigured dir or an unbuilt executable exits 1 with the reason."""
        result = self.run_tool(self.root / 'src-tree')
        self.assertEqual(result.returncode, 1)
        self.assertIn('no CMakeCache.txt', result.stderr)
        (self.build / 'smzuggler-rt').unlink()
        result = self.run_tool(self.build)
        self.assertEqual(result.returncode, 1)
        self.assertIn('executable smzuggler-rt not found', result.stderr)

    def test_truncated_elf_fails_loudly(self):
        """A cut-off ELF file is reported by basename, not measured as raw."""
        write(self.build / 'smzuggler-rt', make_elf(EXE)[:200])
        result = self.run_tool(self.build)
        self.assertEqual(result.returncode, 1)
        self.assertIn('smzuggler-rt: truncated ELF section table', result.stderr)


if __name__ == '__main__':
    unittest.main()
