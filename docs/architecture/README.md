<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
-->
# Per-area architecture notes

One short file per module area (`combat.md`, `content.md`, `ui.md`, ...), at most ~40 lines:
what the area owns, what it may depend on, and its non-obvious contracts. forge puts the
note whose name matches a card's `owns` folder into that card's context pack, so agents
never have to open the whole `docs/ARCHITECTURE.md` (AGENTS.md section 5).
