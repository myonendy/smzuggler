#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
"""Report the shipped size of a smzuggler build and what each dependency costs.

Why: spec section 9 (M0) asks for a binary-size baseline before the V8 swap,
so that Q5 (ANGLE everywhere or not) and Q8 (ICU data size) are decided
against measured numbers rather than guesses. The tool reads one CMake build
directory and reports:
  * the stripped size of smzuggler-rt, of every shared library the build
    produced and the byte total of the shims that `cmake --install` ships;
  * a per-dependency breakdown (QuickJS, SDL3, SoLoud, Effekseer, shader
    tools) of the static archives, as ELF text/data/bss bytes.
Everything is parsed in pure Python, with no strip/ar/size binaries, so the
numbers are the same on every host. "Stripped" means the file size minus the
sections `strip --strip-all` removes (.symtab, .strtab, .debug*) and their
section headers; it can differ from real strip by alignment padding. Non-ELF
files (PE, Mach-O) are reported at their raw size and flagged as such.
Results are documented in docs/perf/binary-size.md (see docs/perf.md). Read
only: the build directory is never written. Exit 0 on success, 1 on a missing
or malformed build, 2 on usage errors.
"""

import argparse
import fnmatch
import json
import os
from pathlib import Path
import re
import struct
import sys

SHF_ALLOC, SHF_EXECINSTR = 0x2, 0x4
SHT_SYMTAB, SHT_NOBITS = 2, 8
STRIPPED_PREFIXES = ('.debug', '.zdebug', '.gnu.debuglto')
SHARED_PATTERNS = ('*.so', '*.so.*', '*.dylib', '*.dll')
ARCHIVE_PATTERNS = ('*.a', '*.lib')
# Archive base names (lower case, "lib" prefix and extension removed) per
# dependency. Order matters: SDL3_shadercross is a shader tool, not SDL3.
GROUPS = (
    ('shader tools', ('sdl3_shadercross*', '*shadercross*', 'glslang*',
                      'spirv*', 'machineindependent*', 'genericcodegen*',
                      'osdependent*', 'oglcompiler*', 'hlsl*')),
    ('QuickJS', ('qjs*', 'quickjs*')),
    ('SDL3', ('sdl3*',)),
    ('SoLoud', ('soloud*',)),
    ('Effekseer', ('effekseer*', 'llgi*')),
)
DEPENDENCIES = tuple(name for name, _ in GROUPS)


class BuildError(Exception):
    """A build directory or file that cannot be measured; carries the reason."""


def elf_sections(data):
    """Parse the section headers of an ELF image.

    Why: both the stripped-size estimate and the archive breakdown need each
    section's name, type, flags and size, and reading them directly keeps the
    tool free of host binutils. data is the whole file as bytes. Returns
    (sections, shentsize), where sections is a list of (name, type, flags,
    size) tuples and shentsize the bytes per section header, or None when data
    is not ELF. Raises BuildError on a truncated or inconsistent header.
    """
    if data[:4] != b'\x7fELF' or len(data) < 52:
        return None
    is64, order = data[4] == 2, '<' if data[5] == 1 else '>'
    try:
        if is64:
            shoff, = struct.unpack_from(order + 'Q', data, 0x28)
            shentsize, shnum, shstrndx = struct.unpack_from(order + 'HHH', data, 0x3A)
            layout = order + 'IIQQQQ'
        else:
            shoff, = struct.unpack_from(order + 'I', data, 0x20)
            shentsize, shnum, shstrndx = struct.unpack_from(order + 'HHH', data, 0x2E)
            layout = order + 'IIIIII'
        raw = [struct.unpack_from(layout, data, shoff + i * shentsize)
               for i in range(shnum)]
    except struct.error as error:
        raise BuildError(f'truncated ELF section table: {error}') from None
    if shstrndx >= len(raw) and shnum:
        raise BuildError('ELF section-name index out of range')
    names = raw[shstrndx][4] if shnum else 0
    sections = []
    for name_off, sh_type, flags, _addr, _offset, size in raw:
        end = data.find(b'\0', names + name_off)
        name = data[names + name_off:end].decode('ascii', 'replace')
        sections.append((name, sh_type, flags, size))
    return sections, shentsize


def is_stripped_away(name, sh_type, flags):
    """Tell whether `strip --strip-all` would drop this section.

    Why: the size that ships is the stripped one, and the debug and symbol
    sections are what strip removes. Allocated sections are always kept.
    """
    if flags & SHF_ALLOC:
        return False
    return (sh_type == SHT_SYMTAB or name in ('.symtab', '.strtab')
            or name.startswith(STRIPPED_PREFIXES))


def stripped_size(path):
    """Return (bytes, is_elf) for the file at path as it would ship stripped.

    Why: Release builds still carry .symtab/.strtab, which no package ships,
    so raw file sizes overstate the binary. Non-ELF files return their raw
    size with is_elf False so the report can flag them. Raises BuildError if
    the file cannot be read or is malformed.
    """
    name = Path(path).name
    try:
        data = Path(path).read_bytes()
    except OSError as error:
        raise BuildError(f'{name}: {error.strerror}') from None
    try:
        parsed = elf_sections(data)
    except BuildError as error:
        raise BuildError(f'{name}: {error}') from None
    if parsed is None:
        return len(data), False
    sections, shentsize = parsed
    removed = 0
    for name, sh_type, flags, size in sections:
        if is_stripped_away(name, sh_type, flags):
            removed += shentsize + (0 if sh_type == SHT_NOBITS else size)
    return len(data) - removed, True


def archive_members(data, label):
    """Yield (name, bytes) for every object member of a Unix ar archive.

    Why: the per-dependency breakdown has to look inside each static archive.
    Handles the GNU ("/", "//", "/N" long names) and BSD ("#1/N") variants and
    skips symbol-index members. label names the archive in error messages.
    Thin archives hold no object data and are rejected with BuildError.
    """
    if data[:8] == b'!<thin>\n':
        raise BuildError(f'{label}: thin archive has no member data')
    if data[:8] != b'!<arch>\n':
        raise BuildError(f'{label}: not an ar archive')
    pos, long_names = 8, b''
    while pos + 60 <= len(data):
        header = data[pos:pos + 60]
        if header[58:60] != b'`\n':
            raise BuildError(f'{label}: bad member header at byte {pos}')
        name = header[:16].decode('ascii', 'replace').rstrip()
        size = int(header[48:58].decode('ascii').strip() or 0)
        body = data[pos + 60:pos + 60 + size]
        pos += 60 + size + (size & 1)
        if name.startswith('#1/'):
            length = int(name[3:])
            name, body = body[:length].rstrip(b'\0').decode('utf-8', 'replace'), body[length:]
        elif name == '//':
            long_names = body
            continue
        elif re.fullmatch(r'/\d+', name):
            start = int(name[1:])
            name = long_names[start:long_names.find(b'/\n', start)].decode('utf-8', 'replace')
        if name in ('/', '/SYM64/', '__.SYMDEF', '__.SYMDEF SORTED'):
            continue
        yield name.rstrip('/'), body


def object_sizes(body, label):
    """Return {'text', 'data', 'bss', 'other'} byte counts for one member.

    Why: archive file sizes are dominated by symbols and relocations; the
    allocated sections are what the dependency adds to the linked binary
    (before the linker drops unused sections). text is executable, data the
    other allocated bits, bss the zero-filled sections. A non-ELF member
    counts its raw bytes under 'other'. Raises BuildError on malformed ELF.
    """
    sizes = {'text': 0, 'data': 0, 'bss': 0, 'other': 0}
    try:
        parsed = elf_sections(body)
    except BuildError as error:
        raise BuildError(f'{label}: {error}') from None
    if parsed is None:
        sizes['other'] = len(body)
        return sizes
    for _name, sh_type, flags, size in parsed[0]:
        if not flags & SHF_ALLOC:
            continue
        if sh_type == SHT_NOBITS:
            sizes['bss'] += size
        elif flags & SHF_EXECINSTR:
            sizes['text'] += size
        else:
            sizes['data'] += size
    return sizes


def group_of(archive):
    """Map an archive path to its dependency name, or its own base name.

    Why: one dependency builds several archives (glslang alone builds about
    six), and the report is per dependency. Matching is case-insensitive on
    the base name without "lib" prefix and extension.
    """
    base = Path(archive).name.lower()
    base = re.sub(r'\.(a|lib)$', '', base)
    base = base[3:] if base.startswith('lib') else base
    for name, patterns in GROUPS:
        if any(fnmatch.fnmatchcase(base, pattern) for pattern in patterns):
            return name
    return base


def find_files(build_dir, patterns):
    """Return the regular files under build_dir matching any glob pattern.

    Why: the build tree mixes shipped products with test-only ones; files
    under the tests/ subdirectory are skipped, and symlinks (libfoo.so ->
    libfoo.so.0) are skipped so each library is counted once. The list is
    sorted by path relative to build_dir so reports are stable.
    """
    found = []
    for root, dirs, files in os.walk(build_dir):
        rel_root = Path(root).relative_to(build_dir)
        if rel_root.parts[:1] == ('tests',):
            dirs[:] = []
            continue
        for name in files:
            path = Path(root) / name
            if path.is_symlink() or not path.is_file():
                continue
            if any(fnmatch.fnmatch(name, pattern) for pattern in patterns):
                found.append(path)
    return sorted(found, key=lambda p: p.relative_to(build_dir).as_posix())


def read_cache(build_dir):
    """Return the CMakeCache.txt entries of build_dir as {name: value}.

    Why: the executable name (SMZUGGLER_BIN), the build type and flags and the
    source directory (for the shims) are configuration, not fixed paths.
    Raises BuildError when the directory was never configured.
    """
    cache = Path(build_dir) / 'CMakeCache.txt'
    if not cache.is_file():
        raise BuildError('no CMakeCache.txt; not a CMake build dir')
    entries = {}
    for line in cache.read_text(errors='replace').splitlines():
        match = re.match(r'^([A-Za-z0-9_.+-]+):[A-Z]+=(.*)$', line)
        if match:
            entries[match.group(1)] = match.group(2)
    return entries


def shim_files(cache, install_dir):
    """Return (directory, files) for the shims a runtime install ships.

    Why: the shims are part of what a user downloads. With install_dir (the
    prefix of `cmake --install BUILD --component runtime`) the installed
    shims/ directory is measured; otherwise the source tree's src/shims, using
    the install rule's filter (*.js, recursive). Raises BuildError if absent.
    """
    if install_dir:
        directory = Path(install_dir) / 'shims'
    else:
        source = cache.get('CMAKE_HOME_DIRECTORY')
        if not source:
            raise BuildError('CMakeCache.txt has no CMAKE_HOME_DIRECTORY')
        directory = Path(source) / 'src' / 'shims'
    if not directory.is_dir():
        raise BuildError('shim directory does not exist '
                         '(src/shims, or shims/ under --install-dir)')
    files = sorted(p for p in directory.rglob('*.js') if p.is_file())
    return directory, files


def find_executable(build_dir, cache):
    """Return the path of the runtime executable in build_dir.

    Why: the binary is named by the SMZUGGLER_BIN cache entry (spec section
    13) and gets .exe on Windows. Raises BuildError when it was not built,
    because a report without the main binary would be misleading.
    """
    name = cache.get('SMZUGGLER_BIN', 'smzuggler-rt')
    for candidate in (name, name + '.exe'):
        path = Path(build_dir) / candidate
        if path.is_file():
            return path
    raise BuildError(f'executable {name} not found; build it first')


def archive_breakdown(build_dir):
    """Return {dependency: totals} over every static archive in build_dir.

    Why: this is the per-dependency view the M0 baseline asks for. Each
    totals dict holds 'archives' (relative paths), 'archive_bytes' (file
    sizes) and the summed object_sizes of all members. Dependencies listed in
    DEPENDENCIES but without an archive are absent from the result.
    """
    groups = {}
    for path in find_files(build_dir, ARCHIVE_PATTERNS):
        rel = path.relative_to(build_dir).as_posix()
        data = path.read_bytes()
        entry = groups.setdefault(group_of(path), {
            'archives': [], 'archive_bytes': 0,
            'text': 0, 'data': 0, 'bss': 0, 'other': 0})
        entry['archives'].append(rel)
        entry['archive_bytes'] += len(data)
        for member, body in archive_members(data, rel):
            for key, value in object_sizes(body, f'{rel}({member})').items():
                entry[key] += value
    return groups


def measure(build_dir, install_dir=None):
    """Measure build_dir and return the whole report as a JSON-ready dict.

    Why: one data structure feeds both the Markdown and the JSON output, so
    the two can never disagree. 'shipped_total' is the stripped executable
    plus stripped shared libraries plus shim bytes, all in bytes.
    """
    build_dir = Path(build_dir)
    cache = read_cache(build_dir)
    exe = find_executable(build_dir, cache)
    exe_bytes, exe_elf = stripped_size(exe)
    libraries = []
    for path in find_files(build_dir, SHARED_PATTERNS):
        size, elf = stripped_size(path)
        libraries.append({'path': path.relative_to(build_dir).as_posix(),
                          'stripped_bytes': size, 'elf': elf})
    shim_dir, shims = shim_files(cache, install_dir)
    shim_bytes = sum(p.stat().st_size for p in shims)
    lib_bytes = sum(lib['stripped_bytes'] for lib in libraries)
    return {
        'build_type': cache.get('CMAKE_BUILD_TYPE', ''),
        'c_flags': cache.get('CMAKE_C_FLAGS', ''),
        'cxx_flags': cache.get('CMAKE_CXX_FLAGS', ''),
        'executable': {'path': exe.name, 'stripped_bytes': exe_bytes,
                       'elf': exe_elf},
        'shared_libraries': libraries,
        'shared_libraries_bytes': lib_bytes,
        'shims': {'source': 'install' if install_dir else 'source tree',
                  'files': len(shims), 'bytes': shim_bytes},
        'shipped_total': exe_bytes + lib_bytes + shim_bytes,
        'dependencies': archive_breakdown(build_dir),
    }


def kib(value):
    """Format a byte count as KiB with one decimal, for human-read tables."""
    return f'{value / 1024:.1f}'


def summary_markdown(report):
    """Render the shipped-size half of the report as Markdown lines.

    Why: docs/perf/binary-size.md embeds this table; byte counts are kept
    next to KiB so later baselines can be diffed exactly.
    """
    exe = report['executable']
    lines = ['| Shipped part | Bytes | KiB |', '|---|---:|---:|']
    flag = '' if exe['elf'] else ' (not ELF, unstripped)'
    lines.append(f"| `{exe['path']}` stripped{flag} | {exe['stripped_bytes']} "
                 f"| {kib(exe['stripped_bytes'])} |")
    for lib in report['shared_libraries']:
        flag = '' if lib['elf'] else ' (not ELF, unstripped)'
        lines.append(f"| `{lib['path']}` stripped{flag} | "
                     f"{lib['stripped_bytes']} | {kib(lib['stripped_bytes'])} |")
    shims = report['shims']
    lines.append(f"| shims ({shims['files']} files, {shims['source']}) | "
                 f"{shims['bytes']} | {kib(shims['bytes'])} |")
    total = report['shipped_total']
    lines.append(f'| **total** | **{total}** | **{kib(total)}** |')
    return lines


def dependency_markdown(report):
    """Render the per-dependency archive breakdown as Markdown lines.

    Why: shows which dependency the binary's size comes from. The named
    dependencies are listed first, in DEPENDENCIES order, with "not built as
    a static archive" when absent (SDL3 is shared on Android, Effekseer is
    optional); other archives follow alphabetically.
    """
    deps = report['dependencies']
    lines = ['| Dependency | Archives | Archive bytes | text | data | bss | non-ELF |',
             '|---|---:|---:|---:|---:|---:|---:|']
    others = sorted(name for name in deps if name not in DEPENDENCIES)
    for name in DEPENDENCIES + tuple(others):
        entry = deps.get(name)
        if entry is None:
            lines.append(f'| {name} | 0 | not built as a static archive | | | | |')
            continue
        lines.append(f"| {name} | {len(entry['archives'])} | "
                     f"{entry['archive_bytes']} | {entry['text']} | "
                     f"{entry['data']} | {entry['bss']} | {entry['other']} |")
    return lines


def to_markdown(report):
    """Return the full report as one Markdown string (build flags first)."""
    lines = [f"Build type: `{report['build_type'] or '(none)'}`; "
             f"CMAKE_C_FLAGS: `{report['c_flags'] or '(none)'}`; "
             f"CMAKE_CXX_FLAGS: `{report['cxx_flags'] or '(none)'}`.", '']
    lines += summary_markdown(report) + [''] + dependency_markdown(report)
    return '\n'.join(lines) + '\n'


def main(argv=None):
    """Command-line entry point; returns the process exit status.

    Why: the tool is run by hand at each milestone to refresh the baseline.
    Prints Markdown (default) or JSON to stdout; errors go to stderr with
    the file that caused them.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('build_dir', help='CMake build directory to measure')
    parser.add_argument('--install-dir', help='prefix of a runtime install; '
                        'measure its shims/ instead of src/shims')
    parser.add_argument('--json', action='store_true', help='print JSON')
    args = parser.parse_args(argv)
    try:
        report = measure(args.build_dir, args.install_dir)
    except (BuildError, OSError) as error:
        print(f'binsize: {error}', file=sys.stderr)
        return 1
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        sys.stdout.write(to_markdown(report))
    return 0


if __name__ == '__main__':
    sys.exit(main())
