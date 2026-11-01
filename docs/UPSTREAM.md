<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Tracks assigned upstream patches and safe fork synchronization (§8).
This is the extraction allowlist for tools/upstream; paths are exact and
repository-relative. Keep fork-only work outside these offers.
-->
# Upstream contributions

Each row is one PR, except the issue-only renderer report. Status vocabulary:
`planned`, `ready to cut`, `opened`, `merged`, `rejected-carried`, `withdrawn`.
A dash means no link or commit has been recorded yet. Notes record review reasons,
upstream merge commits and smzuggler carrying commits as they become available.
The paths column includes the existing upstream tests permitted by DEC-SETUP-005;
no new test infrastructure or unrelated paths may enter an offer.

| PR | Change | Files (exact extraction allowlist) | Depends | Status | Branch | Upstream link | Notes (merge / carried commits) |
|---|---|---|---|---|---|---|---|
| U1 | Document tag lookup includes head/body/html (§5.1) | `src/shims/dom_shim.js`, `tests/test_dom_shim.c` | — | ready to cut | up/U1-document-tags | — | Singleton identity, case-insensitive tags and wildcard covered by QuickJS shim tests |
| U2 | Element subtree tag/id/class selectors (§5.1, §5.7) | `src/shims/dom_shim.js`, `tests/test_dom_shim.c` | — | ready to cut | up/U2-element-selectors | — | Tree-order tag/id/class, compounds, comma lists and syntax errors covered by QuickJS shim tests |
| U3 | Window EventEmitter and window methods (§5.2) | `src/shims/storage_shim.js`, `tests/test_storage.c` | — | planned | up/U3-window-events | — | 100–150 lines; shim unit test |
| U4 | SDL close event and forced quit (§5.2) | `src/platform/platform_sdl3.c`, `src/engine/game_loop.c`, `src/shims/storage_shim.js`, `tests/test_game_loop.c`, `tests/test_storage.c` | U3 | planned | up/U4-close-delivery | — | 50–120 lines; C and control-mode tests |
| U5 | Process environment, platform, versions, arch, timing and nextTick (§5.3) | `src/shims/storage_shim.js`, `src/bindings/bind_platform.c`, `tests/test_storage.c` | — | planned | up/U5-process | — | 80–150 lines; shim unit test |
| U6 | OS-consistent navigator identity (§5.3) | `src/shims/navigator_shim.js`, `tests/test_platform.c` | — | planned | up/U6-navigator | — | <50 lines; shim unit test; U5 helps |
| U7 | Refuse relative and game-root save writes (§5.3) | `src/bindings/bind_io.c`, `src/io/file_io.c`, `tests/test_file_io.c` | — | planned | up/U7-save-guard | — | 50–100 lines; C test |
| U8 | PIXI Circle/Ellipse/Polygon methods and type (§5.4) | `src/shims/pixi_shim.js`, `tests/test_pixi_shim.c` | — | planned | up/U8-shapes | — | 100–200 lines; shim unit test |
| U9 | Optional data-dir moves logs to state dir (§5.6) | `src/main.c`, `tests/test_file_io.c` | — | planned | up/U9-data-dir | — | 50–100 lines; C test |
| U10 | Data-dir moves jsc cache to cache dir (§5.6) | `src/engine/js_engine.c`, `tests/test_js_engine.c` | U9 | planned | up/U10-cache-dir | — | 50–100 lines; C test |
| U11 | canPlayType reflects build decoders (§5.7) | `src/shims/dom_shim.js`, `tests/test_dom_shim.c` | — | planned | up/U11-play-type | — | <30 lines; shim unit test |
| U12 | Pin SoLoud and SDL_shadercross (App. A) | `third_party/CMakeLists.txt` | — | ready to cut | up/U12-pin-soloud-shadercross | — | <20 lines; CI build |
| U13 | Canvas2D text fix if root cause is upstream (§5.5) | `src/rendering/canvas2d.c`, `src/rendering/font_manager.c`, `src/shims/canvas2d_shim.js`, `tests/test_canvas2d.c`, `tests/test_font.c` | — | planned | up/U13-text | — | Size unknown; golden test; confirm exact root cause before cutting |
| U14 | Optional missing-member PROF warnings (§5.4) | `src/shims/pixi_shim.js` | — | planned | up/U14-profile-warnings | — | <80 lines; optional |
| Issue only | UltraMode7 needs Geometry/Buffer/Shader/Mesh (§5.4) | — | — | planned | — | — | File missing-renderer-API report only; no implementation |

U2 queries walk connected document elements or an element's descendants in tree
order, excluding the element receiver. Results are array snapshots. The supported
selector subset is tags (including `*`), IDs, classes, compounds and comma lists;
identifiers use ASCII letters/underscore followed by letters, digits, underscore
or hyphen. Other syntax, including combinators, attributes and escapes, throws an
Error named `SyntaxError`. The head singleton is attached before body so document
queries use the same traversal as element queries.

## Building and opening offers (§8.2)

Upstream-first branches start at the current `upstream-main`, which mirrors
`upstream/master` (DEC-SETUP-001 corrects the spec's `main` spelling). Never base an
offer on smzuggler main. Forge U cards work on main under DEC-SETUP-005; the owner
extracts their ordered commits with:

```sh
tools/upstream/cut_up_branch.sh --build U1 document-tags COMMIT...
```

The helper reads this shipped tracker, rejects whole commits containing any extra
path or fork copyright, and replays with `cherry-pick -x` in a temporary detached
worktree. It publishes the local branch only after success, removes the worktree
on failure, preserves the caller's branch and dirty files, and never contacts a
remote. Existing branches are refused. `--build` configures QuickJS with tests and
fully disconnected FetchContent using `SMZ_DEPS_DIR` (default
`~/.cache/smzuggler-dev/deps`), builds and runs ctest. The default only extracts;
the owner must build/test with QuickJS before opening. No pushes occur.

Supply commits in dependency order. Every intermediate commit is checked, so
removing a forbidden path or header in a later commit cannot hide it in the
offer's history. A replay conflict leaves no destination branch; resolve the
candidate on the fork and retry with the corrected commit sequence.

Keep each offer independent, one concern, ideally under 200 changed lines, with
its own test and explanation. U4 waits for U3; U10 waits for U9. Use ES5 shims,
upstream C style and upstream headers, with no smzuggler copyright or V8/real-PIXI
assumptions. Run upstream's QuickJS CI before opening. The PR describes the public
failure, root cause with file/line, fix, validation and reference behaviour with
MDN/Node/NW.js links. Mention public plugin names only; never game files.

Use the project identity (§16). Copyright assignment to General Arcade is accepted
for these patches only, preserving upstream's dual licence. Q12 remains an owner
action: confirm whether upstream requires a PR statement, signed form or CLA bot,
and whether it accepts the project identity; make the assignment once in the
required form. Cutting and opening PRs are the owner's steps (DEC-SETUP-005).

Never offer V8/jsb, WebGL2/ANGLE, real PIXI, media, WebAudio semantics unless asked,
FreeType/HarfBuzz without a separate proposal, save migration, game scenarios,
privacy deny-lists or anything naming a game.

## Synchronizing the fork (§8.3)

Keep upstream-main a read-only, fast-forward-only mirror of upstream/master.
Fetch and fast-forward it during the owner's sync; never commit to it. Merge it
into main with a normal merge commit named `Merge upstream Outsider <short-sha>`.
Never rebase or force-push public main. Sync monthly, whenever an offer lands, and
before each milestone exit. Run the full suite, read-only check and §14 performance
smoke test. Record conflicts and resolutions below in the merge's documentation.

While pending, carry fixes on main using `git cherry-pick -x` from up/* and record
both commits in the tracker. At the next merge take upstream's reviewed version;
resolve duplicate or changed patches in the merge commit. If rejected, retain the
patch as rejected-carried, record upstream's reason and recheck it at every sync.

Keep fork code in src/jsb, src/webgl, src/media and src/smzuggler. Minimize edits to
upstream files and explain each block with `/* smzuggler: why */` comments.
For each binding sync:

- Inspect upstream bind_*.c changes for API, ownership and error handling changes.
- Reapply those changes through jsb by hand; preserve thread and lifetime contracts.
- Port js_engine.c changes to V8; QuickJS mode is not retained in smzuggler.
- Test the affected bindings and record the manual resolution in the merge commit.

## Sync history

No upstream syncs recorded yet. Each entry must record upstream SHA, merge commit,
carried/rejected patch review, conflict resolutions (especially bindings and V8),
and full-suite, read-only and performance results.
