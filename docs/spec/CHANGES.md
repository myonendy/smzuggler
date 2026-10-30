# smzuggler spec: change log

The project was called "Smuggler" in v0.4 and renamed **smzuggler** in v0.5. The v0.3→v0.4 notes below keep the old name, because they describe that version.

## What changed from v0.5 to v0.5.1

1. **Gate G0 dropped (Q20 closed).** The owner does not care how stock NW.js performs. The stock-NW.js measurement on battery is removed from M0, §9 and §14.2; an NW.js comparison stays possible as an unscheduled, unbudgeted debugging aid that never changes the gate. The laptop's battery/`power-saver` 60 fps gate stands on its own.
2. **Laptop gate operative; Deck is a confirmation pass.** Until a friend's Deck is available, passing on dev-laptop on battery with `power-saver` is taken to mean the Deck passes too (a stated assumption, §14). M2 and M6 sign off on the laptop gate. The Deck run is a confirmation pass, welcome at any milestone and **required before release (M7)**. Report `gate` values gain `confirm-pass`/`confirm-fail`. `deck-proxy` (§14.9) is now optional and never signs anything off. A new §9 note names the risk this carries (Deck-only problems surfacing late, covered by the M7 tuning reserve). Q19 stays open for timing only.
3. **Fixture saves defined (Q14 answered).** New §14.10 lists the three fixtures the perf suite needs: `fx-map92` (Map 92, the start map, for S2/S3/S8/S9), `fx-trade` (a port, Map 92 or 93, next to the store door, event 45 → Map 80, for S4) and `fx-combat` (Map 55 Atlantic Ocean next to an encounter event calling CE400, for S5). It records the owner's chosen slots on dev-laptop (`fx-map92` = `file12`, `fx-trade` = `file4`, `fx-combat` = `file6`, copied read-only next to a checksummed full backup of the Proton save folder), the pre-M0 test setup (a separate test copy of the game under `~` and an isolated runtime save folder, reached through the launch helper's `XDG_DATA_HOME` override, with `--fixture` and `--dry-run`), how fixtures are imported (`--import-fixture`, read-only), and that fixture saves are **never committed or shared**: testers make their own from a word-only recipe. The §16 scan gains a rule rejecting `*.rmmzsave`/`*.rpgsave` files, and the tester kit no longer mentions handing saves over.
4. **Estimate: 38–71.75 → 37.75–71.5 pw** (still ≈ 38–72; −0.25 for G0). M0 is 2–3.5; M0+M1 ~8–14 pw; M0–M2 ~17.75–31 pw. Calendar unchanged (~9–17 months to M7).
5. **Archive:** v0.5 is kept as `V8_MZ_HOST_SPEC_v0.5.md`.

## What changed from v0.4 to v0.5

1. **Renamed to smzuggler** (a pun on MZ), written in lower case. Repository `smzuggler`, binary `smzuggler-rt`, data directories `…/smzuggler-rt/`, source directory `src/smzuggler/`, copyright line "The smzuggler contributors". Every project-sense "Smuggler" is replaced; "the game" still means HIROTONFA's *American Revolution Smuggler*, and its own names (such as the save folder `HIROTONFA_AmericanRevolutionSmuggler`) are untouched. The non-affiliation note stays. The v0.4 residual-risk note (project name = last word of the game's title) is replaced by a note that the new name is an invented word. Q17 is closed.

2. **Engine re-check: the game is MZ, not MV.** New §18.1 lists the evidence from the game folder (read-only): `rmmz_*.js` core files and no `rpg_*.js`; `rmmz_core.js` "v1.10.0", `Utils.RPGMAKER_NAME = "MZ"`, `RPGMAKER_VERSION = "1.10.0"`; PIXI 5.3.12; `package.json` `rmmz-HARS`; NW.js 0.80.0; MZ-only `System.json` `advanced` block; no `www/` folder; `.rmmzsave` saves.

3. **New §18 "MV support (later, not in the main estimate)".** What extending smzuggler to RPG Maker MV games would take: per-game PIXI 4.5.4 (mostly free, since the game's own PIXI is loaded) with a WebGL1 context and PIXI 4 quirks; the `rpg_*.js` core and `index.html` script loading; the NW.js 0.12–0.29 surface (`require('nw.gui')`, old `fs`, `process.mainModule`); WebAudio vs `Html5Audio`; `<video>` element video; `www/save/*.rpgsave` LZString saves, **redirected** out of the game folder (MV even creates a `testDirectory/` there to probe); the `www/` layout; the `requestAnimationFrame` + fixed 1/60 accumulator loop; `@font-face` fonts; the `Graphics` DOM elements and the Canvas fallback; encrypted assets; DOM-poking plugins such as Yanfly's. Every row is tagged verified (game folder, or official corescript file and line) or assumed. Estimate **≈ 10–20 pw**, as milestone MV (MV-0 to MV-2) after M7, outside the total. No MV game is installed locally, so new Q18 asks for a test corpus.

4. **No Steam integration for now.** No Steamworks, no Steam APIs, no Steam client needed. The game folder is found by `--game <path>` or `game_dir` in `$XDG_CONFIG_HOME/smzuggler-rt/config`, never through Steam. The Proton save import is now a plain path hint derived from the game path (no library-list parsing). M7 drops the "Steam launch option" item for a doc note on adding a non-Steam game. §12 distribution is reworded. Steam Deck input works through Steam Input or SDL's own Deck driver. **Q13 closed** (one-time save copy is fine) and **v0.2 Q8 closed** ("no Steam integration for now").

5. **Steam Deck gate without an owned Deck (Q16 closed).** The Deck stays must-pass. New §14.8: the gate runs on a borrowed Deck or with an outside tester, recorded as a **Deck-equivalent** (model, SteamOS version, commit; no names or serials), with step-by-step tester instructions, a `--perf-suite=deck` runner, and `--scrub-report`, which keeps only the fixed report schema and refuses to pack if it finds the local user or host name. New §14.9: an interim **`deck-proxy`** on dev-laptop (4 P-cores via `taskset`, CPU capped at ~2.8 GHz, Iris Xe capped at ~1250 MHz, 1280×800 nested gamescope), labelled `gate: proxy`, **never a pass**, to be calibrated against the first real Deck report. New Q19 asks who lends or tests, and when.

6. **dev-laptop gate on battery with `power-saver` (Q15 closed).** That is now the must-pass state; AC with `balanced` is an extra report. §14.1 records the measured power-saver state (EPP `power` on all CPUs, turbo still enabled, 4.6/3.4 GHz maxima, PL1 30 W / PL2 64 W, GPU not capped). §14.2 is re-assessed honestly: a micro-benchmark on the laptop in this state showed **+42% mean, +48% p99 and ~2× worst case** for frame-sized bursts run at 60 Hz versus back to back; the laptop is now about as tight as the Deck at p99, and 60 fps there is "plausible, but no longer likely by default". smzuggler may reduce per-frame variance but must never change the power profile or busy-wait. New **Gate G0** in M0 measures stock NW.js in that power state first; new Q20 asks what happens if NW.js itself misses.

7. **Estimate: 36–69 → 38–72 pw** (+1.75–3.25): M0 +0.25 (G0), M2 +0.25–0.5 (tester kit, proxy), M7 +1.25–2.5 (larger tuning reserve on battery, release-ready tester kit). Calendar ~9–17 months to M7; M0+M1 ~8.25–14.25 pw; M0–M2 ~18–31.25 pw. MV (≈ 10–20 pw) and Android (+4–8 pw) stay outside the total.

8. **Open questions:** closed Q13, Q15, Q16, Q17 and v0.2 Q8; new Q18 (MV corpus), Q19 (Deck access) and Q20 (laptop target if G0 fails); Q14 gains a question about handing fixture saves to a tester.

9. **Archive:** v0.4 is kept as `V8_MZ_HOST_SPEC_v0.4.md`.

## What changed from v0.3 to v0.4

1. **The project is named Smuggler.** The spec is retitled, and "the fork" is now Smuggler throughout. "The game" always means HIROTONFA's game. A new naming section (§13) says Smuggler is not affiliated with HIROTONFA, and that names, icons and store text must not imply the game's trademark. It suggests the repo `smuggler-mz-runtime` and the binary `smuggler-rt`. The game's save folder name is read at run time, not hard-coded.

2. **General fixes go upstream as code.** Copyright assignment is accepted for those patches only. §8 is rewritten:
   * 14 small upstream PRs (U1–U14), each with its files, size, test and dependencies;
   * one issue (UltraMode7 mesh support) instead of code;
   * a "never upstream" list;
   * rules for building each PR: from upstream `main`, small, independent, ES5, tested on QuickJS, with the why explained;
   * a tracking policy: a mirror branch, merge (never rebase) monthly and before each milestone, carry pending fixes as cherry-picks, and log everything in `docs/UPSTREAM.md`.

3. **Real, unmodified PIXI 5.3.12 is final.** The "cheaper shim alternative" is removed, and the old Q2 is closed.

4. **60 fps on Steam Deck and dev-laptop is must-pass.** A new §14 gives:
   * both target profiles, with the owner's laptop anonymised as "dev-laptop";
   * whether 60 fps is plausible, and the risks;
   * exactly what is measured (frame work time, present interval, missed vblanks);
   * how a run is done: 10 s warm-up, 120 s scored, worst of 3 runs, and a 30-minute soak;
   * pass criteria: p99 < 16.7 ms, p99.9 < 25 ms, ≤ 1% missed vblanks, no frame over 50 ms, and limits on transitions, saves and loads;
   * nine scenes: title, Map 92 with UltraMode7, menus, trade screen, naval combat (the game has no engine battles), dialogue, video, save/load and soak.

   The M2, M6 and M7 exits are gated on it.

5. **Android moves to "Later"** (§17), with its own +4–8 pw, outside the estimate.

6. **New documentation and readability standard (§15).** It requires:
   * doc comments on every file, module and function, explaining why;
   * `ARCHITECTURE.md` and per-subsystem docs;
   * a compat-surface table, checked by CI against what the shims actually install;
   * a guide to adding a missing API;
   * naming and style rules, with no macros that hide control flow;
   * docs changed in the same PR as the code, enforced by CI and review;
   * the V8 handle and GL object lifetime rules, written in plain language.

7. **New privacy rule (§16).**
   * No personal names, emails, user names or host names in code, docs, commits or logs.
   * Commits are authored as `myonendy <350390780+myonendy@users.noreply.github.com>`.
   * A pre-commit hook and a CI job check this. The deny-list is kept outside the repo, and the scan never prints what it matched.
   * Smuggler's logger replaces the home path with `~`.

8. **The estimate is now ~36–69 pw** (was 28–55):
   * documentation +5–9;
   * 60 fps on two devices +2–4;
   * upstream PR work +0.5–1.5;
   * privacy scan +0.25.

   Each milestone now lists the docs it must deliver. Two rows are new: D (documenting inherited code) and U (upstream PRs). Calendar: ~8.5–16 months.

9. **Open questions updated (§11).**
   * Closed: Q1 (upstream), Q2 (PIXI), Q3 (Steam app ID 3789080; Proton saves found on dev-laptop), Q4 (platforms), Q11 (name) and v0.2's Q10 (no engine battles).
   * New: Q12 (assignment form), Q13 (Steam Cloud and native saves), Q14 (save fixtures for the trade and combat tests), Q15 (laptop power state), Q16 (Deck model and availability), Q17 (confirm names).

10. **A new lead for the missing HUD text (§5.5).** The HUD is drawn with the game's TextPicture plugin. It clears and redraws the same bitmap, so Outsider may not be re-uploading the texture. There is a test for this.

11. **Personal identifiers removed from all earlier documents.** The archived v0.2 and v0.3 specs, the QuickJS spec, the analysis documents and the test-drive logs no longer contain the owner's name, user name or host name.

---

## What changed from v0.2 to v0.3

1. **Start from Outsider instead of a new host.** We fork Outsider and change it, rather than writing a new runtime that borrows pieces. We inherit its window and input handling, its build and packaging, its audio output, its file and image code, its test tools and its working Windows, macOS and Linux builds.

2. **Swap QuickJS for V8 behind a thin adapter.** Outsider's 166 native functions currently talk directly to QuickJS. We put a small V8-only adapter under them, so they port almost mechanically. That is about 5,000 lines touched and roughly 2.5–4.5 weeks of work.

3. **Outsider's renderer cannot run real PIXI, so we add a WebGL layer.** Outsider draws sprites and quads, not WebGL. That is why the Mode 7 floor was black: its PIXI stand-in fakes the pieces UltraMode7 needs. We implement WebGL2 so that the game's own PIXI runs unmodified.

4. **WebGL goes through the standard OpenGL ES API, with ANGLE where needed.** This means the system driver on Linux, Steam Deck and Android, and ANGLE (the same layer Chrome uses) on Windows and macOS. Building WebGL on top of Outsider's SDL3 GPU layer would be 2–3 times the work, and its extra reach (consoles) is ruled out anyway.

5. **Keep SoLoud for audio rather than replacing it.** We add Opus/WebM decoding and the WebAudio fade and pan behaviour the game uses on top of SoLoud. A custom mixer node is the fallback only if the audio tests fail.

6. **Replace the video decoder rather than transcoding.** Outsider expects movies converted to MPEG-1 files next to the game. We decode the original VP9/Opus files directly instead.

7. **Fix the five failures from the test drive as concrete requirements:**
   * `document.getElementsByTagName('head')` must find the head element.
   * The NW.js window must support event listeners and a real close event.
   * `process.env` and the platform must be real, so that saves land in the proper per-OS folder.
   * `PIXI.Circle` and the rest come free with real PIXI.
   * HUD text that was missing gets a dedicated investigation, because its cause is not known yet.

8. **Never write into the game folder.** Outsider writes bytecode caches, logs and (by design) saves next to the game. All of that moves to the user's data, cache and log folders, with a guard that refuses writes inside the game folder.

9. **Migrate old saves by copying.** On first run, the fork can import saves from a Steam Proton prefix (or from the stray folder the test drive created). It never moves or deletes the originals.

10. **iOS and consoles stay out.** Without a JIT, V8 is 12–15 times slower. The map would likely run at 12–30 fps, WebAssembly (needed by Effekseer) switches off, and GPL clashes with those stores. Linux, Steam Deck, ARM64, Windows and macOS are all fine with the JIT; binaries come out at about 40–60 MB.

11. **Profile before believing the speedup.** "V8 fixes the 120–150 ms map frame" is now marked as a hypothesis. The first step is to profile Outsider as it is, then rerun the same drive on V8 with Outsider's old renderer. There is a go/no-go gate if the JavaScript still takes more than 12 ms.

12. **New milestone plan and estimate:**
    * **New estimate:** about 28–55 person-weeks, versus 30–58.
    * **Earlier milestones:** a V8 build playing the test drive after about 6–11 weeks, and a correct Mode 7 map at 60 fps after about 14–25 weeks.
    * The total barely drops, because WebGL is still the biggest job.

13. **Upstream only the small general fixes.** Outsider requires contributors to assign copyright to its company. So the decision is ours: either contribute the small general fixes as code under that condition, or report them as issues only. V8, WebGL, ANGLE and video stay in the fork.

14. **Licensing stays GPL-3.0-or-later.** We use Outsider under its GPL option and keep its headers. Our new files are GPL-only. No game files and nothing from the Unity wrapper go into the repo.
