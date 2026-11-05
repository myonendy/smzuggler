<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-11-04: document the v8-mode code cache directory.
Host-owned directories for logs, the bytecode cache and runtime config.
The game folder is never written; see spec §5.3, §5.6 and §13.
-->

# Runtime data directories

smzuggler-rt never writes under the game folder. Host-owned files go to
directories named after the binary (`smzuggler-rt`): the XDG bases on Linux,
or `SDL_GetPrefPath("", "smzuggler-rt")` on Windows, macOS and Android.
Resolution lives in `src/smzuggler/data_dirs.c`. Startup calls `smz_host_dirs()`
so logs, the `.jsc` cache and the config directory are resolved together
before any of them is created. Empty or relative `XDG_*` values are treated
as unset, so nothing is created relative to the working directory. A path
that would land in the game folder is refused.

Upstream Outsider still writes logs next to the game unless `--data-dir` is
passed. smzuggler turns that option on by default
([DEC-M0-T021](DECISIONS.md#dec-m0-t021--default-host-data-directories-when-smzuggler_name-is-set)).

## State: `$XDG_STATE_HOME/smzuggler-rt/`

Persistent runtime state.

| Contents | Role |
| --- | --- |
| `debug.log` | Engine log (`error_handler`) |
| `console.log` | Captured stdout/stderr on Windows GUI launches |

Fallback when `XDG_STATE_HOME` is unset or empty: `$HOME/.local/state/smzuggler-rt/`.
`--data-dir=PATH` sends these two log files to an absolute PATH outside the
game folder instead.

**Clear:** delete the directory (or just the log files). The runtime recreates
it on the next launch that has a usable state path.

## Cache: `$XDG_CACHE_HOME/smzuggler-rt/`

Disposable data. QuickJS `.jsc` bytecode lives in the `jsc/` child, keyed by
each script's path relative to the game root (U10). Clearing it only costs a
recompile on the next load.

In v8 mode (`SMZ_JS_ENGINE=v8`) the `.jsc` files are replaced by V8 code caches in the
`v8cache/` child: `$XDG_CACHE_HOME/smzuggler-rt/v8cache/<sha256>.bin` (M1-T012, spec
§4.1 item 4 and §5.6). The name is the sha256 of the script source, V8's version
string and V8's flag hash (`ScriptCompiler::CachedDataVersionTag`), so an edited
script, a V8 upgrade or a flag change simply gets a new file. A file V8 rejects is
deleted and rewritten. Nothing is ever written next to a script. A directory passed
to `js_engine_set_cache_dir` replaces `v8cache/`; on platforms without XDG and with
no such directory, scripts run uncached. Old keys are not pruned yet: clearing the
directory is always safe.

Fallback: `$HOME/.cache/smzuggler-rt/`.

**Clear:** delete the directory. Safe at any time while the runtime is not
running.

## Config: `$XDG_CONFIG_HOME/smzuggler-rt/`

Runtime configuration. Spec §5.3 places `game_dir = <path>` in the `config`
file inside this directory. Startup always resolves this directory (and logs
the path) even though reading the file is a later card. `--data-dir` does
not move config.

Fallback: `$HOME/.config/smzuggler-rt/`.

**Clear:** delete the `config` file or the directory. `--game` still selects
the game folder.

## Windows, macOS and Android

All three kinds use `SDL_GetPrefPath("", "smzuggler-rt")` (typically
`%APPDATA%\smzuggler-rt\` or `~/Library/Application Support/smzuggler-rt/`).
Clear that folder to drop logs, cache and config together.

## Related

- [Architecture: game, save and runtime directories](../ARCHITECTURE.md#game-save-and-runtime-directories)
- [Privacy: runtime logs](privacy.md#runtime-logs-and-environment)
