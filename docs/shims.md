<!-- SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Shim inventory and ordering contract for the loader and diagnostic walker.
Keep this order aligned with src/engine/script_loader.c; no game is required.
-->
# Shim load order and surface inventory

Run `smzuggler-rt --dump-surface` to write the installed JS surface to stdout,
or `smzuggler-rt --dump-surface=surface.tsv` to write a file. `--shims DIR`
selects an alternative shim directory. The build stages default shims under
`build/shims`, so the diagnostic needs no source-tree reads at runtime.
The engine starts without a game, SDL initialization, a window or audio.
The ordinary native file binding is rooted in the shim directory during the
dump, then the prior root is restored. This keeps storage initialization reads
inside the staged build tree. Host bindings form part of the baseline.
Only an explicitly selected output file is written; no bytecode cache is used.

Each sorted line is `name<TAB>kind<TAB>shim file`. Kind is JavaScript `typeof`
(or `accessor` for a getter/setter descriptor). The walker compares own property
snapshots against the bare engine, then after each shim. Added and replaced
members are attributed to the last file that installs them. Removed members
are omitted. Constructor properties, prototypes and nested namespaces are
walked, including non-enumerable properties; getters are never invoked.
Global aliases and ancestor references are listed but not recursively expanded,
preventing cycles. Values and function bodies are never printed.

The dump lists API members only, as spec §15.4 defines the surface ("every
global and prototype member the shims install"; DEC-M0-T009b). Two kinds of row
are left out because they are not APIs:

- a function's intrinsic `length`, `name` and `prototype` properties. Members
  of `X.prototype` are still walked and keep their `X.prototype.member` name;
  only the bare `X.prototype` row is dropped.
- the members of DOM element instances reached from `document` (an object with
  an own string `tagName`), such as `document.body.*`, `document.head.*` and
  `document.documentElement.*`. They repeat `HTMLElement.prototype`.
  `document.body`, `document.head` and `document.documentElement` keep one row
  each.

Everything a shim installs on a global, constructor, prototype or namespace is
listed, `_`-prefixed members included. The runtime dump measured with
`./build/smzuggler-rt --dump-surface | wc -l` on 2026-10-31 has 2,003 lines,
down from 6,124. `tests/smz/test_dump_surface_members.c` pins both exclusions.

The pre-core list is defined in
[src/engine/script_loader.c:245](../src/engine/script_loader.c#L245), and evaluated
by [script_loader_load_shims:260](../src/engine/script_loader.c#L260):

1. `dom_shim.js`
2. `canvas2d_shim.js`
3. `font_shim.js`
4. `navigator_shim.js`
5. `xhr_shim.js`
6. `storage_shim.js`
7. `pixi_shim.js`
8. `webaudio_shim.js`
9. `effekseer_shim.js`
10. `dom_overlay.js`
11. `plugin_compat.js`

The post-core list is defined at
[script_loader.c:293](../src/engine/script_loader.c#L293): `tilemap_shim.js`.
The dump uses [script_loader_shim_files:298](../src/engine/script_loader.c#L298)
to obtain both lists in that order. With no game classes installed, conditional
post-core patches naturally do nothing.

Normal game startup interleaves the pre-core list, libraries, core scripts,
enabled plugins, post-core list, and `main.js`, then starts the game; see
[script_loader_load_all:318](../src/engine/script_loader.c#L318) and the
[post-core phase:470](../src/engine/script_loader.c#L470).
`control_shim.js` is an optional automation script loaded by the runtime,
outside the loader's shim sequence, so it is outside this inventory.

`tests/smz/test_dump_surface.c` checks representative DOM, PIXI, NW.js, Node
and navigator members, row structure, strict sorting and identical successive
in-process runs without device bindings. This diagnostic is the runtime input
for the compatibility table described in spec §15.4.
