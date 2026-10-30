# smzuggler: Specification v0.5.1, an Outsider fork with V8 and real PIXI

**Target:** run *HIROTONFA's American Revolution Smuggler* (RPG Maker MZ 1.10.0, shipped on NW.js 0.80 / Chrome 117 / V8 11.7; re-verified from the game folder in v0.5, §18.1) natively, with the game's **real, unmodified** JavaScript. That means stock `rmmz_*.js`, real PIXI 5.3.12 and all 57 enabled plugins. It runs on an **embedded V8** inside **smzuggler**, a GPL-3.0-or-later fork of Outsider. smzuggler is portable across Linux (x64/arm64, Steam Deck), Windows and macOS. Android is deferred (§17 Later). RPG Maker MV games are a later extension (§18).

**Naming in this document:** "smzuggler" (or "the project") always means our runtime. The name is a pun on MZ, and it is written in lower case. "The game" always means HIROTONFA's *American Revolution Smuggler*. smzuggler is not affiliated with HIROTONFA (§13).

**Status:** draft v0.5.1, 2026-10-30. Author: Grok Bot, for the project owner.
* **Supersedes v0.5.** Earlier drafts are kept beside this file as `V8_MZ_HOST_SPEC_v0.5.md`, `V8_MZ_HOST_SPEC_v0.4.md`, `V8_MZ_HOST_SPEC_v0.3.md` and `V8_MZ_HOST_SPEC_v0.2.md`. What changed in each version is listed in `CHANGES.md`.
* Like v0.3 and v0.4, the plan is to fork Outsider and change it. We keep its platform layer, build, packaging, tests, audio device and most of its native modules. We replace its JS engine and its PIXI reimplementation.
* Module internals that this spec does not change are **carried forward from v0.2 by reference**. The section map is in Appendix B.

**Decisions taken in v0.5.1 (owner, 2026-10-30):**
* **No stock-NW.js gate:** Gate G0 is dropped. The laptop's 60 fps gate stands on its own, whatever stock NW.js does. Q20 closed (§14.2).
* **The laptop gate is the operative gate:** until a friend's Steam Deck is available, the working assumption is that passing on dev-laptop on battery with `power-saver` means the Deck passes too. M2 and M6 sign off on the laptop gate. The Deck run is a **confirmation pass**, required before release (M7) (§14, §14.8). `deck-proxy` becomes optional (§14.9). Q19 stays open for timing only.
* **Fixture saves:** the perf tests need exactly three fixture saves: Map 92, a port next to the store (trade screen) and the sea next to an encounter (combat). They are never committed or shared; testers create their own (§14.10). Q14 answered.

**Decisions taken in v0.5 (owner, 2026-10-30):**
* **Name:** the project and fork are called **smzuggler** (a pun on MZ). Repository `smzuggler`, binary `smzuggler-rt` (§13). This closes Q17.
* **Engine check:** the game is RPG Maker **MZ** 1.10.0, not MV. The evidence from the game folder is in §18.1.
* **MV support** is planned as a later extension, outside the main estimate (§18).
* **Steam:** no Steam integration for now. No Steamworks, no Steam APIs, no Steam client needed. The game folder is found by path or config (§5.3). Steam Cloud: a one-time save copy is fine (Q13 and v0.2 Q8 closed).
* **Steam Deck:** with no Deck owned yet, Deck runs use a borrowed Deck or an outside tester, recorded as a **Deck-equivalent**, and the tester returns a scrubbed frame-time log (§14.8). A `deck-proxy` run on dev-laptop is clearly labelled as a proxy and **not a pass** (§14.9). Q16 closed. *(Changed in v0.5.1: the Deck run is now a confirmation pass before release, and the laptop gate signs off M2 and M6.)*
* **dev-laptop:** the must-pass gate now runs **on battery with the `power-saver` profile**, the owner's normal way of using it. AC with `balanced` becomes an extra report. Q15 closed. This makes the laptop gate stricter, and §14.2 re-assesses plausibility.

**Decisions from v0.4, unchanged:**
* **Upstream:** general fixes go back to Outsider as **code PRs**. Copyright assignment for those patches is accepted (§8).
* **PIXI:** run real, unmodified PIXI 5.3.12. The cheaper shim route is closed.
* **60 fps:** must-pass on dev-laptop (battery, `power-saver`) at M2, M6 and M7, and confirmed on a Steam Deck before release (§14; changed in v0.5.1).
* **Android:** it can wait (§17 Later).
* **Documentation:** everything is meticulously documented and easy for humans to read, understand and change. §15 sets the standard, and it is a review gate.
* **Privacy:** no personal names, emails, user names or host names anywhere in the project. Commits use a fixed project identity, and a scan enforces both (§16).

**Decisions carried forward (owner, 2026-10-30):**
* V8 is the only JS engine.
* GPL-3.0-or-later.
* libvpx + libopus + nestegg for video, with no transcode cache.
* Real unmodified PIXI 5.3.12 and stock rmmz.
* The game folder is read-only.
* No Steam integration for now (§5.3, §11).
* No game code or assets in any repo.
* Nothing is copied from the proprietary Unity wrapper. Describing its architecture as prior art is fine.

**Evidence tags:**
* **[M]** measured, either on the box or in the Outsider test drive;
* **[V]** verified by reading code at the cited path;
* **[G]** a guess or estimate;
* **[U]** unverified: a hypothesis that needs a specific test, which is named.

**Outsider baseline:**
* Repository: `github.com/General-Arcade/outsider`.
* Commit **`f789f46`** (2026-09-24 13:15 +08:00, "Note the experimental Android build in the README").
* Every path below is relative to that tree unless marked otherwise.

**Test-drive evidence:** a run of Outsider f789f46 (QuickJS) against a *copy* of the game folder, in control mode.
* Its stdout and screenshots are summarised in §2.
* Raw logs stay off-repo. Logs shared outside the project must have the user name and host name scrubbed (§16).

---

## 0. Executive summary

* **What Outsider is, measured against our needs:**
  * Outsider is a working, portable native MZ runtime in C, with ~29.5k lines under `src/` [V `wc`]. It has:
    * an SDL3 platform layer;
    * an SDL3 GPU renderer (Vulkan, D3D12 or Metal) with an OpenGL 4.5 fallback;
    * SoLoud audio and a CPU Canvas2D;
    * a control and screenshot mode;
    * an evaluator, a C test suite and build/packaging tools.
  * Its two weak points for this game are exactly the two things we have already decided to change:
    1. **QuickJS** is 22–26× slower than V8 on our benches [M].
    2. A **hand-written PIXI shim** (`src/shims/pixi_shim.js`, 4,928 lines) stubs out `Geometry`, `Buffer`, `Shader` and `Program`. That leaves UltraMode7, used on ~46 of the 97 maps, rendering a black floor.
* **What the test drive showed [M]:**
  * The title runs at ~60 fps, using 1.4–2 ms of JS per frame.
  * **Map 92 runs at ~7 fps**: 112–166 ms per frame, of which 88–156 ms is JS, with spikes up to 3.6 s. Its floor renders black.
  * The menu was reached; `Scene_Menu` ran at ~53 fps.
  * Save and load worked, but the save went to the relative path `undefined/.local/share/…`.
  * Five compatibility gaps were found. All are small, and all are specified in §5.
* **The adjusted plan:**
  * **Keep:** SDL3 platform/input/windowing, the SoLoud output device and mixer, image and WOFF loading, Canvas2D (initially), file I/O, the game loop and its stats, control mode, the evaluator, the C tests, CMake and the packaging scripts.
  * **Replace:**
    * QuickJS with V8. This goes behind a thin C binding seam, so the 166 existing native functions port mostly mechanically.
    * Outsider's quad/sprite renderer and PIXI shim with a **WebGL2 implementation**, so that real PIXI runs.
  * **Recommended WebGL2 backend: the GLES 3.0 API**, implemented by:
    * the system driver on Linux, Steam Deck and Android;
    * **ANGLE** on macOS and Windows, and optionally everywhere.
    * Emulating WebGL on top of SDL3 GPU is possible but costs roughly 2–3× more work. It only pays off for console-class targets, which GPL and the JIT ban rule out anyway (§4).
* **Estimate: ~38–72 person-weeks to full parity** (37.75–71.5), versus v0.5's 38–71.75 and v0.4's 36–69 [G] (§9).
  * **v0.5.1 removes 0.25 pw:** Gate G0, the stock-NW.js baseline, is dropped. Making the Deck a confirmation pass moves no work; it moves *when* the Deck run happens.
  * **v0.5 added +1.75–3.25 pw** over v0.4, from the stricter 60 fps gates: a larger tuning reserve for dev-laptop on battery with `power-saver` (+1–2), Gate G0 (+0.25, removed again in v0.5.1) and the outside-tester kit plus the Deck proxy (+0.5–1). The rename and dropping Steam integration change nothing measurable.
  * The v0.4 increase over v0.3 came from the documentation standard (§15, +5–9 pw), the 60 fps must-pass on two devices (§14, +2–4 pw) and upstream PR work (§8, +0.5–1.5 pw).
  * Android (§17) and MV support (§18, ≈ 10–20 pw) are **not** in the total.
  * The largest single item is still WebGL2 for real PIXI.
  * **What does change is risk and the time to first results:**
    * smzuggler on V8 should play the same test drive after **~8–14 person-weeks** (M0+M1);
    * Mode 7 and the first 60 fps gate (title, Map 92 and menus on dev-laptop on battery with `power-saver`, §14) arrive at M2, at **~17.75–31 person-weeks**.
* **Performance:** the claim that "V8 removes most of the 120–150 ms map frame" is a **hypothesis [U]**, though a well-supported one.
  * Chrome runs the same frame at a 2.57 ms mean.
  * QuickJS-to-V8 factors of 22–26× would put the pure-JS share at ~55–70 ms.
  * The rest is shim and native work, which has not been attributed yet.
  * The first job in M0 is to profile Outsider itself (§6), and M1 is gated on the measured result.

---

## 1. Goals and non-goals (changed from v0.2 §1)

**Goals:**
1. The game runs on smzuggler at parity with NW.js: visuals within the §10 tolerances, audio, video and saves.
2. Platforms:
   * **Primary:** Linux x64, Linux arm64 and Steam Deck (SteamOS, x64).
   * **Secondary:** Windows x64/arm64 and macOS arm64/x64.
   * **60 fps must-pass:** dev-laptop on battery with `power-saver` (the operative gate), with a Steam Deck confirmation pass before release (§14).
   * **Later:** Android arm64 (§17), not in the estimate.
   * v0.2 listed Windows and macOS as secondary but untested. smzuggler inherits Outsider's working Windows/D3D12 and macOS/Metal build plumbing, so they are realistic now. Their *rendering* path still changes (§4).
3. Stay mergeable with upstream Outsider. General, engine-neutral fixes go upstream as small code PRs (§8).
4. No writes into the game folder, ever. That includes bytecode caches and logs (§5.6).
5. Every file, module and function is documented so that a newcomer can understand and change it (§15).
6. No personal identifiers anywhere in the project (§16).

**Non-goals:**
* iOS and consoles (§3.3).
* Running other MZ games is a side effect, not a goal. Outsider's QuickJS build stays the upstream route for those.
* RPG Maker **MV** games are a later extension with their own estimate (§18), outside this plan. Nothing in M0–M7 may make MV harder (§18.5).
* Steam integration: no Steamworks SDK, no Steam API calls and no Steam client requirement (§5.3).
* No engine abstraction that keeps QuickJS alive in smzuggler (§4.1).
* Android in this plan (§17).

---

## 2. What the Outsider test drive proved [M]

| Observation | Evidence | Root cause | Spec item |
|---|---|---|---|
| Title runs at ~59.8 fps; JS 1.4–2 ms per frame | control-mode stats | n/a | — |
| Map 92 runs at 6.8 fps overall; `dt_avg` 112–166 ms, `js_update_avg` 88–156 ms, max 1,045 ms and 3,601 ms | control-mode stats | QuickJS speed, plus unattributed shim and native cost [U] | §6 |
| Mode 7 floor is black | screenshots `map_*.png` | `PIXI.Geometry` (8 constructions) and `PIXI.Buffer` (16) are stubs; first hit from UltraMode7.js lines 1866/1868 | §4.2 |
| `renderer` is missing `renderTexture`, `filter`, `mask`, `context`, `buffer`; plugins `extract`, `rpgtilemap`, `um7tilemap` are missing | `[PROF]` line | `pixi_shim.js` `PixiRenderer` exposes only what stock rmmz needs | §4.2 |
| Crash in Mimosa_MouseCursor (`styleParent` undefined) | log, before compat patch | `document.getElementsByTagName('head')` returns `[]` | §5.1 |
| CleanExit throws on `removeAllListeners` | log, before compat patch | `nw.Window.get()` has only a no-op `on` | §5.2 |
| CGMZ_Core throws: `PIXI.Circle` undefined | log, before compat patch | no `Circle` in `pixi_shim.js` | §5.4 |
| Save went to `undefined/.local/share/HIROTONFA_AmericanRevolutionSmuggler/file19.rmmzsave`, relative to the working directory; save and load worked | log | `process.env` is `{}` | §5.3 |
| HUD panels drew without their text; message-window text did draw ("Wh…" typewriter) | `map_100.png`, `menu.png` | **unknown [U]**: a font, a Canvas2D op or a plugin path | §5.5 |
| `Scene_Menu` reached; 18.9 ms average at f=2880 (53 fps), 56.9 ms at f=2820 | stats | the visuals are unconfirmed | §10 |
| Driver commands later timed out ("no reply to command … within 30s") | drive log | likely main-thread stalls on QuickJS (the 1–3.6 s frames) [U] | §6 |

The three gaps that crashed were papered over for the drive by a *diagnostic* script, not by any proposed fix (`profiling/compat_experiment.js` in the local Outsider checkout, which is not upstream). That script:
* returns `document.head`/`document.body` from `getElementsByTagName`;
* adds no-op `removeAllListeners`, `removeListener`, `on`, `once` and `off` to `nw.Window`;
* adds a minimal `PIXI.Circle`.

§5 specifies the real fixes.

---

## 3. Portability matrix

| Target | V8 JIT | Graphics (recommended) | Audio | Status in Outsider f789f46 [V README/CMake] | Plan |
|---|---|---|---|---|---|
| Linux x64 (X11/Wayland) | ✅ | system GLES 3.0 / GL 3.3 core via SDL3/EGL; ANGLE-Vulkan optional | SoLoud→SDL3 | ✅ Vulkan via SDL3 GPU | **Primary.** dev-laptop (§14.1) is a **60 fps must-pass** target |
| Steam Deck (SteamOS 3, Zen 2, RADV/radeonsi, gamescope) | ✅ | Mesa GL/GLES | SDL3 (PipeWire) | ✅ same binary as Linux x64 [G] | **Primary.** Build against the Steam Linux Runtime 3.0 "sniper" SDK so glibc and library ABIs match [G] (a build container only: nothing at run time needs Steam); 1280×800 output, letterboxed. **60 fps must-pass (§14)** |
| Linux arm64 | ✅ | system GLES | SDL3 | ✅ listed | Primary |
| Windows x64/arm64 | ✅ | **ANGLE → D3D11** | SDL3 (WASAPI) | ✅ D3D12, MSVC | Secondary. V8 needs clang-cl for the V8 build; the host can stay MSVC or clang-cl [G] |
| macOS arm64/x64 | ✅ (MAP_JIT; hardened runtime needs the `allow-jit` entitlement [G]) | **ANGLE → Metal** (Apple's GL is deprecated and stops at 4.1) | SDL3 (CoreAudio) | ✅ Metal; the GL fallback is unavailable on macOS | Secondary |
| Android arm64 | ✅ | system GLES 3.0 | SDL3 (AAudio) | ⚠️ experimental: `libmain.so`, GPU backend only, no app project, touch or packaging | **Later** (§17), not in the estimate |
| iOS | ❌ JIT forbidden → `--jitless` | ANGLE → Metal | — | ❌ | Non-goal (§3.3) |
| Consoles | ❌ JIT forbidden; GPL conflicts with SDK terms [G] | — | — | ❌ | Non-goal |

### 3.1 V8 and JIT
* V8's optimising tiers (Sparkplug, Maglev, TurboFan) are supported on x64 and arm64 for Linux, Windows, macOS and Android [G: per v8.dev target list; to be re-checked against the pinned version].
* The JIT needs writable-then-executable memory. All four OSes allow it.
* On macOS this needs MAP_JIT and the JIT entitlement once the app is hardened and notarised [G].

### 3.2 Binary size [G]

| Component | Size |
|---|---|
| Outsider binary today (QuickJS, SDL3 static, SoLoud, glslang/SPIRV-Cross/shadercross) | a few MB to ~15 MB [G, not measured; measure it in M0] |
| V8 monolith, stripped | +25–35 MB |
| ICU data | +1–10 MB (Q8) |
| ANGLE (libEGL + libGLESv2) on Windows/macOS | +6–12 MB [G] |
| libvpx, libopus, nestegg, FreeType, HarfBuzz | +3–6 MB |
| **Total** | **~40–60 MB per platform**, versus a couple of hundred MB for NW.js |

Removing the runtime GLSL→SPIR-V→MSL toolchain (glslang, SPIRV-Cross, shadercross) from the real-PIXI build saves several MB where ANGLE is used [G].

### 3.3 Jitless: the honest cost (iOS and consoles)
* **[M]** V8 `--jitless` is **12–15× slower** than JIT on the real-PIXI bench (2.76 ms vs 0.19 ms; 6.91 ms vs 0.56 ms at 116 filters).
  * It is still ~1.8× faster than QuickJS-ng (12.46 ms).
  * WebAssembly is **disabled** under jitless, and Effekseer's wasm then fails to initialise. Per v0.2 §5.11, `main.js` stops if Effekseer fails.
* **[U] Map 92 frame under jitless:**
  * Scaling Chrome's 2.57 ms mean by 12–15× gives ~31–39 ms.
  * Scaling the test drive's QuickJS 88–156 ms by ÷1.8 gives ~50–85 ms.
  * The truth is likely in the 30–85 ms range, i.e. **12–30 fps**: not shippable at 60 fps without native fast paths (v0.2 §7.2 #2).
  * Test: build `v8_jitless=true` in M1 and rerun the drive.
* **Conclusion unchanged:** iOS and consoles stay out of scope. If they are ever wanted, the project would need:
  * jitless V8;
  * native PIXI and plugin hot paths;
  * a non-wasm Effekseer, i.e. Outsider's native `RMMZ_USE_EFFEKSEER` path;
  * a licensing answer for GPL on closed platforms.

---

## 4. Architecture of smzuggler

```
 game folder (read-only) ── stock rmmz + real pixi.js 5.3.12 + 57 plugins
         │
 ┌───────▼──────────────── V8 isolate (one context = window) ─────────────────┐
 │ prelude: Outsider shims, fixed (§5) — dom_shim, canvas2d_shim, font_shim,  │
 │   navigator_shim, xhr_shim, storage_shim, webaudio_shim, effekseer_shim,   │
 │   dom_overlay, plugin_compat   (pixi_shim + tilemap_shim NOT loaded)       │
 │ + new: webgl2 bindings (C++, V8 API directly; hot calls via Fast API)      │
 └───────▲──────────────────────────────────────────────▲─────────────────────┘
         │ jsb_* C seam (§4.1) — 13 __native_* objects  │
 ┌───────┴──────────┐ ┌────────────┐ ┌──────────────┐ ┌─┴────────────────────┐
 │ Outsider C core  │ │ audio:     │ │ canvas2d.c → │ │ webgl2: GLES 3.0 via  │
 │ platform_sdl3,   │ │ SoLoud dev │ │ (M3) FreeType│ │ system driver or      │
 │ input, file_io,  │ │ + WebAudio │ │ + HarfBuzz   │ │ ANGLE (Metal/D3D11/   │
 │ game_loop stats, │ │ node layer,│ │ text         │ │ Vulkan); SDL3 makes   │
 │ control mode     │ │ +Opus      │ │              │ │ the GL/EGL context    │
 └──────────────────┘ └────────────┘ └──────────────┘ └──────────────────────┘
```

### 4.1 Replacing QuickJS: the binding seam
**What exists [V]:**
* QuickJS is touched in only four places:
  * `src/bindings/*.c`: 12 files, 3,688 lines, 166 `JS_CFUNC_DEF` entries, exposed as 13 `__native_*` globals;
  * `src/engine/js_engine.c` (614 lines) behind the small C API in `src/engine/js_engine.h`, covering init, eval, eval_file, eval_file_cached, execute_pending_jobs, console callback, `js_engine_get_context` and the watchdog;
  * `src/engine/game_loop.c` (683 lines, ~93 `JS_` references; it caches `__dom_flushTimers`, `__dom_flushAnimationFrames` and similar at lines ~81–108);
  * `src/engine/error_handler.c` (225 lines, ~37 references).
* `src/engine/script_loader.c` and `src/main.c` contain **no** `JS_` calls. They talk only to `js_engine.h`.

QuickJS references per binding file [V `grep -c`]:

| Binding file | JS_/JSValue references | Functions |
|---|---:|---:|
| `bind_canvas2d.c` | 279 | 27 |
| `bind_renderer.c` | 211 | 27 |
| `bind_audio.c` | 197 | 28 |
| `bind_effekseer.c` | 173 | 26 |
| `bind_filters.c` | 140 | 12 |
| `bind_io.c` | 106 | 12 |
| `bind_image.c` | 92 | 8 |
| `bind_font.c` | 83 | 7 |
| `bind_input.c` | 65 | 3 |
| `bind_video.c` | 62 | 8 |
| `bind_platform.c` | 47 | 7 |
| `bind_tilemap.c` | 34 | 1 |

**Design:**
1. Add a thin **C seam, `jsb.h`**, implemented once in C++ on V8 (~800–1,200 lines [G]). It provides:
   * opaque `jsb_ctx` and `jsb_val` handles, valid only for the duration of the call;
   * argument getters: `jsb_arg_f64/i32/bool/str/buf/obj`;
   * returns: `jsb_ret_*`;
   * `jsb_throw_type`;
   * `jsb_new_object`, `jsb_set_fn`, `jsb_set_prop`;
   * ArrayBuffer and TypedArray views with no copy;
   * persistent callback handles for audio `onended` and video events;
   * `jsb_define_module("__native_audio", table)`.
   
   **This is not an engine abstraction.** It has exactly one implementation, V8. It exists so that Outsider's C bindings port by near-mechanical rewriting, and so that upstream fixes to those bindings can be re-applied with small diffs.
2. Port the bindings we keep through `jsb`: audio, canvas2d, font, image, input, io, platform, video, effekseer (native option). That is ~2,650 lines [V, sum of those files].
   * `bind_renderer.c`, `bind_filters.c` and `bind_tilemap.c` (~1,030 lines) are **not ported** in real-PIXI mode. They serve only `pixi_shim.js` and `tilemap_shim.js`. They are ported temporarily in M1 so that the V8 swap can be measured on Outsider's existing renderer (§9).
3. Reimplement `js_engine.c` on V8, keeping the `js_engine.h` API so that `script_loader.c` and `main.c` need no changes. It covers:
   * isolate and context setup;
   * `MicrotasksPolicy::kExplicit`, with a checkpoint after every macrotask (timer, rAF batch, I/O callback, input event). Today `game_loop.c` drains jobs once per phase, which is close but not HTML-exact; v0.2 §4.1 has the rules;
   * a watchdog via `Isolate::TerminateExecution` from a timer thread, replacing `JS_SetInterruptHandler`;
   * console;
   * the `ScriptOrigin` file name, for stack traces.
4. **Bytecode cache:** replace `eval_file_cached`'s `.jsc`-next-to-source (`js_engine.c:437–571`) with the V8 code cache in `$XDG_CACHE_HOME/smzuggler-rt/v8cache/`, keyed by `sha256(source)` + V8 version + flags. Never write to the game folder.
5. Port `game_loop.c` and `error_handler.c` to `jsb` or V8 (~150–250 changed lines [G]).
6. Remove the MSVC `/STACK:16777216` and `JS_STACK_SIZE` hacks. Set V8's `--stack-size` to match the main thread stack instead.

**Estimate:**
* ~5.2k lines of QuickJS-facing code [V] become ~1k lines of new seam and engine code plus ~3.5–4k lines of mechanically ported bindings [G].
* **2.5–4.5 person-weeks**, including the V8 build pipeline from v0.2 §11.2 (CI-built monolith tarball, option A) [G].
* The hot WebGL entry points do not go through `jsb`. They are written directly against V8 templates with **Fast API calls** (§4.2).

### 4.2 Rendering: real PIXI 5.3.12 on WebGL2

**What Outsider's renderer actually targets [V]:**
* Device: SDL3 GPU (D3D12, Vulkan or Metal), in `src/rendering/gpu_backend.c`, `sprite_batch_gpu.c`, `renderer_gpu.c` and `filters_gpu.c`.
  * Shaders are HLSL, precompiled to DXIL, SPIR-V and MSL (`tools/compile_shaders.py` → `shaders_generated.h`).
  * Plugin GLSL filters are translated at runtime: `glsl_translate.c` → glslang → SPIR-V → SDL_shadercross (`gpu_shader_runtime.c`).
* Fallback: OpenGL 4.5 with DSA (`gl_loader.c`, `sprite_batch.c`, `renderer.c`, `filters.c`). It is not available on macOS and is not GLES-compatible.
* **API exposed to JS** (`src/bindings/bind_renderer.c`, `src/rendering/renderer.h`): a high-level sprite-batch API. It offers begin/end frame, textures, render textures (FBO), `drawQuad` / `drawQuadVertices` / `drawQuadVerticesUV`, blend mode, scissor and readPixels, plus a filter compiler.
  * **There is no WebGL context, no vertex/index buffers, no VAOs, no stencil and no arbitrary `drawElements`.**
  * `pixi_shim.js` maps PIXI's scene graph onto quads.
* **Consequence:** real PIXI cannot run on Outsider's renderer as it stands. Real PIXI calls `canvas.getContext('webgl2')` and drives the full WebGL API, while UltraMode7 adds its own `Program`, `Geometry`, `Shader` and raw `gl.drawElements`.
  * We need a **WebGL2 implementation**, which v0.2 §5.2 already specified.
  * Outsider's renderer is retained only through M1, as a measuring baseline.

**Backend options:**

| | A. WebGL2 → GLES 3.0 API, 1:1 (**recommended**) | B. WebGL2 emulated on SDL3 GPU |
|---|---|---|
| Mapping | Nearly one-to-one: WebGL2 *is* GLES 3.0 plus validation | A GL-style state machine has to be built on a modern explicit API. Blend, depth/stencil, vertex layout and shaders are baked into cached pipeline objects; uniforms are packed into uniform blocks; texture units are mapped to sampler slots; render-pass boundaries are inferred from FBO binds and clears; `readPixels` needs a sync download |
| Shaders | GLSL ES 1.00/3.00 compiled by the driver or ANGLE, as in Chrome. Rewrite only for desktop GL 3.3 core (v0.2 §5.2) | Runtime GLSL ES → SPIR-V → DXIL/MSL. Outsider's `glsl_translate.c` + `gpu_shader_runtime.c` give a head start, but only for fragment filters |
| Fidelity to Chrome | **ANGLE is what Chrome itself uses**, so precision, blending and sampler behaviour match by construction | New code, so every difference is ours to find |
| Portability | Linux/Deck/Android: system GLES or GL. Windows: ANGLE→D3D11. macOS: ANGLE→Metal. iOS: ANGLE→Metal (moot). Consoles: no | Vulkan, D3D12 and Metal natively. Console backends exist for SDL3 GPU under NDA, but are irrelevant (§3) |
| Effort [G] | 5–9 pw for the binding and state layer + 0.5–1.5 pw for ANGLE integration | 12–20 pw, essentially a small ANGLE |
| Size | +6–12 MB ANGLE on Windows/macOS | glslang + SPIRV-Cross + shadercross already in Outsider |

**Recommendation: A.**
* Real PIXI and UltraMode7 are written for WebGL. ANGLE gives Chrome-exact behaviour on the platforms that lack good native GL, and system GLES is enough on Linux, Deck and Android.
* SDL3 still creates the window and the GL/EGL context (`SDL_GL_*`, or EGL with ANGLE).
* Keep a build switch to use ANGLE everywhere (`-DMZ_GLES=angle`). That is also the Linux CI reference oracle (v0.2 Q4, now **Q5** in §11).
* B is the more *theoretical* portability win, but it buys nothing for the targets we can ship. Revisit it only if ANGLE proves unworkable on a target.

**What happens to Outsider's PIXI shim:**
* In smzuggler's real-PIXI mode, `script_loader.c` does **not** load `pixi_shim.js` or `tilemap_shim.js`. The shim list is at `script_loader.c:246–256`, and the post-core tilemap shim at ~294/470.
* It loads the game's own `js/libs/pixi.js` instead.
* `canvas2d_shim.js` keeps providing `HTMLCanvasElement.getContext('2d')`. A new `webgl2` branch returns the native WebGL2 context.
* PIXI creates its view canvas via `document.createElement('canvas')`, which `dom_shim.js` already supports. The WebGL2 context binds to the SDL window's default framebuffer.
* Outsider's mode stays selectable (`--renderer=outsider`) for comparison, and for upstream compatibility while smzuggler tracks upstream (§8.3).

**WebGL2 surface:** unchanged from v0.2 §5.2. It has:
* full context state;
* buffers, VAOs, textures (`texImage2D` from Canvas2D, images and typed arrays), FBOs and renderbuffers with stencil;
* `drawElements`/`drawArrays` and instancing as needed;
* `getExtension` returning only what Chrome 117 reports for the extensions PIXI 5.3.12 probes;
* `getParameter` limits.

Hot calls use V8 Fast API: `uniform*`, `bindBuffer`, `bufferSubData`, `drawElements`, `bindTexture`, `bindVertexArray`, `useProgram`.

### 4.3 Audio: keep SoLoud as the device and mixer
**Outsider today [V]:**
* `src/audio/audio_engine.cpp` (639 lines) wraps SoLoud `Wav` (fully decoded) and `WavStream`. That supports Ogg Vorbis and WAV. There is no Opus and no WebM.
* `src/audio/soloud_sdl3_backend.cpp` is the SDL3 output path.
* `src/shims/webaudio_shim.js` (600 lines) presents a WebAudio subset over it, including `decodeAudioData` at ~461.
* SoLoud is fetched from `GIT_TAG master` (`third_party/CMakeLists.txt:39`), i.e. **unpinned**. Pin it.

**Plan (changed from v0.2, which said "replace with miniaudio"):**
* Keep SoLoud's SDL3 device, mixer, voices, faders and Vorbis decoding.
* Add an **Opus source**: libopus plus a WebM demux (nestegg) for `police radio.ogg`, which is actually WebM/Opus, and for the movie audio.
* Add decode of *truncated* Ogg data handed to `decodeAudioData` (v0.2 §5.6).
* Extend `webaudio_shim.js` toward the node semantics rmmz uses: `GainNode` ramps (`linearRampToValueAtTime`, `setValueAtTime`), `PannerNode`/`StereoPanner`, `playbackRate` and `loopStart`/`loopEnd`. They map onto SoLoud's `fadeVolume`, `setPan`, `setRelativePlaySpeed` and loop points.
* **Risk [U]:** SoLoud fades run per audio block, not per sample. If the conformance audio test (v0.2 §10.2) shows audible differences, add a small custom `AudioSource` graph node. Do not swap out SoLoud.
* Effort 2.5–5 pw [G], versus 3.5–6 in v0.2.

### 4.4 Video
* Outsider plays MPEG-1 via `pl_mpeg` (`src/video/video_player.c`). It expects WebM/MP4 to be **transcoded to `.mpg`** next to the original (`dom_shim.js:964–988`). That breaks both the read-only rule and the no-transcode rule.
* **Replace the decoder with nestegg + libvpx (VP9) + libopus** behind the same `video_player.h` interface and `__native_video` binding. Keep Outsider's overlay and composition path (`dom_overlay.js`, `game_loop.c`'s video phase).
* `canPlayType` (`dom_shim.js:1150–1153`) must answer like Chrome 117 for `video/webm; codecs="vp9,opus"`.
* Effort 3–5 pw (v0.2 §5.7).

### 4.5 Canvas2D and text
* Keep Outsider's CPU Canvas2D (`src/rendering/canvas2d.c`, 1,053 lines, with stb_truetype fonts via `font_manager.c` and WOFF1 via `woff.c`) for M1–M2.
* The game ships 6 TTF/OTF fonts (including `CatComic.otf`) and 2 WOFF files [V `fonts/`].
* In M3, either swap the glyph path to FreeType + HarfBuzz as in v0.2 §5.3/§5.5, or keep stb_truetype if the HUD-text test (§5.5) passes within tolerance. Decide on the evidence.

### 4.6 Kept unchanged from Outsider (inherit as-is, fix in place)
* `src/platform/platform_sdl3.c` (769 lines): window, events, DPI, letterbox, fullscreen.
* `src/platform/android_support.c` (132 lines), kept for the deferred Android target (§17).
* `src/input/input_manager.c` + `navigator_shim.js` gamepads + `dom_shim.js` key and mouse events. Steam Deck controls: when the owner adds the binary to Steam as a non-Steam game, Steam Input presents them as a virtual gamepad; when it is run outside Steam (desktop mode), SDL3's HIDAPI Steam Deck driver reads them directly [G: confirm on a device at M2]. Neither path uses Steamworks.
* `src/io/file_io.c` (534 lines, with Windows drive-letter handling at ~395) and `bind_io.c`, extended by §5.3 and §5.6.
* `src/rendering/image_loader.c` (stb_image) and `apng.c`. PNG colour handling is per v0.2 Q5, now **Q6** in §11.
* `src/engine/script_loader.c`, the load order (`script_loader.c:246–256, 282–287, 486`) and `game_loop.c`'s phases and `GameLoopStats`.
* `--control` mode (stdin commands, frame stepping, PPM screenshots) and `src/shims/control_shim.js`.
* `tools/evaluator/` (CDP-based NW.js reference driver, scenarios, compare and report), `tools/visual_eval.py`, `tools/compare_backends.py`, `tools/build_game.py` (packaging) and the ~24 C tests in `tests/` plus `tests/integration/`.
* The native Effekseer option (`RMMZ_USE_EFFEKSEER`) stays as a fallback. The default is the game's own `effekseer.wasm` under V8 (v0.2 §5.11).

---

## 5. Compatibility conformance list (from the test-drive failures)

Each item lists: **requirement**, **Outsider root cause [V]**, **test**, and **upstreamable?**

### 5.1 DOM: `head`, `body` and element stubs
* **Requirement:** `document.getElementsByTagName('head'|'body'|'html')` returns live arrays containing the singleton elements. The same applies to `getElementsByTagName('*')`, `querySelector('head')` and `document.documentElement.children`.
  * `HTMLElement.prototype.getElementsByTagName` and `querySelector`/`querySelectorAll` search the element's subtree instead of returning `[]` or `null`.
  * `appendChild`/`removeChild` on `head` keep `parentNode` consistent. Mimosa_MouseCursor.js:1158–1160 compares `$.style.parentNode === $.styleParent` every frame.
* **Root cause:**
  * `src/shims/dom_shim.js:1261` creates `_head`, but `_allCreatedElements` starts as `[_refresherCanvas]` (1269) and never includes `_head`/`_body`. `document.getElementsByTagName` (1337–1344) only scans that list.
  * `document.head` (1278) and `querySelector('head')` (1353) work.
  * The element-level `getElementsByTagName` (533) always returns `[]`.
* **Test:** a unit test in `tests/` plus a probe: Mimosa_MouseCursor.js:480 obtains a non-null `styleParent`, and the cursor style toggles without throwing for 600 frames.
* **Upstream:** yes, an engine-neutral ES5 fix.

### 5.2 `nw.Window` events and the `nw` object
* **Requirement:** `nw.Window.get()` returns a singleton that is a minimal **EventEmitter**: `on`/`addListener`, `once`, `off`/`removeListener`, `removeAllListeners([name])`, `emit` and `listeners`, all returning `this`. It also has `closeDevTools`, `close(force)`, `focus`, `show`, `hide`, `maximize`, `restore`, `enterFullscreen`/`leaveFullscreen`/`toggleFullscreen`/`isFullscreen`, `width`/`height`/`x`/`y`, `title` and `zoomLevel`.
  * The host delivers a **`close`** event when the window is closed (SDL quit or window close). As in NW.js, if a `close` listener exists, the window does not close until `close(true)` or `nw.App.quit()` is called.
  * `nw.App` has `argv`, `quit`, `closeAllWindows`, `dataPath` (the host data dir, never the game folder) and `manifest` (parsed `package.json`).
* **Root cause:** `src/shims/storage_shim.js:374–418` has `on: function(){}` as a no-op, and no `removeAllListeners`, `removeListener` or `once`. CleanExit.js:48–51 calls `removeAllListeners("close")` and then `on("close", forceQuit)`.
* **Test:** closing the window during the map runs CleanExit's `forceQuit`, and the process exits 0 within 1 s with no orphan audio thread.
* **Upstream:** yes. The JS part is engine-neutral; the close-event delivery is a small change in `platform_sdl3.c` and `game_loop.c`.

### 5.3 `process`, `require` and platform save paths
* **Requirement:**
  * `process.env` is a real snapshot of the host environment. Optionally, a filtered allow-list can be used for privacy: `HOME`, `USER`, `XDG_*`, `APPDATA`, `LOCALAPPDATA`, `USERPROFILE`, `ProgramData`, `ALLUSERSPROFILE`, `PUBLIC`, `TEMP`/`TMP`, `LANG`.
  * `process.platform` is the **real** OS (`linux`, `win32` or `darwin`; Android reports `android`, as Node does).
  * `process.versions` reports `{node, v8, nw, chromium}` with the reference values (`nw: "0.80.0"`, `chromium: "117.0.5938.63"`).
  * Also required: `process.arch`, `process.argv`, `process.cwd()` = the game root, `process.mainModule.filename` = `<root>/index.html`, `process.hrtime`/`hrtime.bigint`, `process.nextTick` (as a microtask) and `process.exit`.
  * `navigator.platform` and `navigator.userAgent` must be **consistent** with `process.platform`. `navigator_shim.js:24` hard-codes `"Win32"`, while `storage_shim.js` hard-codes `platform: "linux"`.
  * Keep `Chrome/` in the UA so that rmmz enables WebGL. Use `Chrome/117.0.5938.63` to match the reference [G: confirm that no plugin parses the version].
* **Save locations:** the game's `Advanced_ChangeSaveLocation` (plugins.js: `location: "Appdata-Roaming"`, `dir_save_name: "HIROTONFA_AmericanRevolutionSmuggler"`) computes the directory from `process.platform` and `process.env` [V plugin source]:

| Platform | Directory |
|---|---|
| Linux | `$XDG_DATA_HOME` or `$HOME/.local/share`, plus `/HIROTONFA_AmericanRevolutionSmuggler/` |
| Windows | `%APPDATA%\HIROTONFA_AmericanRevolutionSmuggler\` |
| macOS | `$HOME/Library/Application Support/HIROTONFA_AmericanRevolutionSmuggler/` [V, lines ~100–130 for the Appdata branches] |

  * The host must not override this. It only has to tell the truth.
  * **Safety net:** if the resolved save path is relative, or lies inside the game root, `bind_io` refuses the write and logs an error. No silent fallback into the game folder.
* **Migration of existing saves (copy, never move):**
  * On first run, if the native save dir is empty, look for saves from:
    1. **Proton (Linux/Deck):** `<lib>/steamapps/compatdata/3789080/pfx/drive_c/users/steamuser/AppData/Roaming/HIROTONFA_AmericanRevolutionSmuggler/`, where `<lib>` is derived from the game path itself (`<lib>/steamapps/common/<game folder>`). This is a plain file-system path hint. If it does not exist, nothing happens;
    2. **NW.js on Windows:** `%APPDATA%\HIROTONFA_AmericanRevolutionSmuggler\`, which is already the same dir, so there is nothing to migrate;
    3. **NW.js on macOS:** already the same dir;
    4. **Outsider's relative fallback** from the test drive: `<cwd>/undefined/.local/share/…`.
  * Show a one-time prompt, or use `--import-saves=<path>`. The import copies `file*.rmmzsave`, `global.rmmzsave` and `config.rmmzsave`, verifies that each one inflates (pako format, v0.2 §9.2), and writes a `.imported-from` note.
  * **The app ID 3789080** (former Q3) is used only to build that path hint. smzuggler does not parse Steam's library list, call any Steam API or need the Steam client. If the hint misses, the prompt asks for a folder, or the user passes `--import-saves=<path>`. Proton saves exist on dev-laptop, along with a `steam_autocloud.vdf` [M, read-only listing].
  * **Steam Cloud (Q13, closed in v0.5): a one-time copy.** The Proton folder is never written, and smzuggler's native save dir is not cloud-synced. Re-importing later is a manual `--import-saves`, and it never overwrites a native save that is newer without asking.
* **Finding the game folder (new in v0.5).** The game folder is located **by path or config, never by Steam APIs**:
  1. `--game <path>` on the command line;
  2. otherwise `game_dir = <path>` in `$XDG_CONFIG_HOME/smzuggler-rt/config` (`SDL_GetPrefPath` on Windows and macOS);
  3. otherwise smzuggler stops with a message that explains both options. It does not search Steam libraries.
  * The folder is checked by content (`package.json`, `index.html`, `js/rmmz_core.js`), not by where it lives, so a copy on a USB drive or a read-only mount works the same.
  * The folder name is read from the game's `Advanced_ChangeSaveLocation` `dir_save_name` parameter at run time. It is not hard-coded (§13).
* **`require`:** keep `fs` and `path`. Make `fs` Node-exact for the calls the game makes (`existsSync`, `mkdirSync({recursive})`, `readFileSync`/`writeFileSync` with UTF-8 semantics per v0.2 §9.2, `unlinkSync`, `renameSync`, `readdirSync`, `statSync`).
  * An unknown module should **throw** `Cannot find module`, as Node does, instead of returning `{}` (`storage_shim.js:337`). Plugins feature-detect with try/catch [G: re-scan all 57 plugins for `require(` before flipping this].
* **Root cause:** `src/shims/storage_shim.js:344–361`: `env: {}`, a hard-coded `platform: "linux"`, no `versions` and no `hrtime`.
* **Test:**
  * A save on Linux lands in `$XDG_DATA_HOME/HIROTONFA_AmericanRevolutionSmuggler/file1.rmmzsave`, and its bytes match NW.js under the same replay (v0.2 §9.2).
  * A run under a read-only bind mount of the game folder completes with no EROFS errors.
  * Proton saves are imported and load.
* **Upstream:**
  * Yes for the real `env` and platform, `versions`, consistent navigator and the refuse-in-game-root guard.
  * Migration stays in smzuggler only: it is specific to this game.
  * The config-file lookup for `--game` could be offered upstream as a small generic PR [G].

### 5.4 PIXI: `Circle` and the low-level API
* In real-PIXI mode this whole class of bug disappears: `PIXI.Circle`, `Ellipse`, `Polygon`, `RoundedRectangle`, `Geometry`, `Buffer`, `Shader`, `Program`, `Mesh`, `State`, `renderer.state`, `renderer.geometry`, `renderer.shader`, `renderer.renderTexture`, `renderer.filter`, `renderer.mask`, `renderer.buffer` and `renderer.plugins.extract` all come from the real `pixi.js`.
* The conformance requirement moves to **WebGL2 correctness**, tested with v0.2 §10.2's PIXI suites plus a specific UltraMode7 test.
* **UltraMode7 test:** Map 92 rendered at frames 60/300/600 must be within tolerance of the NW.js reference screenshots captured by Outsider's `tools/evaluator`. In particular, the floor must not be black. Also covers UM7's `Geometry` (UltraMode7.js ~1866/1868), its custom shader and `gl.drawElements`.
* **For Outsider upstream (QuickJS mode),** offered separately:
  * `PIXI.Circle` (and `Ellipse`/`Polygon` if missing), with `clone`/`contains`/`getBounds`/`type`. CGMZ_Core subclasses `Circle`.
  * Clearer stub warnings. The `[PROF]` "missing members" list is a good diagnostic to upstream.
  * Real `Geometry`/`Mesh` support in Outsider's own renderer would need a new mesh draw path in `bind_renderer.c`. That is **not our work** (§8).

### 5.5 Canvas2D text (`fillText`)
* **Observed:** HUD panels (`menu.png`, `map_100.png`) drew without text, while message-window text drew. **Cause unknown [U].** Candidates, in order of likelihood [G]:
  0. **New in v0.4: the HUD is TextPicture** (`TextPicture.js`, 565 `set` commands; HUD fields per the analysis). This copy of the plugin **reuses** the picture's bitmap: it calls `bitmap.clear()` and then redraws the text into the same canvas (`redrawTextToBitmap`), and only creates a new `Bitmap` when the text grows. If Outsider does not re-upload a texture whose canvas was cleared and redrawn (the `baseTexture.update()` / dirty-flag path), the HUD would show the empty first upload, while message windows, which redraw through `Window_Base`, look fine. **Test:** log texture uploads per TextPicture bitmap on the HUD frame. Real PIXI's own `BaseTexture.update` path removes this risk in real-PIXI mode.
  1. A plugin-loaded font that Outsider's `font_manager.c` / stb_truetype fails on. `CatComic.otf` has CFF outlines; also check `FontFace` / `document.fonts` loading paths in `font_shim.js`.
  2. A Canvas2D state that `canvas2d.c` ignores for text: `globalCompositeOperation`, `textBaseline`/`textAlign`, `strokeText` outline then `fillText`, a `font` shorthand parse (`bold 22px "HighSchoolUSA"`), or `letterSpacing`.
  3. Text drawn into a bitmap that is only displayed through a stubbed PIXI path, e.g. a Mesh or a filter. Real PIXI would fix this.
* **Requirement:**
  * `fillText`/`strokeText`/`measureText` support every font the game loads (all 8 files) and the full `font` shorthand.
  * Baseline/align, `globalAlpha`, `globalCompositeOperation` (`source-over`, `copy`, `destination-out`, `lighter`) and `maxWidth` squeeze all work.
  * Results fall within the v0.2 Q7 text tolerance (now **Q7** in §11).
* **Test:**
  * First, instrument `canvas2d_shim.js` to log `{font, op, text.length, canvas size}` for each `fillText` on the HUD frame, and compare against NW.js via the evaluator.
  * Then add a per-font golden test (each font at 3 sizes, with outline) as a C test.
* **Upstream:** yes, whatever the fix turns out to be, unless the cause is #3.

### 5.6 Read-only game folder (new, from reading Outsider)
* **Requirement:** the host never creates or modifies any file under the game root.
* **Root causes found [V]:**
  * the `.jsc` bytecode cache sits next to each script (`src/engine/js_engine.c:437–571`);
  * `debug.log` and `console.log` go into the game dir (`src/main.c:176–184`);
  * the movies need transcoding to `.mpg` (`dom_shim.js:964–988`);
  * the README states "saves are written next to the game".
* **Fix:**
  * Logs go to `$XDG_STATE_HOME/smzuggler-rt/` (or `SDL_GetPrefPath` on Windows, macOS and Android).
  * The code cache goes per §4.1.
  * There is no transcoding.
  * `--game` may be a read-only mount.
* **Test:** CI runs on a read-only bind mount (Linux) and checks the tree hash before and after.
* **Upstream:** yes, as an opt-in `--data-dir` / "read-only game" flag. Upstream may want its default unchanged.

### 5.7 Smaller items found while reading [V]
* `canPlayType` for audio (`dom_shim.js:1193–1197`) says "probably" for mp3, which Outsider's SoLoud build cannot decode [G]. Answer as Chrome 117 does, and make the decoder match.
* Timers and rAF are implemented in JS (`dom_shim.js:229–246`, `setTimeout` at ~1617) and flushed by `game_loop.c`. Keep this, but add a microtask checkpoint after each callback (§4.1).
* `HTMLElement.prototype.querySelector` returns `null` for everything. Implement a simple tag/id/class selector over the subtree.

---

## 6. Performance plan

**Hypothesis H1 [U]:** most of the 120–150 ms Map 92 frame is interpreter time that V8's JIT removes, so the V8 build lands at ≲6 ms CPU per frame, mean.
* **For it [M]:**
  * Chrome takes a 2.57 ms mean (p95 8.1 ms) on the same scene with the same scripts.
  * QuickJS-to-V8 factors were measured at 22–26×, so pure JS on QuickJS predicts ~55–70 ms.
* **Against, or unknown:**
  * The remaining ~50–90 ms is unattributed. It could be Outsider-side work that V8 would *not* speed up:
    * CPU Canvas2D raster;
    * per-frame texture re-uploads;
    * `pixi_shim.js` scene walking (JS, so V8 helps);
    * the HLSL filter path;
    * GC pauses;
    * the spikes of 1.0 and 3.6 s, which look like load, GC or compile stalls.
  * The `js_update` figure includes time spent in native calls made from JS, so "JS-bound" is not proven.

**Order of profiling:**
1. **Outsider as it is now (QuickJS), M0, cheap:**
   * Use `perf record -g --call-graph dwarf` on a frame-pinned control-mode run of Map 92, with symbols, and split the time into:
     * `JS_CallInternal` and the interpreter;
     * `canvas2d.c` / `font_manager.c`;
     * `image_loader`/texture upload;
     * `sprite_batch_gpu.c` / `gpu_backend.c`;
     * the driver.
   * Cross-check against `game_loop.c` `GameLoopStats` per phase (timers / jobs / rAF / swap).
   * This one experiment says how much of H1 is plausible before any V8 work starts.
2. **NW.js reference profile:** a Chrome DevTools CPU profile of Map 92 through `tools/evaluator` (CDP `Profiler.start`), giving the per-function budget and the shape we must match.
3. **smzuggler on V8, M1 (V8 + Outsider renderer):** rerun the identical drive.
   * Use `perf` with **`--perf-prof`** (or `--perf-basic-prof`), so that perf can symbolise JIT frames.
   * Use **`--prof`** with V8's tick processor (`tools/linux-tick-processor` / `--prof-process` from a matching d8 build) for JS-level ticks.
   * Expose **`--cpu-prof[=file]`** in our host via `v8::CpuProfiler`, writing a `.cpuprofile` that DevTools loads. Note that `--cpu-prof` is a *Node* flag, not a V8 flag, so we implement it.
   * Also log `--trace-gc` for the spikes.
   * **Gate G1:** after M1, if Map 92 JS time is >12 ms mean, stop and reassess before committing to M2. M2 adds real PIXI's WebGL2 cost and removes the shim's JS cost.
4. **M2, real PIXI:** frame budget split into JS (target ≈ Chrome's 2.5–3 ms), Canvas2D/text (≲2 ms) and GL submit (≲1.5 ms).
   * Use RenderDoc (Linux/Windows) or an ANGLE trace for GPU-side checks.
   * Add the V8 code cache and Fast API calls only if profiles show they matter.
   * Run the §14 acceptance scenes S1–S3 on dev-laptop on battery with `power-saver`, and on a Deck when one is available. The **p99**, not the mean, is the target.
5. **Jitless measurement:** one run with `--jitless` in M1 to replace the §3.3 guesses.

---

## 7. smzuggler source layout and changes

| Area | Outsider path | Action | New or changed LOC [G] |
|---|---|---|---:|
| Binding seam | — (new) `src/jsb/jsb.h`, `jsb_v8.cc` | new | 0.8–1.2k |
| JS engine | `src/engine/js_engine.c` | rewrite on V8, same header | 0.6–0.9k |
| Game loop, errors | `src/engine/game_loop.c`, `error_handler.c` | port to seam; microtask checkpoints; close event | 0.2–0.4k changed |
| Bindings kept | `src/bindings/bind_{audio,canvas2d,font,image,input,io,platform,video,effekseer}.c` | port to `jsb` | ~2.65k ported |
| Renderer bindings | `bind_renderer.c`, `bind_filters.c`, `bind_tilemap.c` | ported for M1 only, then legacy mode | ~1.0k ported |
| WebGL2 | — (new) `src/webgl/` | new, V8 direct + GLES | 4–7k |
| ANGLE | `third_party/` | new dependency, pinned | build glue |
| Shims | `src/shims/*.js` (11.6k lines) | fixes from §5; `webgl2` branch in `canvas2d_shim.js`; loader skips `pixi_shim`/`tilemap_shim` in real-PIXI mode | 0.5–1.0k |
| Audio | `src/audio/*` | Opus/WebM sources, truncated-Ogg decode, node semantics | 1–2k |
| Video | `src/video/video_player.c` | replace pl_mpeg with nestegg+libvpx+libopus | 1.5–2.5k |
| Text | `src/rendering/canvas2d.c`, `font_manager.c` | fixes from §5.5; FreeType+HarfBuzz if needed | 0.5–2.5k |
| Paths and saves | `src/io/file_io.c`, `bind_io.c`, `src/main.c` | data/cache/state dirs; guard; import | 0.5–0.9k |
| Build | `CMakeLists.txt`, `third_party/CMakeLists.txt` | V8 tarball, ANGLE, libvpx/opus/nestegg; pin SoLoud (`master`) and SDL_shadercross (`main`) | build glue |
| Game profile | — (new) `src/smzuggler/game_profile.c` | the game-specific values in one place: the save-folder parameter name, the Proton path hint (app ID used only there), save import (§13) | 0.1–0.2k |
| Config | — (new) `src/smzuggler/config.c` | `--game` / `game_dir` lookup (§5.3) | 0.1k |
| Frame stats | `game_loop.c`, new `src/smzuggler/frame_stats.c` | §14.3 instrumentation; `--perf-suite` runner and `--scrub-report` for outside testers (§14.8) | 0.5–0.8k |
| Perf tooling | — (new) `tools/perf/` | report analyser, `deck-proxy` scripts (§14.9) | 0.2–0.4k |
| Docs and tooling | `ARCHITECTURE.md`, `docs/`, `tools/privacy/`, `tools/doclint/` | §15, §16 | docs, plus 0.5–1k tooling |
| Tests | `tests/`, `tools/evaluator/` | game scenarios (run against a local install), §14 scenes, read-only CI | 1–2k |

**Total new or changed: ~15–25k lines** [G], versus v0.2's 23–45k. smzuggler inherits about 20k working lines.

---

## 8. Upstream contributions and fork tracking (rewritten in v0.4)

**Decision (owner, 2026-10-30):** general fixes go back to Outsider as **code**. Copyright assignment to General Arcade is accepted **for those patches only**. smzuggler's own code (V8, WebGL2, real-PIXI mode, media, game-specific parts) is not offered upstream and stays GPL-only.

**Upstream's rules [V `CONTRIBUTING.md`]:**
* Outsider is dual-licensed: `GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial`.
* Code contributions require assigning copyright to General Arcade, so that they can keep dual-licensing.
* Shims must stay ES5 and simple, because QuickJS has no JIT.
* New files carry upstream's copyright line and SPDX header. Upstream PRs use **upstream's** header, not smzuggler's.

### 8.1 What goes upstream

Each row is one PR. "Depends" lists the only PRs that must land first. Everything else is independent and can be reviewed in any order.

| PR | Change | Files (upstream tree) | Size [G] | Test included | Depends | Spec |
|---|---|---|---:|---|---|---|
| U1 | `document.getElementsByTagName` also returns `head`, `body` and `html` | `src/shims/dom_shim.js` | <50 lines | shim unit test | — | §5.1 |
| U2 | Element-level `getElementsByTagName`, and `querySelector`/`querySelectorAll` for simple tag/id/class selectors over the subtree | `dom_shim.js` | 100–200 | shim unit test | — | §5.1, §5.7 |
| U3 | `nw.Window.get()` is a minimal EventEmitter (`on`/`once`/`off`/`removeListener`/`removeAllListeners`/`emit`/`listeners`), plus the window methods in §5.2 | `src/shims/storage_shim.js` | 100–150 | shim unit test | — | §5.2 |
| U4 | Deliver the `close` event from SDL quit/window close, honouring `close(true)` and `nw.App.quit()` | `platform_sdl3.c`, `game_loop.c`, `storage_shim.js` | 50–120 | C test plus control-mode test | U3 | §5.2 |
| U5 | Real `process.env` snapshot, real `process.platform`, `process.versions`/`arch`/`hrtime`/`nextTick` | `storage_shim.js`, `bind_platform.c` | 80–150 | shim unit test | — | §5.3 |
| U6 | `navigator.platform` and `userAgent` consistent with the OS | `navigator_shim.js` | <50 | shim unit test | — (U5 helps) | §5.3 |
| U7 | Refuse save writes to a relative path or inside the game root, and log an error | `bind_io.c`, `file_io.c` | 50–100 | C test | — | §5.3 |
| U8 | `PIXI.Circle`, `Ellipse` and `Polygon` (with `clone`/`contains`/`getBounds`/`type`) | `src/shims/pixi_shim.js` | 100–200 | shim unit test | — | §5.4 |
| U9 | Opt-in `--data-dir`: logs go to the state dir instead of the game dir | `src/main.c` | 50–100 | C test | — | §5.6 |
| U10 | `.jsc` cache goes to the cache dir when `--data-dir` is set | `js_engine.c` | 50–100 | C test | U9 | §5.6 |
| U11 | `canPlayType` answers match what the build can decode | `dom_shim.js` | <30 | shim unit test | — | §5.7 |
| U12 | Pin SoLoud and SDL_shadercross to fixed revisions | `third_party/CMakeLists.txt` | <20 | CI build | — | App. A |
| U13 | The Canvas2D text fix, once root-caused, if the cause is in Outsider's code | `canvas2d.c` / `font_manager.c` / shims | unknown | golden test | — | §5.5 |
| U14 | Optional: list missing members as clearer `[PROF]` warnings | `pixi_shim.js` | <80 | — | — | §5.4 |
| Issue only | UltraMode7 needs `Geometry`/`Buffer`/`Shader`/`Mesh` in Outsider's renderer. We file a bug report describing what is missing and let upstream decide. We do not write it (§5.4) | — | — | — | — | §5.4 |

**Never upstream:** the V8 engine and the `jsb` seam, WebGL2 and ANGLE, real-PIXI mode, libvpx/Opus media, WebAudio node semantics (unless upstream asks), FreeType/HarfBuzz (could be offered later as its own proposal), save migration, game-specific scenarios, the privacy scan's deny-list (§16), and anything that names the game.

### 8.2 How each upstream PR is built
* **Upstream-first.** Each fix is written on a topic branch `up/<id>-<slug>` cut from the **current `upstream/main`**, never from smzuggler's `main`. That way the PR contains no smzuggler code, and it applies cleanly upstream.
* **Small and independent.** One concern per PR, ideally under ~200 changed lines, with its own test, its own explanation and no drive-by refactors or reformatting. Where a fix needs two steps (U3 → U4, U9 → U10), the second PR says so and is opened only after the first lands.
* **Upstream's style.** ES5 for shims, upstream's C style and header, and nothing that assumes V8 or real PIXI. Each PR is built and tested with **QuickJS** in upstream's own CI before it is opened.
* **Explain the why.** The PR description gives the observed failure (for example "Mimosa_MouseCursor gets `styleParent` undefined"), the root cause with its file and line, the fix, how it was tested, and the reference behaviour (NW.js/Chrome/Node, with a link to MDN or the Node docs). It mentions only plugin names and public behaviour, never the game's files.
* **Identity and assignment.** Commits are authored as the project identity in §16. The assignment statement that `CONTRIBUTING.md` asks for is made once by the owner under that identity, in the form upstream requires (Q12).
* **Tracking.** `docs/UPSTREAM.md` in smzuggler has one row per PR: id, upstream PR link, status (draft / open / merged / rejected / withdrawn), the upstream merge commit, and the smzuggler commit that carries it until then.

### 8.3 How smzuggler tracks upstream
* **Branches.**
  * `upstream-main` is a read-only mirror of `General-Arcade/outsider` `main`. It is fast-forward only and nobody commits to it.
  * `main` is smzuggler. It is public, so **its history is never rebased or force-pushed**.
  * `up/*` are upstream PR topic branches (§8.2).
* **Merge, not rebase.** Upstream comes into smzuggler with a normal merge commit, `Merge upstream Outsider <short-sha>`. That keeps smzuggler's public history stable and makes each upstream sync one reviewable unit. Conflicts are resolved in the merge commit, and the resolution is written up in `docs/UPSTREAM.md` under that sync.
* **Cadence.** Merge upstream every month, whenever one of our PRs lands, and before every milestone exit. Each sync runs the full test suite, including the read-only check and the §14 performance smoke test.
* **While a PR is pending.** smzuggler carries the fix as a cherry-pick (`git cherry-pick -x`) from the `up/*` branch, so the commits are identical. When upstream merges it, the next sync brings in the same change, and the carried commit drops out with no conflict, or a trivial one. If upstream changes the patch during review, smzuggler takes upstream's version at the next sync.
* **If upstream says no.** The patch stays in smzuggler, marked "carried" in `docs/UPSTREAM.md` with upstream's reason, and is rechecked at each sync.
* **Keeping syncs cheap.**
  * smzuggler-only code lives in **new directories** (`src/jsb/`, `src/webgl/`, `src/media/`, `src/smzuggler/`), so it rarely conflicts.
  * Edits to upstream files are kept minimal and grouped. Each block has a one-line `/* smzuggler: why */` comment, so a person resolving a conflict knows what the change is for.
  * The binding port to `jsb` (§4.1) touches every `bind_*.c` file. That is the main conflict surface. `docs/UPSTREAM.md` keeps a checklist for re-applying upstream binding changes through the seam by hand.
  * Upstream's QuickJS mode is not kept in smzuggler (V8 only). A sync that changes `js_engine.c` is re-done against our V8 version, and the merge commit explains what was done.
* **Effort.** About 0.5–1.5 pw over the project for splitting, upstream review rounds and syncs (included in §9).

---

## 9. Milestones and estimate (revised in v0.5)

Every milestone includes its documentation (§15), and it does not exit until those docs pass the review gate. "Docs" in the table lists what each milestone has to deliver. The estimate includes the doc work, the 60 fps acceptance work (§14) and the upstream PR work (§8).

| Milestone | Content | Docs delivered | Exit criteria | v0.4 [G] | v0.5.1 [G] |
|---|---|---|---|---:|---:|
| **M0 Fork + baseline** | Fork at f789f46 as smzuggler. CI on Linux/Windows/macOS. Privacy scan and commit-identity check (§16). Doc lint and the docs-touched gate (§15.7). Game scenario in `tools/evaluator` (reference captures from NW.js, local install only). §5.1–5.3 and §5.6 fixes in QuickJS mode, opened upstream as U1–U12 (§8). **perf attribution of the Map 92 frame (§6.1).** Measure binary size | `ARCHITECTURE.md` (first version), `CONTRIBUTING.md`, `docs/style.md`, `docs/UPSTREAM.md`, `docs/privacy.md`, the compat-surface table seeded from the existing shims, headers on every file we touch | smzuggler (QuickJS mode) boots the game to the map with no compat script, no game-dir writes and a correct save dir; profile report written; all CI gates green | 2–3.5 | **2–3.5** |
| **M1 V8 swap** | V8 CI tarball; `jsb` seam; `js_engine` on V8; all 12 bindings ported; microtasks; code cache; `--cpu-prof`; jitless run | `docs/js-binding.md`, including the V8 handle and lifetime rules (§15.8); doc comments on every function in `jsb` and the bindings | Same drive as §2 on V8 with Outsider's renderer; Map 92 JS time measured; **Gate G1** (§6) | 6–10.5 | **6–10.5** |
| **M2 Real PIXI** | WebGL2 over GLES 3.0; ANGLE build on Windows/macOS (plus Linux CI oracle); load the game's `pixi.js`; drop `pixi_shim`/`tilemap_shim` in this mode; frame-timing instrumentation and the §14 harness; outside-tester kit (`--perf-suite`, `--scrub-report`, §14.8), the fixture-save recipe (§14.10) and the optional `deck-proxy` scripts (§14.9) | `docs/webgl.md`, including the GL object lifetime rules (§15.8); compat rows for every WebGL entry point | Title, Map 92 (Mode 7 floor correct) and menu within tolerance; no WebGL errors over a 10-minute walk; **§14 acceptance for title, Map 92 and menus passes on dev-laptop on battery with `power-saver`** (the operative gate). A Deck run is added if a friend's Deck is available (confirmation, not required here) | 9.5–16.5 | **9.75–17** |
| **M3 Text and Canvas2D parity** | §5.5 root cause and fix; per-font goldens; FreeType/HarfBuzz if needed; upstream PR U13 if it applies | `docs/canvas2d-text.md` | HUD text matches the reference within Q7 tolerance | 3.5–6.5 | **3.5–6.5** |
| **M4 Audio** | Opus/WebM, truncated Ogg, node semantics on SoLoud | `docs/audio.md` | Audio conformance suite (v0.2 §10.2) passes; `police radio` plays | 3–6.5 | **3–6.5** |
| **M5 Saves and migration** | §5.3 paths per OS, import tool, byte-identical saves | `docs/saves.md` | Save diff identical to NW.js; Proton import verified | 1.25–2.5 | **1.25–2.5** |
| **M6 Video and full plugin coverage** | libvpx video; Effekseer wasm; all 57 plugins exercised along the coverage route | `docs/video.md`; `docs/shims.md` and the compat table complete; `docs/adding-an-api.md` with two worked examples | Both movies play; replay corpus passes; **§14 acceptance for the trade screen, naval combat, video and save/load passes on dev-laptop on battery with `power-saver`** (the operative gate). Deck confirmation as for M2 | 5.5–11 | **5.5–11** |
| **M7 Platforms, packaging, 60 fps sign-off** | Steam Deck (sniper runtime), Linux arm64, Windows, macOS signing (JIT entitlement), AppImage/Flatpak. A doc note on adding the binary to Steam as a non-Steam game (documentation only, no Steamworks). Performance tuning found by §14 on the Deck and on dev-laptop on battery with `power-saver` (the larger tuning reserve, §14.2) | `docs/build.md`, `docs/testing.md`, release notes | Packaged builds pass the smoke suite on each primary and secondary target; **the full §14 suite, including the 30-minute soak, passes on dev-laptop on battery with `power-saver`, and the Deck confirmation pass (§14.8) succeeds on a Steam Deck or Deck-equivalent. No release without it** | 4–8 | **5.25–10.5** |
| **D Inherited code documentation** | Bring the ~20k inherited Outsider lines up to §15: file headers, function comments, module READMEs. Done alongside M0–M2, subsystem by subsystem | the module READMEs | §15 lint passes on the whole tree | 1–2 | **1–2** |
| **U Upstream PRs and syncs** | Splitting U1–U14 into upstream-ready PRs, review rounds, monthly syncs (§8) | `docs/UPSTREAM.md` kept current | every U-PR merged, rejected (and carried) or withdrawn | 0.5–1.5 | **0.5–1.5** |
| **Total** | | | | **36.25–68.5** | **37.75–71.5 ≈ 38–72 pw** |
| *MV support (later, §18)* | *not in the total* | | | — | *≈ 10–20 pw* |

**Where the v0.5.1 change of −0.25 pw comes from [G]:**
* **Gate G0 dropped: −0.25 pw** in M0. The owner does not need a stock-NW.js measurement; the laptop gate stands on its own (§14.2).
* **Laptop gate operative, Deck as confirmation: no change.** The Deck run still happens before release, so the tester kit stays. Only its timing moves.
* **Fixture saves (§14.10): no change.** The recipe and slot inventory are part of the §14 harness work already in M2.

**Where the v0.5 change of +1.75–3.25 pw came from [G]:**
* **dev-laptop on battery with `power-saver` (§14.2): about +1–2 pw** of extra tuning reserve in M7. With EPP `power`, the CPU runs short frame bursts at lower clocks, which hurts p99 more than the mean (§14.2 has the measurement).
* **Gate G0 (§14.2): about +0.25 pw** in M0 (dropped again in v0.5.1).
* **Outside-tester kit and Deck proxy (§14.8, §14.9): about +0.5–1 pw**, split across M2 (first use) and M7 (release-ready kit and tester docs).
* **Rename to smzuggler: none.** The names are build constants (§13).
* **No Steam integration: none.** The v0.4 plan had no Steamworks either. Dropping the "Steam launch option" packaging item in M7 and the library-list parsing in M5 saves a day or two at most, which is inside the rounding.
* **MV support: not in the total** (§18, ≈ 10–20 pw, later).

**Where the v0.4 increase of +8–14 pw over v0.3 came from [G]:**
* **Documentation and readability (§15): about +5–9 pw**, i.e. about 15–20% on top of the code work.
* **60 fps on two specific devices (§14): about +2–4 pw.** That covers the harness, device runs at M2/M6/M7, and a tuning reserve for p99 spikes (GC, texture uploads, text rasterisation) and the laptop's 2256×1504 output.
* **Upstream PRs (§8): about +0.5–1.5 pw.**
* **Privacy scan (§16): about +0.25 pw**, inside M0.

**Re-check of the estimate.**
* The milestone rows sum to 37.75–71.5 pw: lows 2 + 6 + 9.75 + 3.5 + 3 + 1.25 + 5.5 + 5.25 + 1 + 0.5 = 37.75; highs 3.5 + 10.5 + 17 + 6.5 + 6.5 + 2.5 + 11 + 10.5 + 2 + 1.5 = 71.5.
* Against v0.4's 36.25–68.5 the net change is +1.5–3 (M0 0/0, M2 +0.25/+0.5, M7 +1.25/+2.5), which matches the v0.5 drivers minus G0.
* The high end still assumes no surprise from Gate G1. If G1 fails (Map 92 JS >12 ms mean on V8), the plan is reassessed before M2.
* **Risk carried by the Deck assumption [G]:** until the first Deck confirmation run, a Deck-only problem (RADV/radeonsi driver behaviour, gamescope pacing, the slower Zen 2 cores at p99) would surface late, in M7. It is covered by the existing M7 tuning reserve; if a Deck turns up earlier, running it at M2 or M6 shrinks that risk.

**Calendar for one experienced C/C++/graphics developer:**
* ~9–17 months to M7;
* a V8-running drive after ~2–3.5 months (M0+M1);
* a correct Mode 7 map that passes the 60 fps test for title, map and menus on dev-laptop on battery after ~4–7.5 months (M0–M2).

**Closed: the shim alternative (former Q2).** Keeping Outsider's PIXI shim on V8 and adding mesh support to it is no longer considered. smzuggler runs real, unmodified PIXI 5.3.12.

---

## 10. Conformance testing (deltas from v0.2 §10)
* **Harness:**
  * Use Outsider's `tools/evaluator/` as is: CDP driver for NW.js, scenarios, compare, report.
  * Add `--control` scenarios for our host.
  * Reference captures are made from the user's local NW.js install and are never committed.
* **New game scenarios from the test drive:**
  * boot → title;
  * new game → Map 92 frames 60/300/600;
  * open the menu → screenshot;
  * save to slot 1 → reload;
  * window close → clean exit;
  * a HUD text frame.
* **Read-only CI:** the game is mounted read-only, and the tree hash is checked before and after.
* **Performance CI:** S1–S3 (§14.6) frame times are recorded per commit on a fixed runner. Fail on a >20% regression of p99. Device acceptance on dev-laptop on battery with `power-saver` is a manual gate at M2, M6 and M7; the Deck confirmation pass is required at M7, before release (§14.7).

---

## 11. Open questions (revised in v0.5.1)

**Closed in v0.5.1:**
* **Q20 Laptop gate if G0 fails: closed.** Gate G0 is dropped. The owner does not care how stock NW.js does; the laptop gate stands on its own (§14.2).
* **Q14 Test fixtures for §14: answered.** Three fixture saves: Map 92, a port next to the store (trade screen) and the sea next to an encounter (naval combat). They stay local, are never committed or shared, and outside testers create their own from the recipe (§14.10). The owner chose the slots (`file12`, `file4`, `file6`, §14.10).

**Closed in v0.5:**
* **Q13 Steam Cloud: closed.** A one-time copy is fine (§5.3). The Proton folder is never written. Native saves are not cloud-synced.
* **Q15 Laptop power state: closed.** The dev-laptop gate runs on battery with `power-saver`. AC with `balanced` is an extra report (§14.4).
* **Q16 Steam Deck model and availability: closed as a procedure.** No Deck is owned yet. The gate runs on a borrowed Deck or with an outside tester, recorded as a Deck-equivalent, with the model (LCD 60 Hz or OLED run at 60 Hz) taken from that device (§14.8). Until then, `deck-proxy` runs on dev-laptop are reported as a proxy, never as a pass (§14.9). Who and when is Q19.
* **Q17 Names: closed.** Project and repository `smzuggler`, binary `smzuggler-rt` (§13).
* **v0.2 Q8 Steam integration: closed as "no Steam integration for now".** No Steamworks SDK, no Steam API calls, no Steam client needed. The game folder is found by `--game` or the config file (§5.3).

**Closed in v0.4:**
* **Q1 Upstreaming: closed.** Code PRs, with copyright assignment for those patches (§8).
* **Q2 Real PIXI vs. the shim: closed.** Real, unmodified PIXI 5.3.12 (§9).
* **Q3 Steam app ID: closed.** The app ID is **3789080**. Since v0.5 it is used only as a path hint for the Proton save import (§5.3).
* **Q4 Platform priority: closed.** Steam Deck and dev-laptop are must-pass 60 fps targets (§14). Since v0.5.1 the laptop gate signs off M2, M6 and M7, and the Deck confirmation pass is required before release. Android is deferred (§17).
* **Q11 Fork name: closed** (renamed in v0.5, see Q17).
* **v0.2 Q10 battles: closed.** The game has 0 "Battle Processing" commands. Combat is a picture-based naval dice duel (CE400–402), which §14 tests as "naval combat".

**Still open:**
* **Q5 ANGLE scope.** ANGLE everywhere (one code path, Chrome-exact, larger binary), or system GL on Linux/Deck with ANGLE on Windows/macOS and as the CI oracle? The 60 fps targets are both Linux/Mesa, so system GL on Linux is what gets measured first.
* **Q6 PNG colour chunks** (from v0.2 Q5).
* **Q7 Text tolerance** (from v0.2 Q7).
* **Q8 ICU data size** (from v0.2 Q12).
* **Q9 V8 version pin** (from v0.2 Q11).
* **Q10 Effekseer:** the game's wasm on V8 (default), or Outsider's native SDK option?
* **Q12 Assignment mechanics.** In what form does upstream want the copyright assignment (a statement in the PR, a signed form, a CLA bot)? Does it accept it from the project identity in §16?
* **Q18 (new) MV test corpus.** No MV game is installed locally (§18.1), so MV work cannot start until the owner picks 3–5 MV games from their own library (ideally one plain, one Yanfly-heavy, one with encrypted assets and one with video).
* **Q19 Deck access: timing only.** The owner has friends with early Decks and will say when one is available. Until then the laptop gate is the operative gate (§14). Still to settle when it happens: the date, the model and how the scrubbed report comes back (§14.8). The tester's name never goes into the project; reports say "outside tester".
* Still open from v0.2: Q2 (Emscripten environment) and Q9 (coverage route).

---

## 12. Licensing
* **Project license:** GPL-3.0-or-later. We receive Outsider under its GPL-3.0-or-later option. We cannot sublicense General Arcade's commercial option, and our modifications are GPL-only.
* **File headers:**
  * Keep each Outsider file's copyright line and SPDX expression.
  * Add our own copyright line, `Copyright (C) 2026 The smzuggler contributors` (no personal names, §16), to files we change, and mark them as modified, with a date (GPLv3 §5a).
  * For combined files as we distribute them, the effective license is GPL-3.0-or-later. Record that in a `LICENSES/README` [G, not legal advice: have this reviewed].
  * New files use `SPDX-License-Identifier: GPL-3.0-or-later` and our copyright line. Do not add the GeneralArcade commercial `LicenseRef`, which is theirs to grant.
* **Dependencies:** v0.2 §11.1 still applies (V8 BSD-3, ICU, SDL3 Zlib, ANGLE BSD-3, glslang/SPIRV-Cross, FreeType FTL, HarfBuzz MIT, stb, libvorbis/libogg BSD-3, libopus BSD-3, nestegg ISC, libvpx BSD-3 + patent grant, zlib). Changes:
  * **Added (kept from Outsider):**
    * SoLoud (Zlib/libpng, GPL-compatible) [G: confirm the exact licence of the pinned revision];
    * SDL_shadercross (Zlib), only in legacy-renderer builds;
    * Effekseer SDK (MIT), only in the native option.
  * **Removed:**
    * QuickJS-ng (MIT);
    * pl_mpeg (MIT);
    * miniaudio, which v0.2 planned and smzuggler does not need unless SoLoud fails (§4.3).
* **Upstream PRs** (§8): the code in them is assigned to General Arcade, carries upstream's header, and comes back into smzuggler through upstream syncs under upstream's licence expression. smzuggler-only code is never included in them.
* **Game content:** none in any repo. That covers scripts, plugins, images, audio, fonts, save files and reference screenshots. Tests read from a local install path passed at run time.
* **Unity wrapper:** it is proprietary prior art. Nothing is copied from it, including code, shaders and data. Only its publicly observable architecture may be described.
* **Distribution:** source accompanies binaries (GPL-3). There is no Steam distribution or Steamworks for now (§5.3). If that changes, shipping a GPL launcher on Steam is generally fine [G, not legal advice], but Steamworks SDK linking would need its own licence review. GPL is generally incompatible with console SDK and App Store terms [G, not legal advice], which is another reason iOS and consoles are out of scope.

---

## 13. Naming (revised in v0.5)

* **Project name: smzuggler** (Q17, closed). It is a pun on **MZ**, the RPG Maker version this runtime targets, and it is always written in lower case, also at the start of a sentence. It is used in prose, docs and the window title.
* **Repository: `smzuggler`.** **Binary: `smzuggler-rt`.** The binary name is also the name of the data directories: `$XDG_DATA_HOME/smzuggler-rt/`, `$XDG_STATE_HOME/smzuggler-rt/`, `$XDG_CACHE_HOME/smzuggler-rt/`, `$XDG_CONFIG_HOME/smzuggler-rt/`, or `SDL_GetPrefPath("", "smzuggler-rt")`. The `-rt` suffix makes it clear that this is a runtime, not the game.
* **Not affiliated.** smzuggler is an independent, open-source runtime for RPG Maker MZ games. It is distinct from *American Revolution Smuggler* and is not affiliated with or endorsed by HIROTONFA, by General Arcade (the Outsider authors), or by the makers of RPG Maker. The README and the About text say this in their first paragraph.
* **Avoid implying the game's trademark** [G, not legal advice]:
  * Repo names, binary names, package IDs, icons, store text and file names never use "American Revolution", "HIROTONFA", the game's logo, its art or a shortened form of its title.
  * smzuggler ships no game art. Its icon is its own.
  * The README may name the game only in a plain compatibility statement ("tested with American Revolution Smuggler; you need your own copy").
  * The v0.4 residual risk, that the project name was the last word of the game's title, is much smaller now: "smzuggler" is a different, invented word whose visible point is "MZ". The name is still a build constant (`SMZUGGLER_NAME` / `smzuggler-rt`), so it can change again without touching code.
  * "RPG Maker" and "MZ" are the RPG Maker makers' names. smzuggler uses them only descriptively ("runs RPG Maker MZ games"), never as its own brand [G, not legal advice].
* **The game's own names in code.** The game's save folder name (`HIROTONFA_AmericanRevolutionSmuggler`) is **not hard-coded**. smzuggler reads `dir_save_name` from the game's `plugins.js` parameters at run time (§5.3). Game-specific values like this, and the app ID used for the Proton path hint, live in one documented file, `src/smzuggler/game_profile.c`, never spread through the code.
* **Upstream's name.** Outsider is General Arcade's project. smzuggler says "based on Outsider by General Arcade" in its README and keeps their copyright lines (§12), but does not use Outsider's name or branding for its own binary.

---

## 14. Target profiles and the 60 fps acceptance test (new in v0.4)

**Requirement:** 60 fps is **must-pass** on both target profiles below, but they gate at different times (changed in v0.5.1):
* **dev-laptop on battery with `power-saver` is the operative gate.** A milestone that lists §14 in its exit criteria (M2, M6, M7) does not exit until the listed scenes pass there.
* **Working assumption [G]:** until a Deck is available, passing on dev-laptop on battery with `power-saver` is taken to mean the Deck passes too. On CPU-bound p99 the two are now about equally tight (§14.2), which is the basis for the assumption. It is an assumption, not a measurement, and the Deck run is what confirms it.
* **Steam Deck is a confirmation pass.** It runs on a friend's Deck when one is available (§14.8), at any milestone, and is **required before release (M7)**. If a Deck run fails at M2 or M6, the failure is fixed before that milestone closes, just as a laptop failure would be.

### 14.1 Target profiles

**`steam-deck`** (from public specifications [G]):

| Item | Value |
|---|---|
| CPU | AMD Zen 2 APU, 4 cores / 8 threads, 2.4–3.5 GHz |
| GPU | AMD RDNA 2, 8 CUs, up to 1.6 GHz; Mesa (RADV / radeonsi) |
| Memory | 16 GB LPDDR5, shared with the GPU |
| Display | 1280×800. LCD model 60 Hz; OLED model up to 90 Hz (tests run at 60 Hz) |
| OS | SteamOS 3, gamescope session; smzuggler built against the Steam Linux Runtime 3.0 "sniper" SDK (build container only) |
| Power | default TDP, no manual frame limit |
| Device | **none owned yet.** Confirmation runs use a friend's Deck (borrowed or run by them), recorded as a **Deck-equivalent** (§14.8). Optional `deck-proxy` runs on dev-laptop are reported as a proxy, never as a pass (§14.9) |

**`dev-laptop`** (the owner's laptop, anonymised; measured on 2026-10-30 [M]):

| Item | Value |
|---|---|
| CPU | Intel Core i5-1340P (13th gen, Raptor Lake-P): 12 cores (4 performance + 8 efficiency), 16 threads, up to 4.6 GHz, x86-64 |
| GPU | Intel Iris Xe (integrated, RPL-P); Mesa 26.2.5: OpenGL 4.6 core and OpenGL ES 3.2 (iris), Vulkan 1.4 (ANV) |
| Memory | 32 GB (31 GiB usable), shared with the GPU. At the time of the check, 22 GiB was in use and 4 GiB of swap was full, so tests must start with memory headroom (§14.4) |
| Display | 2256×1504 (3:2), 59.93 Hz |
| OS | Arch Linux (rolling), Linux 7.2 (zen kernel), Wayland session |
| Power | **gating runs on battery with the `power-saver` profile** (Q15, closed in v0.5). AC with `balanced` is an extra report only. Measured in `power-saver` on battery [M, read-only sysfs, 2026-10-30]: intel_pstate active, turbo **not** disabled (`no_turbo` 0, `max_perf_pct` 100), max 4.6 GHz on P-cores and 3.4 GHz on E-cores, **EPP `power` on every CPU**, RAPL PL1 30 W / PL2 64 W, Iris Xe max 1450 MHz (= RP0, not capped) |

Profile names are the only identifiers used in code, test reports and logs. They never include a host name, a user name or the name of the laptop maker (§16).

### 14.2 Is 60 fps plausible? (re-assessed in v0.5) [G, with the evidence it rests on]
* **For:**
  * Chrome runs the Map 92 frame at a 2.57 ms mean and 8.1 ms p95 [M, box]. Note that this was measured on the box, not on either target.
  * Real PIXI 5.3.12 on V8 costs 0.56 ms per frame at 116 filters in our bench [M, box].
  * The game renders at **1280×720** (`System.json` `screenWidth`/`screenHeight` [V]). That is close to the Deck's 1280×800, so the Deck's GPU load is about what Chrome handles.
  * On battery with `power-saver`, dev-laptop's hardware limits are **not** lowered: turbo stays enabled, `max_perf_pct` is 100, the P-cores can still reach 4.6 GHz, and the Iris Xe can still reach its 1450 MHz maximum [M, read-only sysfs].
* **What `power-saver` changes, and why it matters here [M + G]:**
  * The profile sets the energy/performance preference (EPP) to `power` on every CPU [M]. The hardware (HWP) then prefers low clocks and ramps up slowly. Thread placement on this hybrid CPU is also likely to favour the efficiency cores for short, light work [G].
  * A game frame is exactly that kind of work: a few milliseconds of busy main thread, then idle until the next vblank. The CPU rarely runs long enough to ramp up.
  * **Measurement on dev-laptop, on battery with `power-saver` [M, 2026-10-30, a Python CPU micro-benchmark, not the game]:** the same fixed piece of work took 2.66 ms mean / 3.89 ms p99 / 4.24 ms max when run back to back, but **3.77 ms mean / 5.75 ms p99 / 8.27 ms max** when run once per 1/60 s frame (16% duty). That is about **+42% mean, +48% p99 and about 2× the worst case**, purely from the power state and the frame rhythm. At about 52% duty the penalty fell to about +9% mean and +21% p99.
  * Not yet measured: the same run on `balanced` or on AC (the owner's settings were not changed), the GPU side, and the real game.
* **Risks:**
  1. **p99, not the mean.** Chrome's p95 is already 8.1 ms on the box. GC pauses, texture uploads (TextPicture redraws, toasts) and first-use shader compiles are what break p99 < 16.7 ms. On battery with `power-saver` each of these spikes may be ~1.5–2× longer (above).
  2. **The Deck CPU** is perhaps 1.5–2× slower than the box for JS [G], which turns 2.6 ms into ~4–5 ms. That is fine on average and tight at p99.
  3. **dev-laptop on `power-saver`** may now be about as tight as the Deck, or tighter, at p99 for CPU spikes [G]. In v0.4 the laptop was the one with headroom, because it gated on AC with `balanced`. That is no longer true.
  4. **dev-laptop's output size.** Fullscreen scales 1280×720 up to 2256×1504, which is 3.7× the pixels in the final pass. Filters run at game resolution, so only the last blit is bigger. If the GPU also clocks low under `power-saver`, that blit is where it shows.
  5. **Memory pressure** on dev-laptop (full swap at check time) can cause page-fault stalls.
  6. **Wayland/gamescope frame pacing** at 59.93 Hz: present with FIFO vsync and drive the game's own timing from the presentation clock, not from a 60.000 Hz timer.
  7. **Efficiency cores.** If the main thread lands on an efficiency core it can be ~2× slower [G]. Measure first. Pinning the main thread to performance cores is an experiment, off by default, kept only if the data says it helps on battery without costing battery life.
* **What smzuggler may and may not do about it:**
  * **May:** cut per-frame variance. That means pre-warming shaders and textures during scene transitions, spreading texture uploads across frames, running V8 idle-time GC in the vblank slack (`Isolate::IdleNotificationDeadline` / `MemoryPressureNotification`), and avoiding allocation in hot paths.
  * **May not:** change the power profile, EPP or frequency limits; busy-wait or spin to keep clocks up; or use a background thread to fake load. These would defeat the point of battery mode, and most need root.
* **No stock-NW.js gate (changed in v0.5.1):** v0.5 had a Gate G0 measuring stock NW.js 0.80 on dev-laptop in this power state. It is dropped. The laptop gate is a target for smzuggler in its own right, and it stands whatever NW.js does there. Comparing with NW.js stays possible as an ad-hoc debugging aid, but it is not scheduled, not budgeted and never changes the gate.
* **Verdict [G]:**
  * **Steam Deck:** likely, **if** Gate G1 passes and the laptop gate passes. It is tight at p99. This is the reasoning behind the working assumption in §14.
  * **dev-laptop on battery with `power-saver`:** **plausible, but no longer likely by default.** The mean has plenty of room (a 2.6 ms Chrome frame at ~1.5× is still about 4 ms). The p99 criterion is at real risk on Map 92 with UltraMode7 (S2) and on the trade screen (S4). Passing will probably need the variance work above, and that is why the tuning reserve grows by +1–2 pw (§9).
  * The AC/`balanced` extra report should pass comfortably if the battery run passes. It is kept so that a regression can be told apart from a power-state effect.

### 14.3 What is measured
* **Instrumentation in smzuggler, always compiled in** (enabled with `--frame-stats=<file>`):
  * **Frame work time:** CPU time from the start of the frame (timers, rAF, game update and render) to the swap call, plus GPU time for the frame from GL timer queries (`GL_TIME_ELAPSED`, or `EXT_disjoint_timer_query` on GLES). This is the number the budget applies to, because with vsync on the present interval is pinned to the display.
  * **Present interval:** time between successive presents, from the presentation timestamp where the platform has one, or from the swap return otherwise.
  * **Missed vblanks:** present intervals longer than 1.5× the refresh period.
  * **Markers:** scene changes (`SceneManager.goto`, map transfer) and save/load write marker events, so the analyser can exclude transition windows.
  * Output: JSON Lines with one record per frame, plus a summary of mean, p50, p95, p99, p99.9, max and the count of frames over budget.
* **Budget:** one refresh period. That is 16.7 ms on the Deck at 60 Hz, and 16.69 ms on dev-laptop at 59.93 Hz.

### 14.4 How a run is done
* **Build:** release build, V8 JIT on, default graphics backend for the platform (system Mesa GL/GLES on both profiles), vsync on.
* **Display:** fullscreen at native resolution (Deck 1280×800, dev-laptop 2256×1504; the `deck-proxy` runs at 1280×800, §14.9). dev-laptop is also run once windowed at 1280×720 (reported only).
* **Machine state:**
  * **dev-laptop (gating):** on battery, unplugged for at least 2 minutes, `powerprofilesctl get` = `power-saver`, battery at least 50% at the start, other apps closed, at least 4 GB of RAM free and swap not full. The report records the power source, the profile, the battery level at start and end, and the EPP value read from sysfs.
  * **dev-laptop (extra report):** the same suite on AC with `balanced`. It is reported next to the gating run, never instead of it.
  * smzuggler never changes the power profile, EPP or CPU frequency limits itself, and never busy-waits to keep clocks up. Battery life is part of the goal.
  * **Deck:** game mode (gamescope) at default TDP, launched as a non-Steam game, or in desktop mode with a note in the report. Game mode is what gates.
* **Inputs:** scripted with control-mode input replay, so every run walks the same route.
* **Timing:** each scene gets **10 s warm-up** (JIT and shader compile, not scored) followed by **120 s scored**. The run is repeated **3 times**, and the **worst** of the three counts.
* **Soak:** one **30-minute** scored run looping Map 92 → menu → trade screen → Map 92.

### 14.5 Pass criteria (each scene, each profile)
1. **p99 frame work time < 16.7 ms** (< 16.69 ms on dev-laptop).
2. **p99.9 frame work time < 25 ms.**
3. **Missed vblanks ≤ 1%** of presented frames in the scored window.
4. **No frame over 50 ms** outside excluded windows.
5. **Excluded windows** (scene change or transfer until 30 frames after the new scene's first frame; save or load) are scored separately: a transition takes ≤ 500 ms, a save ≤ 150 ms and a load ≤ 250 ms.
6. **Soak:** criteria 1–4 hold over the full 30 minutes, **and** the p99 of the last 5 minutes is no more than 10% worse than the first 5 minutes. Resident memory also grows by less than 64 MB after the first 5 minutes. Together these catch leaks and GC creep.

### 14.6 Scenes

| # | Scene | How it is reached | Gating at |
|---|---|---|---|
| S1 | **Title** (MOG title layers and commands, idle and cursor moves) | boot | M2 |
| S2 | **Start Map 92 with UltraMode7**: scripted walk loop, camera zoom, lighting and fog | fixture `fx-map92` (§14.10); a new game also starts here | M2 |
| S3 | **Menus**: main menu, then 10 s in each subscene (items, quests, encyclopedia, reputations, mailbox, options, save screen) | from S2 | M2 |
| S4 | **Trade screen** (picture and TextPicture UI, CE473/486): cursor through all 12 goods, then buy and sell | fixture `fx-trade` (§14.10), then the store door | M6 |
| S5 | **Naval combat** (picture dice duel, CE400–402): one full encounter. The game has **no engine battles** (0 Battle Processing), so this is its "battle" | fixture `fx-combat` (§14.10), then the encounter event | M6 |
| S6 | **Dialogue**: message windows with busts, choices and toasts | opening sequence | M6 |
| S7 | **Video**: both movies. The movie keeps its own frame rate with A/V sync within ±45 ms, and UI frames stay within criteria 1–4 | scripted | M6 |
| S8 | **Save and load**: save to a slot, then load it, 5 times. Criterion 5 applies | from S2 | M6 |
| S9 | **Soak** (§14.4) | from S2 | M7 |

### 14.7 Reporting and CI
* Each device run writes `perf-report-<profile>-<scene>.json` and a short Markdown summary. Reports contain profile names only, never host names, user names or home paths. They go through the same scan as commits (§16).
* Each report has a `gate` field: `pass`/`fail` for a laptop gating run, `confirm-pass`/`confirm-fail` for a Deck confirmation run, `extra` for the AC/`balanced` laptop run, or **`proxy`** for a `deck-proxy` run (§14.9). The analyser marks the Deck confirmation as done only from a `steam-deck` report.
* **CI** cannot hold the real devices, so it runs S1–S3 on a fixed Linux runner and fails on a >20% regression of p99 (this replaces the earlier mean/p95 rule in §10). The laptop runs are manual gates at M2, M6 and M7. The Deck confirmation run is required at M7, and both are re-run before every release.

### 14.8 Running the Deck confirmation on a borrowed Deck or with an outside tester (new in v0.5, revised in v0.5.1)
The project owner has no Steam Deck yet, but has friends with early ones and will say when one is available (Q19). The Deck run is a **confirmation pass**: it is required before release (M7) and is welcome at any earlier milestone. It is run on a borrowed Deck, or by the friend on their own Deck, and recorded as a **Deck-equivalent**.

* **What counts as a Deck-equivalent run:** any retail Steam Deck (LCD or OLED) on a stable SteamOS 3 release, at default TDP with no frame limit, at 60 Hz, running a smzuggler release build of the commit under test. The report records:
  * `profile: steam-deck`, `run_on: borrowed` or `run_on: outside-tester`;
  * the model (LCD 60 Hz or OLED, run at 60 Hz) and the SteamOS version and channel;
  * the smzuggler commit and build ID;
  * the session (game mode or desktop mode), TDP and refresh rate;
  * **no** tester name, Steam account name, device serial or host name.
* **What the tester needs:**
  * a legitimately installed copy of the game on that Deck (the tester's own, or the owner's account on the borrowed device). smzuggler never ships game files;
  * the smzuggler AppImage for the release under test, and the test kit: the `--perf-suite` scene scripts and the fixture recipe (§14.10). The kit contains no game content and **no saves**: the tester makes their own three fixture saves in their own copy of the game, following the recipe;
  * about one hour, with the Deck charged or plugged in (the Deck run is at default TDP, so power source does not change it [G: note it in the report anyway]).
* **Steps for the tester** (also shipped as `docs/testing-deck.md`, written for a non-developer):
  0. Once: play the game normally and make the three fixture saves described in the recipe and import them with `--import-fixture` (§14.10). `--perf-suite` checks that each one is on the right map before it starts.
  1. Switch to desktop mode, copy the AppImage to `~/smzuggler/`, and mark it executable.
  2. Optional, for the game-mode run that gates: in Steam, *Add a Non-Steam Game* and pick the AppImage, then set its launch options to the command in step 3. This is ordinary Steam UI; smzuggler itself uses no Steam API.
  3. Run: `./smzuggler-rt-<version>.AppImage --game "<path to the game folder>" --perf-suite=deck --frame-stats=~/smzuggler/perf/`
     The suite plays S1–S9 by itself with scripted input (§14.4). Do not touch the controls until it says it is finished.
  4. Scrub and pack the result: `./smzuggler-rt-<version>.AppImage --scrub-report ~/smzuggler/perf/ --out ~/smzuggler/deck-report.tar.gz`
  5. Optional: open the `.json` files and read them. They are plain text, and the tester can check that nothing personal is in them.
  6. Send `deck-report.tar.gz` to the owner by whatever channel the two of them agree (Q19). Nothing is uploaded automatically.
* **What `--scrub-report` does:**
  * It keeps only the fields of the fixed report schema (frame records, summaries, markers, the run metadata above) and drops everything else, rather than trying to find and remove secrets.
  * Home paths become `~`. The host name, the user name, machine IDs, MAC addresses and serials are never collected in the first place, and the scrubber checks again for the local user and host names and refuses to pack if it finds them.
  * It writes a `SCRUBBED.txt` listing what it kept, so the tester can see it.
* **On receipt:** the report runs through the §16 privacy scan before it is stored or committed, and it is recorded as the Deck confirmation for that commit, marked "Deck-equivalent". The scrubber never packs save files.

### 14.9 Optional proxy: `deck-proxy` on dev-laptop (new in v0.5, optional since v0.5.1)
This is **optional**. The laptop gate on battery is already the operative gate (§14), so the proxy is only an extra early warning for Deck-specific CPU and GPU limits, run when it seems useful. **It is a proxy, not a pass.** Its reports use `profile: deck-proxy` and `gate: proxy`, and the analyser cannot turn them into a Deck pass (§14.7).

* **How the caps are set** (`tools/perf/deck-proxy.sh`, run by the owner with root; every step is undone by `deck-proxy.sh --restore`) [G: the numbers are first guesses, not calibrated]:
  * **CPU threads:** run the binary under `taskset -c 0-7`, i.e. the 4 performance cores and their 8 threads (the Deck has 4 cores / 8 threads). On dev-laptop CPUs 0–7 are the performance cores [M: 4.6 GHz maximum on cpu0, 3.4 GHz on cpu8 and cpu15].
  * **CPU clock:** cap the maximum frequency at about **2.8 GHz** (`cpupower frequency-set -u 2.8GHz`). The Deck boosts to 3.5 GHz, but its Zen 2 cores do less work per clock than these cores [G], so the cap is set lower.
  * **GPU clock:** cap the Iris Xe at about **1250 MHz** (`gt_max_freq_mhz`, from its 1450 MHz maximum). That puts its peak arithmetic close to the Deck's ~1.6 TFLOPS [G, from public specifications].
  * **Output:** run at 1280×800 and 60 Hz in a nested gamescope (`gamescope -W 1280 -H 800 -r 60 -- …`), as on the Deck.
  * **Power:** on AC with `balanced`, so the proxy measures the caps, not the battery governor.
* **Calibration:** once the first Deck-equivalent report exists, run S1–S3 on `deck-proxy` with the same commit, compare the p50 and p99, adjust the caps, and record the new values in `tools/perf/deck-proxy.md`. Until then, proxy numbers are early warnings only.
* **Never a substitute:** proxy results neither sign off a milestone (the laptop gate does that) nor stand in for the Deck confirmation before release (M7).

### 14.10 Fixture saves (new in v0.5.1)
The perf suite needs exactly **three** fixture saves. Everything else starts from boot or from one of these.

| Fixture | Used by | Where it must be [V-game: `data/*.json`, read-only] | Why |
|---|---|---|---|
| `fx-map92` | S2, S3, S8, S9 (soak start) | **Map 92** ("Northern US Port TEST", the game's start map: `System.json` `startMapId` 92 at (31,11)), standing on the map, no message open | a fixed, repeatable start state for the UltraMode7 walk loop; a new game also lands here, but with the opening sequence first |
| `fx-trade` | S4 | a port map, **Map 92 or Map 93** ("Southern US Port TEST"), a few tiles from the store door (event 45 at (25,10), action button, which transfers to Map 80 "shop loading"; the store UI is CE195 → CE473 `shop` with CE486 `shopControllerNEW` running in parallel), with money and cargo for at least one buy and one sell | the trade screen itself cannot be saved into [G], so the suite saves next to the door and its scripted input walks in |
| `fx-combat` | S5 | **Map 55** ("Atlantic Ocean") at sea, next to an encounter event that calls CE400 `combatStart` (for example event 11 at (134,39), action button) | one full picture dice duel (CE400–402); the encounter itself cannot be saved into [G] |

* **Where they live:** `$XDG_DATA_HOME/smzuggler-rt/fixtures/` as `fx-map92`, `fx-trade` and `fx-combat`. Whoever runs the suite makes each save in any slot during normal play and copies it there once with `--import-fixture <slot> <fixture name>`, which only reads the slot. `--perf-suite` loads each fixture, checks its map ID before scoring, and stops with a clear message if one is missing or on the wrong map. It never writes the fixtures; S8 saves into a scratch save directory.
* **Chosen slots on dev-laptop (owner, 2026-10-30) [M]:** copied once, read-only, out of the Proton save folder:
  * `fx-map92` = `file12` (Map 92 at (37,20)).
  * `fx-trade` = `file4` (Map 92 at (24,14)), 4 tiles below the store door.
  * `fx-combat` = `file6` (Map 55 at (133,40), sailing), next to event 11 at (134,39).
  Whether `file4` has enough money and cargo, and whether the encounter at event 11 is still live in `file6`, is **not yet checked** [U]; the first suite run tells. Until smzuggler's own `--import-fixture` exists, the three files sit in a local fixtures folder under `~` (outside every repository) with a SHA-256 list and a README recording this mapping, next to a timestamped, checksummed full backup of the Proton save folder.
* **Test copy and isolated save directory (dev-laptop, pre-M0) [M]:** Outsider test runs use a separate test copy of the game under `~`, never the Steam install folder, and never write the Proton save folder. Because the game's save-location plugin reads `process.env` (empty in Outsider, which produced a stray `undefined/.local/share/...` folder inside the test copy) and Outsider only opens paths inside the game root, the local launch helper sets `XDG_DATA_HOME` in its per-run shim copy to a symlink inside the test copy that points at a separate runtime save folder under `~`. The helper refuses Steam or Proton paths, has a `--dry-run` that prints the resolved paths, and has `--fixture NAME [--slot N]`, which backs up the runtime save folder and then installs a fixture as slot N with a matching `global.rmmzsave` entry. smzuggler replaces this with a real `process.env` and `$XDG_DATA_HOME/smzuggler-rt/` (§5.3).
* **Never committed or shared.** Fixture saves are the owner's own play data. They live outside the repository (`$XDG_DATA_HOME/smzuggler-rt/fixtures/`), are listed in `.gitignore` and in the §16 scan as a forbidden file type (`*.rmmzsave`, `*.rpgsave`), are never attached to issues or reports, and are never handed to a tester. `--scrub-report` refuses to pack them.
* **Testers create their own.** `docs/testing-fixtures.md` is the recipe: for each fixture, where to stand, what to have, and how to import it, described in words with map names and tile coordinates. The recipe contains no save data and no game screenshots (those would be game content, §1).
* The Proton save folder is only read (for the one-time import, §5.3). Fixtures are copied out of it, never written back.

---

## 15. Documentation and readability standard (new in v0.4)

**Goal:** someone who has never seen smzuggler can open any file, understand why it exists and what it does, and change it safely. They should not need to ask the author. This is a **review gate**: a change that does not meet this section is not merged, however good the code is.

### 15.1 Doc comments: files, modules and functions
* **Every file** starts with a header block after the SPDX/copyright lines (§12). The header says:
  * what the file is for, in one or two sentences;
  * **why it exists**: which problem it solves and which part of the browser, NW.js or Node it stands in for;
  * where it sits in the architecture (a link to the subsystem doc);
  * the rules a reader must not break (threading, ownership, ordering).
* **Every module** (each directory under `src/`) has a `README.md` with its purpose, its entry points, its data flow, who owns which resources, how it is tested, and links to the relevant compat-table rows.
* **Every function**, including `static` helpers and JS shim methods, has a doc comment that covers:
  * **why** the function exists, not only what it does;
  * its parameters, with units and valid ranges, and its return value;
  * ownership: who frees what it returns, and whether it keeps any pointer it is given;
  * which thread may call it, and what happens on error;
  * for bindings and shims, **which web, NW.js or Node API it implements**, with the compat-table ID and a link to the reference (MDN, WHATWG, Khronos, the NW.js or Node docs). Any deviation from the reference is stated.
* Trivial functions still get one line. "Gets the width" is fine when that is all it does.
* Comments explain **why** and **what is surprising**. They do not repeat the code. When code is changed, the comments are fixed in the same change.

### 15.2 `ARCHITECTURE.md`
At the repo root, about 2–4 pages:
* a one-diagram overview: platform layer → game loop → V8 → shims → WebGL2/Canvas2D/audio/video → GPU and audio device;
* the startup sequence, from `main()` to `Scene_Boot`;
* the frame lifecycle (timers → rAF → microtasks → render → present), and where frame stats are taken (§14.3);
* threads, and what each one may touch;
* a directory map with one line per directory;
* a "where do I start?" list for common tasks (fix a plugin crash, add an API, chase a frame spike).

### 15.3 Per-subsystem docs (`docs/`)

| Doc | Covers |
|---|---|
| `js-binding.md` | V8 isolate and context setup, the `jsb` seam, how a native function is exposed, exceptions, microtasks, the code cache, the inspector, and **the handle and lifetime rules (§15.8)** |
| `webgl.md` | the WebGL2 → GLES 3.0 mapping, ANGLE vs. system GL, the state cache, GLSL ES handling, extensions, context loss, and **the GL object lifetime rules (§15.8)** |
| `canvas2d-text.md` | the Canvas2D subset, the font pipeline, text measuring, and how bitmaps reach textures |
| `audio.md` | the WebAudio node graph on SoLoud, decoders, timing and latency |
| `video.md` | the nestegg + libvpx + libopus pipeline, A/V sync, and how frames reach PIXI |
| `saves.md` | per-OS paths, the read-only game folder guard, the save format, and Proton import (and Q13) |
| `shims.md` | the DOM, NW.js, Node and navigator shims, load order, and what each shim fakes and why |
| `build.md`, `testing.md` | dependencies and pins, per-platform builds, test suites, the evaluator, and the §14 harness |
| `UPSTREAM.md`, `privacy.md`, `style.md` | §8, §16 and §15.6 |

### 15.4 The compat-surface table (`docs/compat-surface.md`)
* One row per shimmed browser, NW.js or Node API: global, constructor, method or property.
* **Columns:** ID · API · kind (DOM / NW.js / Node / WebGL2 / WebAudio / Canvas2D / other) · status (full / partial / stub / absent) · implementation file and symbol · tests · plugins or rmmz files that use it · notes and deviations.
* **Kept honest by CI.** `smzuggler-rt --dump-surface` lists every global and prototype member the shims install at run time. A CI script fails when the dump has an API that is missing from the table, when the table lists one that no longer exists, or when a row names a file or test that does not exist.

### 15.5 Guide: adding a missing API (`docs/adding-an-api.md`)
A step-by-step guide for when a new plugin needs something smzuggler does not have:
1. Reproduce it: find the failing call from the error, the stack and the missing-member log.
2. Look up the reference behaviour (MDN, Node or NW.js docs), and check what NW.js 0.80 actually does with the evaluator.
3. Decide where it goes: JS shim or native binding (with criteria: speed, access to OS or GPU).
4. Write the test first.
5. Implement it, with doc comments (§15.1), in the file the table says owns that area.
6. Add or update its compat-table row.
7. Run the conformance and performance suites.
8. Decide whether it goes upstream (§8.1). If the fix is engine-neutral, open the upstream PR first.

It includes two worked examples from this project: `document.getElementsByTagName('head')` (§5.1) and `nw.Window` events (§5.2).

### 15.6 Naming and style (`docs/style.md`)
* **Tooling:** formatting is automatic (`clang-format`, and `eslint`/`prettier` for shims), with the configs checked in. Nobody argues about formatting in review.
* **Names:**
  * C uses `snake_case`, with a module prefix (`jsb_`, `webgl_`, `audio_`, `video_`, `saves_`); types are `PascalCase` and constants `UPPER_SNAKE`.
  * Units go in names (`_ms`, `_px`, `_bytes`, `_hz`).
  * No abbreviations except a short glossary in `style.md` (GL, VAO, FBO, UBO, rAF, CE…).
* **Control flow is visible.**
  * **No clever macros:** no function-like macro that contains `return`, `goto`, `break`, a loop or a hidden allocation.
  * Macros are allowed only for constants, and for documented X-macro tables that do not hide control flow.
  * Errors are checked where they happen, and each error path is easy to see.
* **Keep it simple:**
  * Functions are short: a soft limit of ~60 lines. A longer one needs a comment that says why.
  * No template metaprogramming in the V8 glue beyond what V8's own API requires.
  * One concept per file.
  * Prefer plain loops and plain structs over clever abstractions.
* **Generated code:** if the WebGL entry points (~150 functions) are generated, the generator is a readable, documented script in `tools/`. The generated file is committed, carries a "generated by …, do not edit" header, and is reviewed like normal code.
* **JS shims:**
  * Shims shared with upstream stay ES5 (§8).
  * smzuggler-only shims may use ES2020, because V8 has it, but they still follow §15.1.

### 15.7 Docs change with the code (review gate)
* **Same change.** Code and its docs change in the same PR. A PR that changes behaviour without changing its docs is not merged.
* **CI checks:**
  * doc lint fails when a file header or a function doc comment is missing (C and C++ via a small parser script; JS via eslint `jsdoc` rules);
  * the compat-table check (§15.4);
  * Markdown link check;
  * the **docs-touched gate**: if `src/<subsystem>/` changes, then that subsystem's doc, its module README or the compat table must also change. The only exception is a PR labelled `no-doc-change`, and the reviewer must agree with the stated reason.
* **PR template checklist:** why the change is needed · docs updated · compat row updated · tests added · upstream-eligible? · privacy scan clean (§16).
* **Fresh-eyes review:** each subsystem doc is read once, at its milestone, by someone who did not write it. Every question they ask becomes a doc fix.

### 15.8 Lifetime and handle rules, in plain language
These rules are written out in full in `js-binding.md` and `webgl.md`. A short comment pointing to them sits in every file that touches handles or GL objects.
* **V8 handles:**
  * A `Local<T>` is a temporary handle. It is valid only until the `HandleScope` it was created in ends. **Never store a `Local` in a C struct, a global or a static.**
  * Every native function called from JS, and every callback we drive (timers, rAF, audio and video events), opens its own `HandleScope`. A loop that creates many handles opens one scope per iteration, so handles do not pile up.
  * Return a `Local` out of a scope with `EscapableHandleScope::Escape`. Nothing else is safe.
  * To keep a JS object beyond the current call, use `v8::Global<T>`. Each `Global` has exactly one owner, and the owner calls `Reset()` when done. Leaked Globals are leaked memory.
  * Weak Globals with finalizers are used only to release native resources, and the finalizer does not run JS or touch GL (see "GL objects" below).
  * `ArrayBuffer` backing stores: say who owns the memory each time one is created (V8, or our allocator), and report external memory to V8 (`AdjustAmountOfExternalAllocatedMemory`) so that GC timing stays honest.
  * Only the main thread enters the isolate. Other threads post work to it.
* **GL objects:**
  * Every GL object (texture, buffer, framebuffer, renderbuffer, program, shader, VAO, query, sampler) belongs to one context and is only touched on the main thread.
  * Each JS WebGL object (`WebGLTexture` and so on) wraps exactly one GL name and **owns** it.
  * The GL name is freed when JS calls `delete*()`, or when the wrapper is garbage-collected. A GC finalizer never calls GL directly: it **queues** the name, and the queue is drained at the end of the frame on the main thread.
  * After `delete*()`, the wrapper is marked dead, and later use reports the same WebGL error that Chrome does.
  * On context loss, every wrapper is marked invalid and GL names are forgotten, not deleted.
  * Shader compilation errors are logged with the shader source line, so plugin authors can see them.

---

## 16. Privacy and commit hygiene (new in v0.4)

**Rule:** no personal names, email addresses, user names or host names in **code, docs, commits, logs, test reports or fixtures**. This also applies to anything sent upstream (§8) and to logs shared with anyone outside the project.

* **What to use instead:**

  | Instead of | Use |
  |---|---|
  | a person's name | "the project owner", or a role ("the reviewer") |
  | a home directory path | `~`, `$HOME` or `<home>` |
  | a machine | its target-profile name (`dev-laptop`, `steam-deck`, §14.1) |
  | an email | `@users.noreply.github.com` (the project identity below) or `example.invalid` |

* **Copyright lines** name the project, not a person: `Copyright (C) 2026 The smzuggler contributors`. Upstream PRs carry the copyright line that upstream requires (§8.2).
* **Commit identity.** Every commit, in smzuggler and in upstream PRs, is authored **and** committed as:
  `myonendy <350390780+myonendy@users.noreply.github.com>`
  * Set it per repository (`git config user.name` / `user.email`), never globally, so that other projects are not affected.
  * Commit messages and PR text follow the same rule.
* **Logs written by smzuggler:**
  * The logger replaces the home directory prefix with `~` before writing anything.
  * It never writes the host name, the user name or the full environment. `process.env` is visible to the game (§5.3) but is never logged.
  * Crash and perf reports go through the same filter.
  * A unit test feeds the logger a fake home path and host name and checks that neither comes out.
* **Scan, run in two places:**
  1. **pre-commit hook** (`tools/privacy/pre-commit`, installed by `tools/privacy/install-hooks.sh`). It scans the staged diff, the names of added files and the commit message.
  2. **CI job** `privacy-scan`. It runs on every PR and on `main`, over the full tree, every commit in the PR (message, author, committer) and any test or perf reports the PR adds.
* **What the scan (`tools/privacy/scan.py`) checks:**
  * **Built-in, generic rules (in the repo):**
    * absolute home paths: `/home/<name>/`, `/Users/<name>/`, `C:\Users\<name>\`;
    * email addresses, except `*@users.noreply.github.com` and `*@example.invalid`;
    * `user@host` shell-prompt patterns;
    * PNG/JPEG metadata text chunks in committed images;
    * save files of any kind (`*.rmmzsave`, `*.rpgsave`), so fixture saves can never be committed (§14.10, new in v0.5.1).
  * **Commit identity:** author and committer must equal the project identity above, or CI fails.
  * **Deny-list (not in the repo):**
    * The specific names, emails, user names and host names to block are **never committed**, because the list would leak what it protects.
    * Locally, the hook reads `~/.config/smzuggler-rt/privacy-denylist.txt`. CI reads a repository secret, `PRIVACY_DENYLIST`.
    * Matching is case-insensitive. Each entry is a literal string, or `re:` followed by a regex.
    * The scan reports the file and line of a match but **not the matched text**, so CI logs do not leak it either.
  * **Overrides:** none. A false positive is fixed by changing the text, or by adding a reviewed entry to the in-repo allow-list of generic patterns. A deny-list hit can never be overridden.
* **History.** If something slips through to `main`, it is fixed by a new commit, and the scan is extended to catch it. Rewriting the history of the public `main` is a last resort that needs the owner's explicit decision, because it breaks everyone's clones (§8.3).
* **Effort:** ~0.25 pw in M0 (§9).

---

## 17. Later (deferred, not in the estimate)

* **Android arm64** (was the v0.3 "stretch" target), **+4–8 pw [G]**:
  * an app project (Gradle, SDL3's Android Java glue, `libmain.so`);
  * touch input, mapping MZ touch to SDL;
  * storage and save paths through `SDL_GetPrefPath`, plus Android scoped storage for the user's game copy;
  * packaging and signing;
  * system GLES 3.0 testing across GPU vendors;
  * V8 arm64 JIT on Android;
  * a 60 fps target profile for one reference phone, defined like §14.1.
  * The design keeps Android open: GLES 3.0 is the primary graphics API, paths come from SDL, and `process.platform` reports `android`. Nothing in M0–M7 may make Android harder.
* **iOS and consoles** stay out of scope (§3.3). They are listed here only so they are not forgotten. They would need jitless V8, native hot paths, non-wasm Effekseer and a licensing answer.
* **FreeType/HarfBuzz upstream proposal** (§8.1 "Never upstream" notes this could be offered later).
* **RPG Maker MV games:** see §18, with its own estimate of ≈ 10–20 pw.
* **Steam integration** (Steamworks, Steam Cloud API, achievements, overlay): not planned for now (§5.3, §11). It would need a licence review before any work.

## 18. MV support (later, not in the main estimate) (new in v0.5)

This section answers "doesn't this game use RPG Maker MV?" and then describes what it would take to extend smzuggler to RPG Maker MV games. **None of this is in the 38–72 pw estimate.** It is planned work for after M7.

Tags used here: **[V-game]** verified by reading the installed game folder (read-only); **[V-core]** verified by reading the official MV core scripts in `github.com/rpgtkoolmv/corescript` (branch `master`, last commit 2019-01-07), file and line given; **[V-web]** from public release notes and forum posts; **[G]** a guess; **[U]** a hypothesis with the test that would settle it.

### 18.1 Engine check of the game: it is MZ, not MV [V-game, 2026-10-30]
* **Core file names:** `js/` holds `rmmz_core.js`, `rmmz_managers.js`, `rmmz_objects.js`, `rmmz_scenes.js`, `rmmz_sprites.js` and `rmmz_windows.js`. There is **no** `rpg_*.js` file anywhere in the game folder (MV's core files are `rpg_core.js`, `rpg_managers.js` and so on).
* **Version strings:**
  * `rmmz_core.js` line 2: `// rmmz_core.js v1.10.0`;
  * `rmmz_core.js` line 187: `Utils.RPGMAKER_NAME = "MZ";`
  * `rmmz_core.js` line 195: `Utils.RPGMAKER_VERSION = "1.10.0";`
  * `main.js` header: v1.10.0. It loads `libs/pixi.js`, `pako.min.js`, `localforage.min.js`, `effekseer.min.js` and `vorbisdecoder.js`, then the `rmmz_*` files, then `plugins.js`. `effekseer.wasm` is present.
* **PIXI:** `js/libs/pixi.js` header `pixi.js - v5.3.12, Compiled Wed, 23 Mar 2022 18:38:07 UTC`, and line 3938 `VERSION = '5.3.12'`.
* **`package.json`:** `"name": "rmmz-HARS"`, `"main": "index.html"`, `"js-flags": "--max-old-space-size=4096 --expose-gc --optimize-for-size"`, `"chromium-args": "--force-color-profile=srgb --disable-devtools"`, window 1280×720.
* **NW.js:** `nw.dll` FileVersion and ProductVersion 0.80.0.
* **`System.json` `advanced`:** gameId 13281867, screen 1280×720, uiArea 1000×700, `mainFontFilename` `LTWoodchuckRegular.ttf`, `fallbackFonts` "Verdana, sans-serif", fontSize 28, `picturesUpperLimit` 255. The `advanced` block is an MZ-only structure.
* **Layout:** the game files sit at the folder root, with no `www/` folder. Saves use the `.rmmzsave` extension (pako-compressed), which is MZ's format.
* **Node and NW.js use:** `rmmz_managers.js` calls `require("fs")` (lines 681–758) and `require("path")` (line 763), and uses `nw.Window.get().showDevTools()` (line 2062), i.e. the NW.js ≥ 0.13 API.
* **Conclusion:** the game is **RPG Maker MZ 1.10.0** on NW.js 0.80.0 with PIXI 5.3.12. Nothing in the main plan changes.
* **Side note:** the game's Steam build changed on 2026-10-30 at 08:43 EDT (build 26300128). The Unity wrapper files changed, and an empty `save/` folder appeared in the game folder at that time; smzuggler work did not create it. The JS core files are unchanged since 2026-09-25 [V-game, read-only listing].

### 18.2 Where the MV facts come from
* **No MV game is installed locally:** none of the 35 installed Steam games contains `rpg_core.js` [V-game, read-only search]. So the MV facts below come from the official core scripts [V-core] and public notes [V-web], not from a real deployed game. They must be re-checked against the Q18 corpus before MV work starts.
* **The MV core scripts** at corescript `master` report `Utils.RPGMAKER_NAME = 'MV'` (`js/rpg_core/Utils.js:19`) and `Utils.RPGMAKER_VERSION = "1.6.1"` (`Utils.js:29`), and ship PIXI **4.5.4** (`js/libs/pixi.js` header, compiled 2017-08-23) with `pixi-tilemap.js`, `pixi-picture.js`, `fpsmeter.js`, `lz-string.js` and `iphone-inline-video.browser.js` [V-core].
* **MV version history [V-web]:** PIXI 4.5.4 since MV 1.5.1 (September 2017); MV 1.6.0 (February 2018) moved to NW.js 0.25.4; MV 1.6.1 (June 2018) shipped NW.js 0.29.0 and Linux export. Earlier MV releases shipped NW.js 0.12.x [G, widely reported, not checked here]. MV before 1.3 used PIXI 2.x, and 1.3.0 brought PIXI 4.0 [G, from memory]. **Deployed MV games keep whatever PIXI and NW.js version they were exported with**, so all of these exist in the wild.

### 18.3 What is different in MV, and what smzuggler would need

| Area | MV behaviour | What smzuggler needs | Basis |
|---|---|---|---|
| **PIXI version** | PIXI 4.5.4 (or older) from the game's own `js/libs/pixi.js`, plus `pixi-tilemap` and `pixi-picture` | **Mostly free:** smzuggler already loads the game's own PIXI (§4), so each game brings its own. The work is in the WebGL context PIXI 4 asks for (next row) and in testing both PIXI generations | [V-core] |
| **WebGL1 and PIXI 4 renderer quirks** | `Graphics._createRenderer` uses `PIXI.CanvasRenderer`, `WebGLRenderer` or `autoDetectRenderer` depending on `_rendererType` (default `'auto'`) (`Graphics.js:1104–1118`). PIXI 4 asks for `getContext('webgl')` / `'experimental-webgl'` only (`pixi.js:27706`), i.e. **WebGL1**, while PIXI 5.3 prefers WebGL2. `Graphics.hasWebGL` probes the same way (`Graphics.js:201–204`) | A **WebGL1 context** on the same GLES 3.0 backend: GLSL ES 1.00 shaders (GLES 3 accepts them), the WebGL1 extension list PIXI 4 checks (`OES_vertex_array_object`, `OES_element_index_uint`, `WEBGL_lose_context`, …), WebGL1 texture-format and NPOT rules, `UNPACK_PREMULTIPLY_ALPHA_WEBGL` for canvas uploads. Report WebGL as available so `auto` never falls back to `CanvasRenderer`; the Canvas renderer path is not a target [G] | [V-core] for the calls; quirks [G] |
| **Difference-blend probe** | `Graphics._canUseDifferenceBlend` draws to a canvas and reads pixels back (`Graphics.js:854`) | Canvas2D `globalCompositeOperation = 'difference'` and `getImageData` must be exact, or the probe says no and the game silently uses a different effect | [V-core] |
| **Core scripts** | `rpg_core.js`, `rpg_managers.js`, `rpg_objects.js`, `rpg_scenes.js`, `rpg_sprites.js`, `rpg_windows.js`; listed as `<script>` tags in `index.html` [G: corescript builds them from `js/rpg_*/`] | Engine detection by file name and `Utils.RPGMAKER_NAME`; a loader that runs the `<script>` tags of `index.html` in order instead of MZ's `main.js` list. No changes to the core files, as for MZ | [V-core] names; loading [G] |
| **NW.js era (0.12–0.29)** | `require('nw.gui')` (`SceneManager.js:114`), `require('nw.gui').Window.get().showDevTools()` (`SceneManager.js:193`); `process.mainModule.filename` gives the base folder (`StorageManager.js:197`) | `require('nw.gui')` returns the same object as `nw` (with `Window.get()`, `App`, the old event API); `process.mainModule.filename` = `<root>/www/index.html`; old Node `fs` sync calls (`existsSync`, `mkdirSync` without `recursive`, `writeFileSync`, `readFileSync`); `process.versions` reporting the game's own NW.js version from its `nw.dll`/`package.json` era [G] | [V-core] calls; versions [G] |
| **Audio** | WebAudio by default: whole-file XHR + `decodeAudioData` for `.ogg` or `.m4a` (`WebAudio.js`, `_canPlayOgg` / `_canPlayM4a` at line 127). `AudioManager.shouldUseHtml5Audio` returns `false` in 1.6.1 (`AudioManager.js:400–404`); older versions used `Html5Audio` on Android or for encrypted audio | Full-file Ogg Vorbis `decodeAudioData` (MZ streams through `vorbisdecoder.js` instead). Answer `canPlayType('audio/ogg')` yes and `audio/mp4` no, so games pick Ogg and no AAC decoder is needed. A minimal `Html5Audio` (an `<audio>` element) for old versions [G] | [V-core] |
| **Video** | `Graphics._createVideo` creates a DOM `<video>` element, appended to `document.body` and shown over the canvas (`Graphics.js:1023–1032`); the interpreter picks `.webm` or `.mp4` via `canPlayType` (`Graphics.js:588`) | A `<video>` element backed by the existing libvpx/nestegg pipeline, composited **above** the game canvas (MZ draws video inside PIXI). Answer `canPlayType('video/webm')` yes and `video/mp4` no, so games pick WebM. No transcode cache, as for MZ. Vorbis audio in WebM as well as Opus [G] | [V-core] |
| **Saves (path and format)** | `www/save/file<N>.rpgsave`, `global.rpgsave`, `config.rpgsave` (`StorageManager.js:204–215`), as `LZString.compressToBase64(JSON)` text (`StorageManager.js:112–121`). The folder is `<base>/save/`. To decide that, MV **creates and removes a `testDirectory/` inside the game folder** (`canMakeWwwSaveDirectory`, `StorageManager.js:228–243`), and falls back to `<base>/../save/` | **This conflicts with the read-only game folder rule, so it is redirected:** `bind_io` maps `<base>/save/` and `<base>/../save/` to `$XDG_DATA_HOME/smzuggler-rt/mv/<game-id>/save/` (game ID from `System.json` or a hash of the title and path), and answers the `testDirectory` probe virtually so no directory is ever created in the game folder. The `.rpgsave` bytes stay exactly as MV writes them (LZString is the game's own code). A one-time import copies an existing `www/save/` | [V-core] |
| **Folder layout** | Root `package.json` with `"main": "www/index.html"`; game files under `www/` (`js/`, `data/`, `img/`, `audio/`, `movies/`, `fonts/`, `save/`) [G: common knowledge plus the 1.6.1 note that it auto-creates the root `package.json`] | Accept both layouts: read `package.json` `main`, set the web root to its folder, and keep `process.cwd()` and `mainModule` consistent with what MV computes | [V-web] + [G] |
| **Frame loop** | `SceneManager.requestUpdate` calls `requestAnimationFrame` (`SceneManager.js:146–148`); `updateMain` runs a fixed-step **1/60 s accumulator** (`_deltaTime` line 30, `_accumulator` line 32), caps catch-up at 0.25 s, updates 0..n times, then renders once (`SceneManager.js:225–245`). MZ instead drives `SceneManager` from PIXI's `Ticker` (`Graphics._onTick`, `rmmz_core.js:797`) | A vblank-driven `requestAnimationFrame` with an accurate `performance.now()`, as for MZ. At 59.93 Hz the accumulator occasionally runs two updates in one frame; the §14 analyser must count that as normal, not as a stall [G] | [V-core] |
| **Fonts** | `Graphics.loadFont` injects a CSS `@font-face` rule through a DOM `<style>` element (`Graphics.js:501–504`); `fonts/gamefont.css` is linked from `index.html` [G]; `Graphics.isFontLoaded` uses `document.fonts.check` if present, or else compares `measureText` widths on a hidden canvas (`Graphics.js:519–540`) | Parse `@font-face` rules from `<style>` text and linked CSS, load the files through `font_manager`, and make `document.fonts.check` and the width comparison both report correctly. MZ uses the `FontFace` API instead | [V-core] |
| **Graphics DOM elements** | `Graphics._createAllElements` creates an error printer `<div>`, the canvas, the video, an upper canvas, an **FPSMeter** (`fpsmeter.js`, heavy DOM and style use), a mode box and a font-loader `<div>` (`Graphics.js:763–771`); it centres elements with `style` margins and handles window resize | Enough DOM for these: `createElement`, `appendChild`, `style` properties, `id`, `innerHTML` for the error printer, resize events, and the fullscreen API. FPSMeter only needs to not crash and can draw nothing [G] | [V-core] |
| **Encrypted assets** | `Decrypter` XORs a 16-byte header (`Decrypter.js:9`, 73–93) of `.rpgmvp` / `.rpgmvo` / `.rpgmvm` files with `encryptionKey` from `System.json`, then loads through a Blob URL [G for the extensions and Blob URL; the XOR code is V-core] | XHR `arraybuffer`, `Blob` and `URL.createObjectURL` feeding `Image` and WebAudio. This is the game's own code, so smzuggler only needs those web APIs | [V-core] + [G] |
| **Common MV plugins** | Yanfly (YEP_CoreEngine, YEP_MessageCore, YEP_OptionsCore, YEP_KeyboardConfig …) and others change resolution through `Graphics`/`SceneManager`, resize the NW.js window, add DOM elements (for example `<input>` for name entry), listen to `document`/`window` events and sometimes read `navigator` [G] | A plugin compatibility pass over the Q18 corpus, adding DOM and NW.js surface only where a real plugin needs it, documented in the compat table (§15). Plugins that need a full browser DOM are listed as unsupported rather than emulated [U: scan the corpus for `document.`, `window.`, `require(` and `nw.` before estimating more closely] | [G] |

### 18.4 Estimate and placement (later, not in the main estimate) [G]
| Item | pw [G] |
|---|---:|
| Engine detection, both folder layouts, `index.html` script loader, per-game PIXI | 0.5–1 |
| WebGL1 context on the GLES 3.0 backend, PIXI 4.5.4 + pixi-tilemap/pixi-picture conformance | 2–4 |
| NW.js 0.12–0.29 surface (`nw.gui`, old `fs`, `mainModule`) | 0.5–1 |
| DOM for MV's `Graphics` (elements, FPSMeter, fullscreen, resize) | 1–2 |
| Fonts via `@font-face` and `document.fonts.check` | 0.5–1 |
| Audio: full-file Ogg `decodeAudioData`, minimal `Html5Audio` | 0.5–1.5 |
| Video: `<video>` element overlay on the libvpx pipeline | 0.5–1.5 |
| Saves: path redirect, virtual `testDirectory`, import | 0.5–1 |
| Encrypted assets (Blob URLs) and the `difference` probe | 0.25–0.5 |
| Frame-loop check at 59.93 Hz | 0.25–0.5 |
| Plugin compatibility over the Q18 corpus (3–5 games) | 2–4 |
| Documentation to the §15 standard (~15–20%) | 1–2 |
| **Total** | **≈ 9.5–20, i.e. about 10–20 pw** |

* **Placement:** a separate milestone **MV**, after M7, in three steps:
  * **MV-0** (first MV game boots to the title): detection, layout, loader, WebGL1, `nw.gui`;
  * **MV-1** (one plain MV game at parity): DOM, fonts, audio, video, saves, encrypted assets;
  * **MV-2** (the Q18 corpus): plugin pass and docs. Optionally a §14-style 60 fps check for one chosen MV game, which would add about +0.5–1 pw [G].
* **Prerequisites:** M2 (WebGL backend), M4 (audio), M5 (saves and the redirect guard) and M6 (video). Q18 must be answered first.
* **Biggest uncertainty:** the plugin pass. MV games lean on plugins much more than MZ games of the same size [G], and the corpus decides the real number.

### 18.5 Keeping MV open in M0–M7 (no extra cost)
* Never hard-code `rmmz_` file names, `js/libs/pixi.js` contents or the MZ `main.js` load list outside `game_profile.c` and the loader.
* Keep the WebGL context type (`webgl` vs `webgl2`) a property of the context, not a global assumption.
* Keep the save-path guard (§5.3) generic: it refuses any write under the game root, whatever the engine.
* Keep `nw` as one object, so that `require('nw.gui')` can return it later.

### 18.6 Verified vs assumed
* **Verified:** that the game is MZ 1.10.0 with PIXI 5.3.12 on NW.js 0.80.0 [V-game]; every MV behaviour with a file and line above [V-core, at corescript `master` = MV 1.6.1]; the PIXI 4.5.4 and NW.js 0.25.4/0.29.0 release history [V-web].
* **Assumed:** how deployed games differ from corescript `master` (older PIXI and NW.js, the `www/` layout and `index.html` script tags); WebGL1 and PIXI 4 quirks on the GLES 3.0 backend; the behaviour of common plugins; every number in §18.4. All of these are checked against the Q18 corpus before MV-0 starts.

---

## Appendix A: Outsider facts used, with paths [V at f789f46]
* Size: `src/` has 29,468 lines of .c/.cpp/.h/.js. (v0.2 said ~34.5k, which included non-source files.) `src/shims/*.js` has 11,619 lines.
* QuickJS-ng v0.9.0, SDL 3.4.16 and Effekseer are pinned in `third_party/CMakeLists.txt`. SoLoud (`master`) and SDL_shadercross (`main`) are **unpinned**.
* Shim load order: `src/engine/script_loader.c:246–256`. Core files: 282–287. Post-core tilemap shim: ~294/470. `main.js`: 486.
* Shims are installed as files: `CMakeLists.txt`, `install(DIRECTORY src/shims/ DESTINATION shims)`.
* `pixi_shim.js` stubs:
  * `Buffer` ~3708–3730;
  * `Geometry` 3732–3770 ("stub for Tilemap.Layer VAO creation");
  * `Program` 3772–3783;
  * `Shader` 3785–3797;
  * `Graphics` circles and polygons in JS at 2652–2945;
  * built-in filter shaders at ~4249–4901.
* DOM: `dom_shim.js` `_head` at 1261, `_allCreatedElements` at 1269, `document.head` at 1278, `getElementsByTagName` at 1337–1344, `querySelector` at 1353, element `getElementsByTagName` at 533, video `.mpg` mapping at 964–988, `canPlayType` at 1150/1193, rAF at 229–246.
* `storage_shim.js`: `require` at ~330–340, `process` at 344–361, `nw` at 374–418.
* `navigator_shim.js:15–40`: Windows UA and `platform: "Win32"`.
* Logs in the game dir: `src/main.c:176–184`. `.jsc` cache: `src/engine/js_engine.c:437–571`.
* Licence: SPDX `GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial`; `LICENSES/GPL-3.0.txt`, `COMMERCIAL.md`. Copyright assignment requirement in `CONTRIBUTING.md`.

## Appendix B: Map from v0.2 sections
* **Carried forward unchanged** (read them in `V8_MZ_HOST_SPEC_v0.2.md`):
  * §2 API surface;
  * §4 event-loop semantics, except §4.1 here;
  * §5.1 V8 host details: isolate, ArrayBuffer allocator, external memory, ICU, inspector;
  * §5.2 WebGL2 shim surface;
  * §5.3 Canvas2D subset;
  * §5.4 image decode;
  * §5.7 video requirements;
  * §5.9 XHR/fetch;
  * §5.11 Effekseer, wasm and the Worker quirk;
  * §6 V8 gaps;
  * §7.1 measurements;
  * §9.2 save format;
  * §10.2 test suites;
  * §11.1–11.4, except as amended in §3.2 and §12 here;
  * §14 hardest parts. Ranking change: "Canvas2D text parity" moves up after the test drive, and "ANGLE integration" is added.
* **New after v0.2:** §13 naming, §14 profiles and 60 fps, §15 docs, §16 privacy, §17 later, §18 MV support (later).
* **Replaced:**
  * §0, §1, §3, §8, §12 and §13.1 by §0, §1, §4, §7, §9 and §8 here;
  * §5.6 audio by §4.3;
  * §5.10 nw/DOM by §5;
  * §7.3 jitless by §3.3;
  * §9.1 paths by §5.3/§5.6;
  * §15 open questions by §11.
