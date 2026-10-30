# V8-MZ Host: Specification and Difficulty Estimate

**Target:** run *HIROTONFA's American Revolution Smuggler* (RPG Maker MZ 1.10.0, stock NW.js 0.80.0 / Chrome 117.0.5938.63 build [V `nw_versions.txt`]) natively on Linux. The game's **real, unmodified** JavaScript runs on an **embedded V8**, which hosts the real PIXI 5.3.12, the rmmz core and the 57 enabled plugins. Rendering and I/O use a native SDL3 + OpenGL backend. The host is **GPL-3.0-or-later** FOSS and portable.

**Status:** draft v0.2, 2026-10-30. Author: Grok Bot, for the project owner. This **supersedes `QJS_MZ_HOST_SPEC.md`** (v0.1, QuickJS-ng). That file is kept beside this one with a "superseded" banner so the QuickJS measurements stay on record.

**Decisions taken (owner, 2026-10-30):**
* **JS engine: V8 only.** QuickJS-ng is dropped entirely: no engine abstraction layer and no QuickJS build.
* **License: GPL-3.0-or-later** (former Q6). Reusing Outsider code (GPL-3.0-or-later) is allowed (§13.1).
* **Video: libvpx (BSD-3) for VP9 + libopus + nestegg**, with **no transcode cache** (former Q3).

**Evidence tags:**
* **[M]** measured on the box this session;
* **[V]** verified by reading code or files;
* **[G]** a guess or estimate, which should be treated as such.

**Where the evidence lives** (all on the box, none in any repo):
* `/workspace/smuggler/hostspec/` holds the static scan, the dynamic traces, the scene census, the save test and the benchmarks.
* The phase-1 analysis is `/workspace/smuggler/SMUGGLER_RPGMZ_ANALYSIS.md`.

---

## 0. Executive summary

* **Verdict: feasible and the right architecture for this game.** It is still a substantial project, but switching to V8 removes the biggest risk in v0.1.
  * The estimate is **~30–58 person-weeks**, roughly 7–14 months, for one experienced C++/graphics/JS developer to reach M6 [G]. That covers ~23–45k lines of new or adapted host code, not counting vendored libraries and V8.
  * v0.1 (QuickJS) was 38–73 person-weeks.
  * Reaching a *walkable start map at 60 fps* (M2) takes roughly 4–7 months of that [G].
* **Why real PIXI rather than a PIXI reimplementation:** the start map (Map 92) and **~46 of 97 maps** use `<UltraMode7>` [V].
  * UltraMode7 drives PIXI's low-level `Program`/`Geometry`/`Shader`/`renderer.state`/raw `gl.drawElements` API with its own GLSL.
  * **Outsider** is the one existing FOSS native MZ runtime. It stubs exactly those APIs in its `pixi_shim.js` [V by code]. Its non-PIXI parts are reusable under our GPL-3.0 license (§13.1).
* **CPU speed is no longer the main risk.**
  * Real PIXI 5.3.12 rendering a Map-92-shaped scene (116 filters) takes **0.56 ms/frame on V8**, versus 12.5 ms on QuickJS-ng [M].
  * Chrome's whole rAF callback on Map 92 measured a mean of 2.57 ms and a p95 of 8.1 ms [M].
  * **Native PIXI/pako fast paths become optional.**
* **The main risks are now fidelity and build weight:**
  * the WebGL2 shim and Canvas2D text parity;
  * WebAudio and VP9 media;
  * the V8 build pipeline (gn/ninja) and binary size (§5.1, §11).
* **Quirks that only show up empirically, each of which breaks a naive host:**
  * saves are pako binary strings written as UTF-8;
  * `police radio.ogg` is WebM/Opus;
  * both movies are VP9+Opus;
  * Effekseer's wasm must initialise or `main.js` stops;
  * a Web Worker is spawned at load and never used;
  * `decodeAudioData` is fed truncated Ogg data.

---

## 1. Goals and non-goals

### Goals
1. **Byte-identical game logic.** The shipped `js/**/*.js` and `data/*.json` are executed exactly as shipped. The same inputs and the same RNG sequence must give the same `$game*` state and the same save files as NW.js (§10).
2. **No edits to the game folder.** The folder is treated as read-only: no bytecode caches, logs or saves are written inside it, and nothing is transcoded. Everything the host writes goes to XDG paths (§9).
3. **Real libraries:** the real `pixi.js` 5.3.12, `pako`, `localforage`, `effekseer.min.js` + `effekseer.wasm`, and `vorbisdecoder.js` are loaded as shipped. The host may *optionally* add native fast paths (e.g. native zlib behind `pako`) by patching prototypes after a library loads, as long as observable behaviour is identical. None is required (§7.2).
4. **Native on Linux x86-64 and arm64,** with no Chromium and no Node runtime: **V8 is embedded directly** (§5.1, §6). Host code is C11/C++17; V8 itself is built with clang via gn (§11.2). Windows and macOS are secondary targets.
5. **FOSS throughout:** the host is **GPL-3.0-or-later**, so Outsider code can be reused (§13.1). Every dependency must be GPL-3-compatible (§11.1).
6. **Performance:** a steady 60 fps on Map 92 on a mid-range 2020+ x86 CPU with integrated GPU. The p95 frame time must be no worse than ~2× Chromium's.

### Non-goals
* Being a browser. There is no HTML/CSS layout, and DOM objects are stubs (§5.10).
* Supporting arbitrary MZ games in v1. Other games are welcome but not acceptance criteria.
* Supporting the RPG Maker editor's playtest/devtools features.
* Pixel-perfect text. Pixel-near text is accepted against a threshold (§10.2).
* Encrypted assets. This game has none [V]; the hook is trivial to add later.
* Mobile/touch (Theo_DisableTouchUI is enabled anyway), and console/iOS ports (V8 would have to run jitless there, and GPL-3 conflicts with their store/SDK terms; §7.3).

---

## 2. Measured API surface

### 2.1 Method
* **Static scan** (`hostspec/static_scan.py`) is a token-level scan of the 57 enabled plugins plus core/libs. Its output is `scan/api_by_category.tsv`, `globals.tsv`, `webgl_calls.tsv` and `events_requires_exts.txt`.
  * It found **193 browser/Node APIs** (canvas2d 51, misc_dom 41, webaudio 20, node 19, video 12, image 11, net 10, timing 8, wasm 5, fonts 5, workers 4, nw 4, storage 3), **98 globals** and **97 WebGL identifiers**.
  * Static counts are noisy upper bounds. For example, `rect` has 1,159 token hits, almost all of them `Rectangle`/`rect` variables.
* **Dynamic trace** (`trace_shim.js` injected into headless Chrome serving a box copy of the game) covers boot → title → New Game → Map 92 walking, with a save/config/load test (`save_test.js`) and a scene census (`census.js`).
  * It recorded **237 distinct APIs**, of which 72 are WebGL2 methods. Output is `scan/dynamic_api_counts.tsv` (all rows, with sampled caller files) and `trace/run2/trace.json`.
  * The dynamic list is the authoritative "must work" set. The static list adds what is present in code paths that were not exercised (battle, minigames, options, movies). It is the "must at least not throw" set.

### 2.2 API table (dynamic call counts for the traced run; callers sampled)

Strategy key: **N** = native (C/C++), **P** = JS polyfill in the host prelude, **S** = stub returning plausible values, **0** = no-op.

| API (dynamic) | Calls | Main callers | Importance | Strategy |
|---|---:|---|---|---|
| **WebGL2RenderingContext** (72 methods, see §5.2) | 2,363,324 | pixi.js, UltraMode7, VisuMZ_0_CoreEngine (stencil), effekseer | critical | N, 1:1 |
| `uniformMatrix3fv` / `bindTexture` / `bindBuffer` / `bufferSubData` / `drawElements` | 412k / 287k / 191k / 191k / 171k | pixi batch + filters | critical, hot | N |
| `HTMLImageElement.width/height` get | 328k | rmmz_core Bitmap, pixi | hot | P data property set by N decode |
| `HTMLCanvasElement.width/height` get/set | 131k / 1.4k | rmmz_core, pixi, CoreEngine | hot | P object over N surface |
| `Navigator.userAgent` get | 40,470 | rmmz_core `Utils.isNwjs/isMobile*`, pixi | hot (per frame) | P constant string |
| `performance.now()` | 9,503 | pixi Ticker, rmmz, Mimosa | critical (timing) | N (monotonic, µs) |
| `requestAnimationFrame` | 3,566 | pixi Ticker | critical | N main loop (§4) |
| `KeyboardEvent.keyCode/key/ctrlKey/altKey/charCode` | ~5.8k | rmmz Input, CoreEngine, CGMZ_Core | critical | N events → P objects |
| `navigator.getGamepads()` | 2,334 (per frame) | rmmz Input | high | N (SDL gamepad, standard mapping) |
| **CanvasRenderingContext2D** (28 members) | 13,999 | rmmz Bitmap, LightingEffects, pixi | critical | N CPU raster |
| `save/restore/font=/globalAlpha` | ~1.7k each | Bitmap.drawText | hot | N |
| `measureText().width` | 1,054 | Bitmap.measureTextWidth, CoreEngine | critical for layout | N (HarfBuzz) |
| `fillText/strokeText` + textAlign/Baseline/lineWidth/lineJoin | 674 each | Bitmap.drawText (outline) | critical (visual) | N (FreeType) |
| `getImageData` | 88 | Bitmap.getPixel/getAlphaPixel, pixi | high | N; forces a CPU-side canvas |
| `createLinear/RadialGradient`, `addColorStop`, `arc`, `fill`, `globalCompositeOperation` | 5–40 | Bitmap.gradientFillRect, LightingEffects | medium | N |
| `drawImage`, `clearRect`, `fillRect`, `strokeRect`, `putImageData` | 1–48 | Bitmap.blt etc. | high | N |
| `document.createElement` | 671 | main.js (script), managers, pixi, core | critical | P (canvas/img/script/div/style/video) |
| `HTMLCanvasElement.getContext` | 583 | core, pixi, CGMZ_Core (webgl1 probe) | critical | P→N |
| `new Function()` | 1,302 | VisuMZ_0_CoreEngine, VisuMZ_1_OptionsCore | critical | V8 built-in |
| `Function.prototype.toString` | 13 | Mimosa_MouseCursor (re-evals source), localforage, OptionsCore | critical | built-in, **source must be kept** |
| `Number.toLocaleString` | 344 | CoreEngine (number formatting), rmmz_objects | high (visible text) | V8 + ICU built-in (P fallback if built without `Intl`) |
| `TextDecoder.decode` | 369 | effekseer.min.js | high | P/N (UTF-8; not in V8) |
| `HTMLMediaElement.src/paused/play/canPlayType` | ~1.3k | core Video, CGMZ_Core | high | P over N video; `canPlayType('audio/ogg')` → "probably" |
| `BaseAudioContext.*` (state, currentTime, createGain/Panner/BufferSource, decodeAudioData, destination) | ~665 | rmmz WebAudio, effekseer | critical | N (§5.6) |
| `AudioBufferSourceNode.*` (buffer, loop, loopStart/End, playbackRate, start, stop, onended) | ~360 | rmmz WebAudio, CaeF_PauseFix | critical | N |
| `AudioParam.setValueAtTime/linearRampToValueAtTime`, `GainNode.gain` | ~115 | WebAudio fades | high | N |
| `PannerNode.panningModel/setPosition` | 72 | WebAudio pan | medium | N (equal-power) |
| `fetch` + `Response.ok/body.getReader()` | 11 (+reads) | rmmz WebAudio streaming | critical for audio | P over N file read |
| `XMLHttpRequest` (open/send/onload/onerror/status/responseText/overrideMimeType/responseType) | 104 | DataManager JSON, main.js, effekseer wasm | critical | P over N file read |
| `Image` (`new`, src=, onload/onerror, complete, naturalWidth/Height) | ~700 | core Bitmap, CoreEngine | critical | P over N decode |
| `HTMLScriptElement` (src/type/async/defer, onload) | 279 | main.js, PluginManager | critical | P: load and eval the file, fire onload async |
| `Node.appendChild/removeChild/textContent/parentNode`, `Element.id/innerHTML/setAttribute`, `HTMLElement.style/onload/onerror/oncontextmenu`, `document.body/getElementById/getElementsByTagName/title/currentScript/fonts` | ~1.3k | main.js, core, CGMZ, Art_VerboseErrorScreen, Mimosa | must not throw | S (tiny DOM tree) |
| `addEventListener/removeEventListener` | 69 | pixi, Mimosa, core | high | P (window/document/canvas targets) |
| `FontFace` (`new`, `.load()`), `document.fonts.add/ready` | 7 | FontManager | critical (text) | N |
| `indexedDB.open`, `createObjectStore`, `transaction`, `openCursor` | ~20 | localforage init only | low | S (in-memory IDB) |
| `WebAssembly.instantiate` | 2 | effekseer.min.js, vorbisdecoder.js | critical for boot | V8 built-in (§5.11) |
| `new Worker(currentScript.src)` | 1 | vorbisdecoder.js | must not throw | S (inert Worker) |
| `OffscreenCanvas` + getContext | 4 | pixi feature detection | low | S/P |
| `WebGLRenderingContext.getParameter/getExtension/getContextAttributes` (webgl1) | 4 | CGMZ_Core GPU probe, pixi | low | N (same backend) |
| `setTimeout/setInterval/clearInterval` | 4 | CaeF_PauseFix, effekseer, EventsMoveCore | high | N timers in the loop |
| `atob` | 1 | vorbisdecoder (inline wasm) | must exist | P |
| `Element.webkitRequestFullScreen`, `document.webkitFullscreenElement`, `scrollTo` | 3 | Graphics | low | N (SDL fullscreen) / 0 |
| `navigator.languages/platform/maxTouchPoints` | 4 | effekseer, pixi | low | P constants |
| **Node/NW** (save test and static scan): `require('fs')` existsSync 13 / writeFileSync 3 / readFileSync / mkdirSync / readdirSync / unlinkSync / renameSync, `require('path')`, `process.env/platform/versions/hrtime` (12), `nw.App.argv/quit`, `nw.Window.get().on('close')`, `nw.Shell` | ~50 | StorageManager, Advanced_ChangeSaveLocation, CleanExit, core Utils | critical (saves) | N fs subset + P `path` + S `nw` |

The static-only items, present in code but not hit in the trace, are battle/minigame/option/movie paths. They include:
* `HTMLVideoElement` (NIJI_VideoPlayer, core Video);
* `AudioContext.resume/suspend`;
* `ctx.quadraticCurveTo/bezierCurveTo/closePath/clip/setTransform/translate/rotate/scale` (mostly LightingEffects, MessageCore and CGMZ);
* `localStorage` (Utils fallback);
* `window.open`, `nw.Shell.openExternal` (links);
* `MutationObserver` (2 hits);
* `SharedArrayBuffer` (4, inside the emscripten glue, feature-tested only);
* `replaceAll` and similar, which are built in.

These all belong in the **"must not throw"** layer (§5.10). Each must be backed by a minimal real implementation once its feature is exercised in M5.

### 2.3 Surprises worth recording
1. **The start map is Mode7** (`<UltraMode7>` note on Map 92, ~46 of 97 maps). That puts PIXI's low-level shader/geometry API and custom GLSL (`shaders/07.glsl`: `uMode7ProjectionMatrix`, `uFadeBegin/Range`, `aAnimation`) on the M2 critical path [V].
2. **`navigator.userAgent` is read ~40k times** and image/canvas `width`/`height` getters ~460k times. These must be cheap: plain data properties refreshed on change, not native accessors [M].
3. **`nw.App.argv` is read every frame** (Utils.isOptionValid) [V].
4. **1,302 `new Function` calls and Mimosa's `eval(fn.toString())`.** V8 keeps function source natively, and its code cache preserves it [G: standard V8 behaviour].
5. **Saves are binary strings written as UTF-8** (§9.2) [M].
6. **`decodeAudioData` is called on *partial* Ogg data** while a long BGM is still streaming (≥200 KB and ≥1 s since the last decode). The host decoder must accept truncated streams, as Chrome does [V `rmmz_core.js` `_updateBufferOnFetch`].
7. **Media codecs.** `audio/se/police radio.ogg` is **WebM/Opus**. Both `movies/*.webm` are **VP9 video + Opus audio**, with MP4 siblings that are unused because `canPlayWebm()` is true [M, read-only header inspection on dev-laptop].
8. **Effekseer must initialise** or `main.js` stops with an error [V]. It is used by only ~5 "Show Animation" commands.
9. **V8 is only ECMAScript.** `console`, timers, `TextDecoder`, `atob`, `structuredClone`, `fetch`, XHR and the DOM must all come from the host (§6) [V embedding docs].
10. **Frame pacing is game-configurable:** MultiTweaks overrides `Graphics._onTick` with an optional "optimise" throttle, and VisuMZ_1_OptionsCore writes `ticker.maxFPS` (§4.2) [V].
11. **The vorbisdecoder wasm is never used for decoding** (`_shouldUseDecoder()` is `!canPlayOgg()`), but it still loads and spawns a Worker [V].

---

## 3. Architecture

```
┌────────────────────────────── one process, one JS thread ───────────────────────────────┐
│ SDL3 (window, GL context, events, gamepad)                                              │
│                                                                                         │
│  main loop (§4) ── poll SDL events ─► JS event dispatch (keydown/keyup/blur/focus/…)    │
│      │           ── native completions ─► JS (image/audio decoded, file read)           │
│      │           ── due timers ─► JS                                                    │
│      │           ── rAF callbacks(ts) ─► PIXI Ticker ─► Graphics._onTick ─► rmmz        │
│      │           ── microtask checkpoint after every macrotask                          │
│      │           ── SwapWindow (vsync) ─► frame pacing                                  │
│                                                                                         │
│  V8 isolate (1 context) ◄── host prelude JS (DOM/BOM/nw/process stubs, polyfills)       │
│      + v8::Platform message pump (PumpMessageLoop) every iteration                      │
│      native modules (C):                                                                │
│      ├── webgl    ─► GL 3.3 core / GLES 3.0 (or ANGLE) + GLSL ES rewriter               │
│      ├── canvas2d ─► CPU raster (RGBA8 premultiplied) + FreeType/HarfBuzz text          │
│      ├── image    ─► stb_image / libpng (decode on a worker thread)                     │
│      ├── audio    ─► WebAudio-subset graph → miniaudio device; Vorbis/Opus decoders     │
│      ├── video    ─► nestegg + libvpx (VP9) + libopus → GL texture + audio graph        │
│      ├── io       ─► sync fs subset, file fetch/XHR, XDG save dir                       │
│      ├── wasm     ─► V8 built-in WebAssembly (effekseer.wasm as shipped)                │
│      └── fastpath ─► optional native zlib behind pako (§7.2)                            │
│ worker threads: image decode, audio decode, audio mixing callback, video decode         │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

* **One JS thread and one realm.** Asynchronous work runs on native threads, and its results come back as macrotasks on the main loop, which keeps browser ordering semantics.
* **Script loading:**
  * The host reads `index.html`. It references only `js/main.js` plus CSS [V].
  * `main.js` creates `<script>` elements. For each one, the host stub reads the file and evaluates it as a classic global script under its real filename (for `currentScript` and stack traces), then fires `onload` as a macrotask.
* **Load-time hooks:** "after script X evaluated" hooks install optional fast paths and gap fixes. Example: `pako.deflate/inflate` → native zlib (optional). They live in the host, never in the game.
* **No HTML parser.** The initial DOM is hardcoded: a `<body>`. Graphics creates `#gameCanvas`, `#errorPrinter` and `#loadingSpinner` itself [V].
* **Process and CLI:** one executable, `v8mz <gameDir> [--save-dir D] [--replay F] [--record F] [--frames N] [--screenshot-at LIST] [--trace] [--gl=native|angle]`. Steam launch option: `/path/v8mz "$(dirname %command%)"`, or a wrapper script [G: the exact Steam integration is to be confirmed, Q8].

---

## 4. Main loop, event loop and frame timing

### 4.1 Event-loop semantics (Chromium-compatible enough)
Each loop iteration:
1. Poll SDL events and dispatch each DOM event as a macrotask: keyboard, mouse, gamepad connect, focus/blur, `visibilitychange`, resize, close. Run a microtask checkpoint (`isolate->PerformMicrotaskCheckpoint()`; the policy is `MicrotasksPolicy::kExplicit`) after each.
2. Drain the native completion queue (image or audio decode done, file read done), one macrotask each, with a checkpoint after each.
3. Run expired `setTimeout`/`setInterval` callbacks, with HTML clamping (nesting level >5 gives a ≥4 ms minimum) and a checkpoint after each.
4. **Rendering opportunity.** If a vsync is due, run every rAF callback that was registered before this frame, all with the same `DOMHighResTimeStamp` (the frame start). Run a checkpoint, then `SDL_GL_SwapWindow`.
5. Wait on vsync or events until the next deadline.

* Promise jobs come from V8's microtask queue under `kExplicit`. **Never** run a checkpoint re-entrantly from inside a native call.
* Each iteration also calls `v8::platform::PumpMessageLoop(platform, isolate)` until it returns false. This runs V8 foreground tasks such as async WebAssembly compile results and GC tasks (§5.1).
* `eval`, indirect eval and `new Function` are V8 built-ins, exactly as in NW.js.

### 4.2 Frame timing
* PIXI's `Ticker` computes `deltaTime = elapsedMS × 60/1000`, clamped by `minFPS` (default 10) and `maxFPS`, which OptionsCore may set.
* rmmz `SceneManager.determineRepeatNumber` smooths `deltaTime` and runs **`Math.round(smooth)` logic updates per render**, with `deltaTime` capped at 2 [V `rmmz_managers.js:1993`].
  * At ~30 fps the game therefore runs **2 `updateMain`s per frame**. With V8 the logic cost is Chrome-like, so this is no longer a structural risk (§7.1). Still measure p95, not just the mean.
  * Below 30 fps the game runs in slow motion, which is also how NW.js behaves.
* rAF timestamps come from a monotonic clock sampled at vsync, and `performance.now()` uses the same clock.
* High-refresh displays (120/144 Hz) are handled by rmmz itself (the `smooth < 0.9` branch), so vsync at any rate is fine [V].
* **Deterministic mode** (`--replay`, conformance):
  * rAF timestamps advance by exactly 1000/60 ms, so `deltaTime` is exactly 1 and there is exactly one `updateMain` per frame;
  * `Date.now` and `performance.now` are virtualised;
  * `Math.random` is seeded (§10).
* **Background windows.** Chromium throttles rAF when a window is hidden; NW.js does not by default [G]. Follow NW.js: no throttling, audio keeps playing, and CaeF_PauseFix's `setInterval` keeps working.

---

## 5. Module specifications

### 5.1 JS host (embedded V8)

**Engine and version**
* V8, built from source with gn/ninja as a static monolith (build details in §11.2).
* **Pin a current stable Chrome branch**, not NW.js's 11.7, which is three years old and has security fixes missing.
  * NW.js 0.80 ships Chrome 117, i.e. V8 11.7 [V `nw_versions.txt`; the 117 ↔ 11.7 mapping is G from Chrome's numbering].
  * This ES5/ES2015-era game code has no V8-version-sensitive behaviour that we know of [G]. The conformance replay (§10) is the check.
* Embedding docs: https://v8.dev/docs/embed

**Process model**
* One `v8::Platform` (`platform::NewDefaultPlatform`). Its worker threads handle concurrent GC, Sparkplug/Maglev/TurboFan compiles and wasm tier-up.
* **One `Isolate` and one `Context`** = one realm. That matches a single NW.js window.
* The main loop must call `platform::PumpMessageLoop(platform, isolate)` every iteration. Without it, foreground tasks such as finalisation and the async `WebAssembly.instantiate` completion never run.

**Microtasks**
* `isolate->SetMicrotasksPolicy(MicrotasksPolicy::kExplicit)`.
* Run `isolate->PerformMicrotaskCheckpoint()` after every macrotask (§4.1). This is exactly the HTML ordering.
* `SetPromiseRejectCallback` feeds `unhandledrejection`.

**Scripts**
* Each `<script>` is compiled with `ScriptCompiler::Compile` and a `ScriptOrigin` carrying the real file URL. Stacks and `document.currentScript` then match NW.js.
* `eval`/`new Function` (1,302 calls) and `Function.prototype.toString` (Mimosa re-eval) are native V8 behaviour.
* `Error.stack` format and `Error.captureStackTrace` are identical to NW.js, which VisuMZ/Art error screens rely on.

**Host objects**
* Host classes are built with `FunctionTemplate`/`ObjectTemplate` and `SetInternalFieldCount(1)` pointing at the C++ object.
* `Global::SetWeak` finalisers release GL names and canvas surfaces.
  * Also call `AdjustAmountOfExternalAllocatedMemory` for big CPU canvases and textures, so GC pressure is realistic.
* Pixels and audio are zero-copy: `ArrayBuffer::NewBackingStore(ptr, len, deleter)` exposes `ImageData.data`, `getImageData` and `AudioBuffer.getChannelData`.

**Hot bindings**
* Plain `FunctionTemplate` callbacks cost ~20–60 ns each [G]. At ~870–4,600 GL calls per frame that is <0.3 ms.
* V8's **Fast API calls** (`CFunction`) for `uniform*`, `bindTexture`, `bindBuffer` and `drawElements` are an optional M6 tweak, not a requirement.

**Web APIs V8 does not provide** (V8 is only ECMAScript plus WebAssembly). The prelude and native code must supply:
* `console`: a full object implemented natively. V8's built-in `console` methods are no-ops unless an inspector or console delegate is attached [G];
* `TextDecoder`/`TextEncoder` (effekseer, 369 calls);
* `atob`/`btoa`;
* `structuredClone` (via `ValueSerializer` if ever needed);
* `queueMicrotask`;
* `performance`;
* all of DOM/BOM, `fetch` and timers.

**`Intl`/ICU (optional; tradeoff below)**
* **With ICU** (`v8_enable_i18n_support=true`, recommended):
  * `toLocaleString` (344 calls) and `Date` locale strings behave exactly like Chrome, with no polyfill.
  * The cost is ICU code plus data: the Chromium ICU data file is ~10 MB [G]. For comparison, the box's distro ICU is 38 MB [M: `libicudata.so.76` 31.9 MB + `i18n` 3.8 MB + `uc` 2.2 MB].
  * An en-only filtered data file is possible, at ~1–3 MB [G].
* **Without ICU:** saves ~10–15 MB, but the game then needs the en-US `toLocaleString` polyfill from v0.1.
* **Recommendation:** ICU on, with Chromium's data, for exact parity. Revisit in M6 packaging.

**Code cache** (optional, M6)
* `ScriptCompiler::CreateCodeCache` / `kConsumeCodeCache`, keyed by the file's sha256 and stored in `$XDG_CACHE_HOME/v8mz/<gameid>/`, **never in the game dir**.
* Gain [M]: compiling the 7 largest scripts took 35 ms (Node, lazy compile). Startup-only.

**Debugging (optional, M6):** `v8-inspector` gives a real Chrome DevTools connection (`--inspect`), a big win for plugin debugging. It costs ~1–2 pw [G].

**Prelude** (JS, ~1.5–3k LOC, partly adapted from Outsider's shims, §13.1), installed before `main.js`:
* `window === globalThis`, `self`, `document`.
* `navigator`: `userAgent` set to NW.js 0.80's Chrome/117 string, plus `languages`, `platform` and `getGamepads`.
* `location` as a `file:///…/index.html` URL.
* Node and NW objects:
  * `process`, with `versions = {node: "20.5.1", nw: "0.80.0", chromium: "117.0.5938.63"}` [V], `platform`, `env`, `hrtime`, `cwd()`;
  * `require` for `fs`, `path`, `os`, and a `child_process` stub;
  * `nw`.
* Error routing: uncaught exceptions (`isolate->AddMessageListener`) and rejections go to `window.onerror`/`unhandledrejection`, so rmmz `SceneManager.catchException` and Art_VerboseErrorScreen behave as in NW.js.

### 5.2 WebGL2 shim over GL 3.3 core / GLES 3.0

**Choice:** a **1:1 binding**. Each WebGL call maps to its GL call plus a validation subset, over a native context. There are two backends behind one C ABI:
* **(a) Native driver GL**, as desktop 3.3 core or GLES 3.0 via SDL. This is the default on Linux (Mesa and proprietary drivers).
  * GLSL ES 1.00/3.00 has to be translated with a **small, deterministic source rewriter**. It sets the `#version` line, maps `attribute`/`varying`/`texture2D`/`gl_FragColor`, and handles precision qualifiers.
  * The fallback translator is glslang + SPIRV-Cross.
  * **34 shaders were captured** (pixi GLSL ES 1.0, UltraMode7, and Effekseer's 22) in `trace/run2/shaders/`. They form the regression corpus [M].
* **(b) ANGLE (BSD-3)** gives Chromium-identical GLSL validation, precision, and blend/stencil behaviour. It is recommended for Windows and macOS, and as a conformance oracle on Linux. The cost is a big dependency with a heavy build (depot_tools/gn).

**Required surface:**
* The 72 traced methods (`scan/dynamic_api_counts.tsv`), plus the rest of the WebGL2 entry points PIXI can reach on untraced paths (`scan/webgl_calls.tsv`, 97 identifiers).
* Hot calls over the run [M]:
  * `uniformMatrix3fv` 412k, `bindTexture` 287k;
  * `bindBuffer` and `bufferSubData` 191k each, `drawElements` 171k;
  * `bindVertexArray` and `useProgram` 163k each, `bindFramebuffer` 152k;
  * `viewport` 146k, `uniform4f` 110k, `clear` 78k;
  * `stencilFunc` and `stencilOp` 11k each.
* Per-frame load in Chrome [M]:
  * Map 92 averages **~870 GL calls per frame**, peaking around 3,100;
  * the title screen averages about 76.
  * The real-PIXI bench issued 1,491–4,643 calls per frame, rising with the number of filters.
* Binding overhead target: <100 ns per call, i.e. <0.5 ms per frame.

**Semantics that need care:**
* **Context attributes** as PIXI requests them (`stencil: true`, `antialias`, `premultipliedAlpha`, `preserveDrawingBuffer`). Report them honestly through `getContextAttributes()`.
* **Texture uploads from host objects.** `texImage2D`/`texSubImage2D(…, source)` take `HTMLImageElement`, `HTMLCanvasElement`, `ImageData` or `HTMLVideoElement` sources.
  * This needs a direct path from host object to pixel pointer (internal field → native surface).
  * It must honour `UNPACK_PREMULTIPLY_ALPHA_WEBGL` and `UNPACK_FLIP_Y_WEBGL` from `pixelStorei` (365 calls) [M].
  * Canvas sources upload the CPU raster. rmmz calls `baseTexture.update()` after every `Bitmap` draw, so keep a dirty generation counter to avoid redundant uploads.
* **Object model.**
  * `WebGLTexture`, `Buffer`, `Program`, `Shader`, `Framebuffer`, `VertexArrayObject` and `UniformLocation` are JS objects wrapping GL names, freed by finalizers.
  * `getActiveUniform`/`getActiveAttrib` must return WebGL-style names (e.g. `uSamplers[0]`) and WebGL type enums.
* **Stencil.** VisuMZ_0_CoreEngine calls `gl.stencilFunc`/`stencilOp`/`clearStencil` directly, around PIXI's own state [V/M].
  * The default framebuffer and PIXI's render targets need depth24-stencil8 (`renderbufferStorage` was traced).
  * Stencil behaviour across PIXI's framebuffer pool must match Chrome.
* **`readPixels`** (`Bitmap.snap`): flip Y, and match Chrome's premultiplication handling.
* **`isContextLost()`** (2,947 calls) always returns false. There is no context-loss simulation.
* **WebGL1 probe.** CGMZ_Core creates a separate `webgl` context to read the GPU name. Serve it from the same backend with its own state shadow; `WEBGL_debug_renderer_info` may return a generic string.
* **Extensions.** Advertise only what is backed. On WebGL2 there were 18 `getExtension` calls [M].

### 5.3 Canvas2D subset (CPU raster)

**Why a CPU raster:**
* rmmz `Bitmap.getPixel`/`getAlphaPixel` and PIXI make 88 `getImageData` calls.
* 577 contexts were created; `Bitmap` is the source of truth, so a GPU canvas would need constant readbacks.

**Representation:**
* RGBA8 *premultiplied*, as Skia stores it.
* `getImageData` un-premultiplies exactly as Chrome does (the same lossy rounding), which matters for pixel diffs.

**Surface:**
* Everything in the dynamic table, plus static extras:
  * transforms: `setTransform`, `translate`, `rotate`, `scale`;
  * `clip`;
  * paths: `moveTo`, `lineTo`, `arc`, `arcTo`, `quadraticCurveTo`, `bezierCurveTo`, `rect`, `closePath`;
  * `fill`/`stroke` with nonzero and evenodd rules;
  * line styles: `lineCap`, `lineJoin`, `miterLimit`;
  * `imageSmoothingEnabled`;
  * gradients.
* Compositing: `source-over` and `lighter` at minimum, plus whichever others actually occur [G: enumerate in M1].
* `shadow*` is in the static list only.

**Library options:**
* **A. canvas_ity** (ISC, single header, gamma-aware): the quickest start.
* **B. PlutoVG** (MIT, C).
* **C. Blend2D** (Zlib, JIT).
* **D. Skia** (BSD-3): matches Chrome exactly, but the build is heavy.
* **Recommendation:** start with A or B behind a backend interface, and swap in Skia only if the text and AA diffs prove unacceptable [G].

**Text** (the hardest part of this module):
* CSS `font` shorthand parsing, e.g. `"bold 28px rmmz-mainfont, sans-serif"`, including decimal sizes.
* Shaping with HarfBuzz; rasterising with FreeType using Chrome-like light hinting and grayscale AA.
* `measureText().width` must match Chrome to about ±0.5 px per string, because rmmz and MessageCore wrap and align text on it.
  * A wrong width gives visibly different line breaks, but not different logic, unless a plugin stores widths in state [G: believed none].
* `strokeText`, with `lineJoin='round'` and `lineWidth` 3–8, draws rmmz's outlined text on every UI label. Use FreeType's stroker or the raster library's path stroker.
* `textBaseline` metrics must use the same OS/2-versus-hhea source as Chrome on Linux [G].
* Glyph fallback: the 8 bundled fonts plus Noto as a system fallback.

### 5.4 Image decode
* Format: PNG only. There are 2,313 files, none encrypted [V].
* Decoder: stb_image, or libpng + zlib.
* **Decode flow:**
  * Decode on a worker thread.
  * Complete as a macrotask that sets `width`, `height`, `naturalWidth`, `naturalHeight` and `complete`, then fires `onload`.
  * `onload` ordering need not match Chrome, since rmmz polls `ImageManager.isReady()`.
* **Colour management.** Chrome colour-manages PNGs that carry gAMA/iCCP chunks. Match it or ignore those chunks consistently (Q5).

### 5.5 Fonts
* **How the game loads fonts:** `FontManager.load` calls `new FontFace(name, "url(...)")` twice, then `document.fonts.add`, then waits on `document.fonts.ready` [M]. There are 8 font files in `fonts/` [V].
* **Native registry:** maps family name to a FreeType face, with CSS-style weight/style matching. When a face is missing, it uses synthetic bold and oblique, as Chrome does.
* **`FontFace.load()`** resolves after the file has been read and parsed.

### 5.6 Audio (WebAudio subset)

**Nodes in use** [M+V]:
* `AudioContext` ×2 (rmmz and Effekseer), with `destination`, `currentTime`, `state`, `resume` and `suspend`.
* `GainNode`: `gain`, `setValueAtTime`, `linearRampToValueAtTime`.
* `PannerNode`: `panningModel = 'equalpower'`, `setPosition`.
* `AudioBufferSourceNode`: `buffer`, `loop`, `loopStart`/`loopEnd` (from the Ogg `LOOPSTART`/`LOOPLENGTH` comments rmmz parses itself), `playbackRate` (pitch), `start(when, offset)`, `stop`, `onended`.

**Engine:**
* A sample-accurate mini-graph rendered in miniaudio's device callback at 48 kHz, in 128-frame render quanta as Chrome does.
* Parameter automation uses the WebAudio spec formulas.
* JS talks to the audio thread through a lock-free command queue. `currentTime` is the device clock.

**`decodeAudioData(arrayBuffer) → Promise<AudioBuffer>`:**
* Ogg Vorbis via stb_vorbis (public domain), or libvorbis for closer parity [G: Chrome's FFmpeg Vorbis output and these decoders differ by about 1 LSB].
* **Opus in Ogg or WebM** via libopus plus a demuxer (libogg; nestegg for WebM), for `police radio.ogg`. Sniff by magic bytes, not by extension.
* **Truncated input must be accepted** (§2.3 #6).
* Resample to the context rate with a high-quality resampler. `AudioBuffer.duration` is read by CaeF_PauseFix and rmmz.

**`fetch()` + `body.getReader()`:**
* rmmz streams files and decodes progressively.
* For local files the host may deliver the whole body as **one chunk**, which is legal stream behaviour and gives a single decode per file.
* Keep 64 KB chunking as an option for fidelity tests.

**vorbisdecoder.js** loads but is never used for decoding, because `canPlayType('audio/ogg; codecs="vorbis"')` returns "probably" [V].

### 5.7 Video (NIJI_VideoPlayer and the stock `Video`)
* **Content:** 2 clips, both **WebM VP9 + Opus** [M]. They play for the game over (CE127) and fishing scenes [V]. MP4 siblings exist but go unused.
* **`HTMLVideoElement` subset:**
  * properties: `src`, `autoplay`, `muted`, `volume`, `loop`, `currentTime`, `duration`, `ended`, `paused`, `readyState`, `videoWidth`, `videoHeight`;
  * methods: `play()` (returns a Promise) and `pause()`;
  * events: `canplay`, `canplaythrough`, `loadeddata`, `ended`, `error`.
* **Decision (Q3 resolved, the owner, 2026-10-30): play the shipped `.webm` files directly. No transcoding and no transcode cache.**
* **Decode pipeline:**
  * Demux with nestegg, decode VP9 with **libvpx** (BSD-3), and turn YUV420 into a texture with a GPU shader.
  * PIXI's `VideoResource` uploads frames via `texImage2D(video)`.
  * Opus audio goes into the mixer, and A/V sync follows the audio clock.
* **PIXI readiness:** PIXI 5's `VideoResource` gates on `readyState` and the `canplay`/`canplaythrough`/`error` events, then updates on its ticker. These state transitions must be emulated faithfully.
* **Stock `Video`:** MZ overlays a `<video>` element via the DOM. The host draws it as a full-window quad after the GL frame.
* **Rejected alternatives:**
  * dav1d (BSD-2) is AV1-only, so it does not apply.
  * FFmpeg (LGPL) covers everything but is heavy.
  * pl_mpeg (MIT) needs transcoding, which is ruled out (Q3 resolved: libvpx, no transcode cache).

### 5.8 Input
* **Keyboard:** translate SDL keycodes to the legacy DOM `keyCode` values, because rmmz `Input.keyMapper` keys on `keyCode`.
  * Also fill `key`, `code`, `charCode` (CoreEngine reads it on keypress [M]), the modifier flags and `repeat`.
  * Dispatch `keydown`/`keyup`/`keypress` on `document`, using SDL text events for typed characters.
* **Gamepad:** `navigator.getGamepads()` is polled every frame [M].
  * Return standard-mapping `Gamepad` objects (buttons 0–15, axes 0–3), as rmmz `Input.gamepadMapper` expects. SDL3's gamepad API maps onto this directly.
  * Fire `gamepadconnected` and `gamepaddisconnected`.
* **Mouse:** mouse and pointer events (down, up, move, wheel) for PIXI's InteractionManager and rmmz `TouchInput`.
  * Map `pageX/Y` and `clientX/Y` through the letterbox transform.
  * Mimosa_MouseCursor sets a custom CSS cursor; implement it via `SDL_CreateColorCursor` [G].

### 5.9 Files, saves, XHR and fetch
* **XHR and fetch URLs** resolve relative to the game root and are percent-decoded (e.g. `police%20radio.ogg`).
  * Status codes: rmmz only checks `xhr.status < 400` [V], so returning 200 is safe.
  * `overrideMimeType` and `responseType = 'arraybuffer'` are supported.
* **Node `fs` subset** (sync): `existsSync`, `readFileSync` (string or Buffer), `writeFileSync`, `mkdirSync({recursive})`, `readdirSync`, `unlinkSync`, `renameSync`, `statSync`.
  * Strings are written and read as **UTF-8, exactly as Node does** (§9.2).
  * A minimal `Buffer` is needed.
* **`path`:** a JS polyfill with `dirname`, `join`, `basename`, `resolve`, `sep`.
* **IndexedDB:** localforage initialises it at load (~20 calls) [M], but rmmz uses files whenever `Utils.isNwjs()` is true. An in-memory IDB is enough, with a JSON-file fallback if it is ever used.
* **localStorage:** file-backed.

### 5.10 `nw`, window and DOM stubs

**`nw` stubs:**
* `nw.App`:
  * `argv` is `[]`, or passes through args given after `--`;
  * `quit()` does an orderly exit.
* `nw.Window.get()`:
  * `on('close', cb)`: the window manager's close button calls `cb`, as NW does. The game then calls `nw.App.quit()` itself;
  * `close(true)`;
  * `enterFullscreen`, `leaveFullscreen`, `isFullscreen`;
  * `resizeTo`, `moveTo`;
  * `showDevTools` is a no-op.
* `nw.Shell.openExternal(url)` calls `xdg-open`, after asking the user to confirm.
* CleanExit's win32 `taskkill` branch is skipped on linux [V].

**Mini DOM tree:**
* `createElement`, `appendChild`, `removeChild`, `getElementById`, `getElementsByTagName`, `setAttribute`, `textContent`, `classList`, `remove`.
* `style` is a plain object. Graphics writes the letterbox size and margins into it, and the host reads the canvas's style to place the GL viewport.
* `innerHTML` is stored as a string. Art_VerboseErrorScreen puts its error page there; the host shows it as a text overlay or logs it.

**Window:**
* `innerWidth` and `innerHeight` come from the SDL window, which fires `resize`.
* `MutationObserver` is an inert stub.

### 5.11 Effekseer, wasm and Worker quirks

**Requirement:** `effekseer.min.js` (Emscripten) loads `effekseer.wasm` via XHR and `WebAssembly.instantiate`. `main.js` treats a failed `effekseer.initRuntime` as fatal [V].

**Approach: V8's built-in WebAssembly.**
* `effekseer.wasm` runs as shipped (Liftoff → TurboFan), and effekseer's GL calls go through the same WebGL shim. There is no WAMR or wasm3 dependency.
* `WebAssembly.instantiate` resolves through platform tasks, which is why the message pump in §5.1 is needed.
* `WebAssembly.Memory.grow` buffer detachment is handled by V8.

**Emscripten environment detection:**
* If `process.versions.node` exists, Emscripten takes its Node path, e.g. `require('fs')` for the wasm [V].
* NW.js exposes both `window` and `process`, so trace the reference behaviour under real NW.js and match it (§15 Q2).
* If needed, the host can hide `process.versions.node` while evaluating `effekseer.min.js` and `vorbisdecoder.js`, then restore it. This is a host hook, not a game edit.

**Interim stub (M0–M1 only):** after `effekseer.min.js` evaluates, the host may swap in a stub `effekseer` object. `initRuntime` succeeds and `play` draws nothing. Real Effekseer is expected from M2, because wasm comes free with V8.

**Jitless builds (iOS/console-style policies):** `--jitless` **disables WebAssembly entirely** [M: under `node --jitless`, `typeof WebAssembly` is `undefined`].
* There, Effekseer would need V8's wasm interpreter (DrumBrake; build flag and maturity unverified [G]) or Outsider's native Effekseer SDK path (`src/effects/effekseer_backend.cpp`, §13.1).
* Not needed for the Linux/desktop target.

**`new Worker(document.currentScript.src)`** from vorbisdecoder returns an inert Worker object. It is never messaged on this game's path [V].

---

## 6. V8-embedding gaps versus NW.js (what V8 alone does not give us)

The QuickJS gap table from v0.1 is gone. With V8, the language engine is the *same* engine family NW.js uses, so `Intl` (with ICU), `Error.stack`, function source, `Math.*` results and property order all match.

| Gap | Impact here | Mitigation |
|---|---|---|
| No Web/DOM APIs in V8: `console`, timers, `TextDecoder`, `atob`, `fetch`, `XMLHttpRequest`, DOM, `performance`, `structuredClone`, `queueMicrotask` | Everything outside ECMAScript | Prelude + native (§5.1, §5.9, §5.10) |
| No event loop or message pump of its own | Async wasm compile, finalisers, GC tasks | `PumpMessageLoop` + explicit microtask checkpoints (§4.1, §5.1) |
| No Node built-ins (`require('fs')`, `Buffer`, `process`) | Saves, CleanExit, ChangeSaveLocation | Native fs subset + small `Buffer` (§5.9). We do **not** embed Node itself (§11.2) |
| V8 version drift from NW.js's 11.7 | Minor: newer built-ins and different perf heuristics | Conformance replay (§10); optionally pin 11.7 for reference runs only |
| `Math.sin/cos/exp/pow` across V8 versions | V8 ships its own `ieee754` routines, so results should be identical to Chrome on every platform [G: source detail not re-verified] | State-hash replay checks it (§10) |
| Jitless mode disables WebAssembly [M] | Effekseer on iOS/console-style targets | §5.11 alternatives; not a desktop concern |

---

## 7. Performance

### 7.1 Measurements [M]

Measured on the box (8-vCPU Intel Xeon VM). Node 20.19 (V8 11.3.244.8) is used as the V8 proxy. QuickJS-ng figures are kept for the record.

| Benchmark | V8 (JIT) | V8 `--jitless` | QuickJS-ng (v0.1) |
|---|---:|---:|---:|
| **Real PIXI 5.3.12 `renderer.render()`**: 900 sprites in 15 containers, 40 textures, no filters (`bench/render_bench.js`, no-op WebGL2) | **0.19 ms/frame** | 2.76 | 5.05 |
| same + 40 AlphaFilters | **0.30** | — | 7.32 |
| **same + 116 AlphaFilters** (≈ Map 92) | **0.56** | 6.91 | 12.46 |
| PIXI transform + vertices + bounds only, 1,200 sprites | 0.18 | — | 5.70 |
| `JSON.parse` CommonEvents.json (5.9 MB) | 30.5 ms | — | 71.8 |
| `JSON.parse` Map039 (727 KB) | 3.5 | — | 10.1 |
| compile of the 7 largest scripts (`new Function`; lazy) | 34.8 | — | 90.6 |

**Re-measured today for saves** (2.5 MB save JSON, median of 10; `/tmp/pz.js`):

| Step | Time |
|---|---:|
| `pako.deflate` level 1, `to:'string'` | 28.1 ms |
| `pako.inflate`, `to:'string'` | 62.4 ms |
| native `zlib.deflateSync` level 1 | 8.8 ms |
| native `zlib.inflateSync` | 6.2 ms |
| `JSON.stringify(JSON.parse(save))` | 16.4 ms |
| UTF-8 write of the 129,541-char binary string | 1.0 ms |

**Whole-frame reference, Chrome 156 headless rAF callback time** (`trace/run3_noshim`) [M]:

| Scene | Mean | p95 | Max |
|---|---:|---:|---:|
| Title | 0.55 ms | — | — |
| Map 92 | 2.57 ms | 8.1 ms | 120 ms (scene changes) |
| Idle map | 2.42 ms | 8.2 ms | — |

**Projected Map 92 CPU time on the V8 host [G]:**
* JS: ≈ Chrome's 2.5–3 ms mean, 8 ms p95.
* Native Canvas2D raster and glyphs: +0.5–2 ms on text-heavy frames. This is a CPU raster versus Chrome's Skia, and it is the most uncertain part.
* GL submission: +0.5–1.5 ms. It is direct GL with no GPU-process IPC, which is probably *cheaper* than Chromium [G].
* **Total: ≈3–6 ms mean, ≲12 ms p95, so 60 fps has comfortable headroom.**
* The 2-updates-per-frame trap (§4.2) is no longer a structural risk.

### 7.2 Optional optimisations (none is on the critical path)
1. **Native zlib for saves.** Under V8, pako costs ~28 ms to save and ~62 ms to load. Native zlib costs ~9 and ~6 ms, and its output is **byte-identical** at level 1 [M].
   * This would remove one ~20–55 ms hitch per save or load.
   * It is a nice-to-have. **Default: off.** Turn it on only if save or load hitches are noticed, because the hook is a prototype override and adds a little conformance risk.
2. **Native PIXI hot paths** (`calculateVertices`, `packInterleavedGeometry`, FilterSystem / ColorFilter): **not planned.** PIXI is 0.56 ms per frame on V8.
3. **V8 Fast API calls** for the hottest GL entry points; startup **code cache**; the **inspector** for profiling. All are M6 polish.
4. **Profile Canvas2D text**, the likely hotspot. Glyph caches, keyed by font and size, are a normal part of the Canvas2D module, not an extra.

### 7.3 Jitless (iOS/consoles): an honest note
* V8 `--jitless` was **×12–15 slower than JIT** on the PIXI bench (2.76 / 6.91 ms) [M].
* rmmz and plugin logic would slow down by a similar or larger factor [G].
* Map 92 would likely land at **25–60 ms per frame**, i.e. below 60 fps, and **WebAssembly is unavailable** [M].
* A jitless target would therefore bring back the native fast-path work from v0.1 (§7.2 #2) and the Effekseer alternative (§5.11).
* Separately, **GPL-3.0 is generally incompatible with console SDK and App Store distribution terms** [G, not legal advice].
* **Conclusion:** consoles and iOS stay out of scope (§1).

### 7.4 Memory
* V8 heap plus host: target <1 GB RSS. Chrome uses ~400–700 MB for MZ games of this size [G].
* Textures dominate: 2,313 PNGs, loaded on demand. Report external memory to V8 (§5.1).

---

## 8. Module boundaries

| Module | Language | Boundary | Depends on |
|---|---|---|---|
| `host_core` | C++17 | `v8mz_run(gameDir, opts)`, main loop, timers, microtasks, platform pump, script loader, hooks | V8, SDL3 |
| `dom_prelude` | JS (embedded) | `window`, `document`, `navigator`, `nw`, `process`, `require`, polyfills | host_core natives |
| `webgl` | C++ | context create, 1:1 entry points, object table | glad loader; ANGLE optional |
| `glsl_rewrite` | C | `glsl_es_to_target(src, stage, target)` | none (fallback glslang/SPIRV-Cross) |
| `canvas2d` | C/C++ | surface + 2D state machine; pixel access for uploads | raster library, FreeType, HarfBuzz |
| `image` | C | async PNG decode → RGBA | stb_image or libpng |
| `fonts` | C | registry, CSS font parse, face matching (FreeType reads the 2 `.woff` fonts directly) | FreeType (+zlib) |
| `audio` | C | graph command queue; `decode(bytes) → PCM` | miniaudio, libvorbis/stb_vorbis, libopus, libogg, nestegg |
| `video` | C | open/step/frame-texture/audio tap | nestegg, libvpx, libopus |
| `input` | C | event translation; gamepad poll | SDL3 |
| `io` | C | fs subset, URL→path, XDG dirs, file fetch/XHR | — |
| `fastpath` (optional) | C | native zlib override for pako | zlib |
| `conformance` | Python/JS | record/replay, screenshots, state hashes, save diff | NW.js/Chrome via CDP (adapted from Outsider `tools/evaluator`) |

The binding glue lives in `host_core/bind_*.cc`. It wraps each C module with V8 templates, so the C modules themselves stay engine-free and testable on their own.

---

## 9. Files and save I/O

### 9.1 Paths
* **Game root:** read-only. The host never writes there. CI runs it on a read-only bind mount.
* **Saves:** the game's own plugin decides the folder. `Advanced_ChangeSaveLocation` on linux uses `process.env.XDG_DATA_HOME || $HOME/.local/share`, plus `HIROTONFA_AmericanRevolutionSmuggler`. The result is **`~/.local/share/HIROTONFA_AmericanRevolutionSmuggler/{fileN,global,config}.rmmzsave`** [V/M]. The host only has to be truthful about `process.platform`, `env` and `fs`.
* **Contrast with Outsider:** its `storage_shim.js` writes a relative `save/` folder, and its engine writes `.jsc` files next to the scripts. We do neither (§13.1).
* **Existing Proton/Windows saves** live in the Proton prefix (`…/AppData/Roaming/HIROTONFA_AmericanRevolutionSmuggler/`). Offer an import tool that **copies** them and never moves or deletes them.
* **Host-owned data:** `$XDG_CACHE_HOME/v8mz/` for the V8 code cache, and `$XDG_STATE_HOME/v8mz/` for logs, replays and screenshots.

### 9.2 Save-format compatibility [M]
* **Write path:** `StorageManager.jsonToZip` produces `pako.deflate(json, {to: "string", level: 1})`, a JS *binary string* with chars 0–255. NW writes it with `fs.writeFileSync(path, string)`, i.e. **UTF-8-encoded**, so every char ≥0x80 becomes 2 bytes.
  * Example: file1 is 129,541 chars, which is **193,350 bytes on disk** from 2.56 MB of JSON.
* **Read path:** `readFileSync(path, {encoding: 'utf8'})`, then `pako.inflate(…, {to: 'string'})`.
* **Requirement:** the host's fs must reproduce Node's UTF-8 encode and decode exactly.
* `pako.deflate` level 1 output is **byte-identical to zlib `deflateSync` level 1** on this save. So the optional native-zlib fast path (§7.2) would keep saves byte-identical. By default the shipped `pako` runs on V8, which takes ~28 ms to save and ~62 ms to load [M].
* **Save diff:** under the same replay, with `Date.now` virtualised in both runtimes, the `.rmmzsave` bytes must be identical.

---

## 10. Conformance testing

### 10.1 Reference harness (no game edits)
* **Implementation:** adapt Outsider's `tools/evaluator` (Python CDP driver, scenarios and comparison; GPL-3, §13.1), and add the seeding, virtual time and hashing below.
* **Reference runtime:** run the game in **NW.js 0.80.0**, or in headless Chrome with the same NW shims this session used (`hostspec/trace_shim.js`, `drive.js`).
* **Injection, without touching game files:**
  * a CDP `Page.addScriptToEvaluateOnNewDocument`, as done this session with puppeteer;
  * or NW's `inject_js_start`, given in a *copy* of `package.json` outside the game folder.
* **The same harness runs in both runtimes.** It:
  * seeds `Math.random` with xorshift128+, using the same state in both runtimes;
  * virtualises `Date.now`, `performance.now` and rAF timestamps to exact 1/60 s steps;
  * records and replays input as `[frame, type, code, value]`;
  * every N frames, hashes `$gameSystem`, `$gameMap`, `$gamePlayer`, `$gameVariables`, `$gameSwitches`, `$gameSelfSwitches` and `$gameParty` (JSON → sha1);
  * takes screenshots at the listed frames.

### 10.2 Test suites
1. **Logic determinism.** Replay identical inputs; the state hashes must match every 60 frames.
   * Route: title → New Game → Map 92 walk → menu → save → load → a choice event → a minigame → a scene with LightingEffects.
   * Duration: 10+ minutes.
2. **Save diff.** `cmp` the `.rmmzsave` bytes after the same replay.
3. **Screenshot diff.** Compare `readPixels` at the same frames (ΔE and SSIM).
   * Thresholds [G]: non-text regions ≤0.5% of pixels off by more than 2/255; text regions SSIM ≥0.97.
   * Mode7 maps get their own suite.
4. **API unit tests:**
   * WebGL: a selected subset of the Khronos WebGL conformance suite, plus PIXI v5's own tests running on v8mz.
   * Canvas2D: a subset of WPT `html/canvas/element`.
   * WebAudio: offline-render comparisons against Chrome's `OfflineAudioContext`, within tolerance.
5. **Shader corpus.** All 34 captured shaders plus Mode7 must compile and link on the GL 3.3, GLES 3.0 and ANGLE backends.
6. **Read-only guard.** Run with the game dir mounted read-only. The game must run, and nothing may be written there.
7. **Performance check.** A scripted 2-minute walk on Map 92, recording frame-time p50/p95/p99 and the share of frames with 2 updates. M2 needs p95 ≤16.7 ms.

---

## 11. Build, dependencies and licenses

### 11.1 Dependencies and GPL-3.0-or-later compatibility

The host is licensed **GPL-3.0-or-later** (owner, 2026-10-30).

| Dependency | Use | License | GPL-3 compatible? | Link |
|---|---|---|---|---|
| V8 | JS engine + WebAssembly | BSD-3 | ✅ | https://github.com/v8/v8 (license: https://chromium.googlesource.com/v8/v8/+/refs/heads/main/LICENSE.v8) |
| ICU (inside V8's third_party) | `Intl`, `toLocaleString` | Unicode License v3 (ICU) | ✅ permissive [G: not re-checked against the FSF list this session] | bundled with the V8 checkout |
| SDL3 | window, GL context, input, gamepad | Zlib | ✅ | https://github.com/libsdl-org/SDL |
| ANGLE (optional backend) | Chromium-exact GLES | BSD-3 | ✅ | https://github.com/google/angle |
| glslang + SPIRV-Cross (fallback translator) | GLSL ES → target GLSL | BSD-3 (mostly) / Apache-2.0 | ✅ Apache-2.0 is GPL-**3** compatible (not GPL-2) [V FSF list] | https://github.com/KhronosGroup/glslang , https://github.com/KhronosGroup/SPIRV-Cross |
| canvas_ity / PlutoVG / Skia | Canvas2D raster | ISC / MIT / BSD-3 | ✅ | https://github.com/a-e-k/canvas_ity , https://github.com/sammycage/plutovg , https://github.com/google/skia |
| FreeType | glyphs, stroker, WOFF | FTL **or** GPLv2-or-later | ✅ Use under FTL, which is "compatible to the GNU GPL version 3" per FreeType's LICENSE.TXT [V]. FTL needs a credit line in the docs. Or take the GPLv2+ option | https://github.com/freetype/freetype |
| HarfBuzz | shaping, advances | MIT ("Old MIT") | ✅ | https://github.com/harfbuzz/harfbuzz |
| stb (stb_image, stb_vorbis) | PNG, Vorbis | MIT / public domain | ✅ | https://github.com/nothings/stb |
| libvorbis (+libogg) | exact Vorbis | BSD-3 | ✅ | https://github.com/xiph/vorbis |
| libopus | Opus (police radio, movies) | BSD-3 | ✅ | https://github.com/xiph/opus (canonical; not fetched) |
| nestegg | WebM demux | ISC | ✅ [V FSF: ISC compatible] | https://github.com/mozilla/nestegg |
| libvpx | VP9 decode | BSD-3 + Google patent grant (`PATENTS`) | ✅ The grant adds rights and does not restrict | https://github.com/webmproject/libvpx , https://github.com/webmproject/libvpx/blob/main/PATENTS |
| miniaudio | audio device, resampler | MIT-0 / public domain | ✅ | https://github.com/mackron/miniaudio |
| zlib | optional pako fast path; FreeType WOFF | Zlib | ✅ | https://zlib.net (canonical; not fetched) |
| Outsider (reused files, §13.1) | shims, platform, I/O, harness | GPL-3.0-or-later OR commercial | ✅ We take the GPL option | https://github.com/General-Arcade/outsider |
| Effekseer runtime | ships *inside the game* (`effekseer.min.js`/`.wasm`); not redistributed by us | MIT | n/a (game asset) | https://github.com/effekseer/Effekseer |

**Result:**
* **No incompatible dependency was found.**
* Things to watch:
  1. Never pull in a **GPL-2.0-only** library. None is listed.
  2. Keep FreeType's FTL credit, or pick its GPL option.
  3. Apache-2.0 parts are fine only because we are GPL-**3**.
  4. Keep Outsider's copyright headers, and mark modified files (GPLv3 §5a).
* Dropped since v0.1: quickjs-ng (MIT), WAMR (Apache-2.0+LLVM exception), wasm3 (MIT). They were all compatible anyway.
* Game files are never redistributed.

### 11.2 Building V8: pick one

| Option | How | Pros | Cons |
|---|---|---|---|
| **A. gn/ninja from source** (https://v8.dev/docs/build-gn) | depot_tools `fetch v8`; `gn gen` with `v8_monolith=true is_component_build=false v8_use_external_startup_data=false use_custom_libcxx=false v8_enable_i18n_support=true symbol_level=0`; `ninja v8_monolith` | Official path. Any version and flags (jitless, no-ICU, sandbox, pointer compression). Matches the toolchain we link with | Multi-GB checkout. ~30–90 min build on 8 cores [G]. Python/depot_tools toolchain. Fiddly on Windows and for cross builds |
| **B. Node's V8** (`libnode`, https://github.com/nodejs/node/blob/main/BUILDING.md) | Link a `--shared` Node build, or a distro `libnode`, and use only its V8 symbols | Familiar `./configure && make`. Distros ship it | Drags in the whole Node runtime: the box's `libnode.so.115` is **55 MB** [M]. Node's V8 version cadence. Two event loops in one process if misused |
| **C. rusty_v8 prebuilt static libs** (https://github.com/denoland/rusty_v8) | Download `librusty_v8.a` per platform and release | No V8 build at all. Covers linux x64/arm64, macOS and Windows | Built for Rust with Chromium's bundled libc++ (`std::__Cr`), so C++ ABI clashes with a host on system libstdc++ are likely [G]. Version tied to Deno |

**Choice: A**, done **once per platform in CI**.
* CI publishes a pinned tarball: headers + `libv8_monolith.a` + ICU data. The host's CMake downloads it by sha256, so contributors and distros never run gn.
* `-DV8MZ_V8_FROM_SOURCE=ON` re-runs option A for full-source builds.
* Option B is an acceptable **M0 bootstrap** on Linux [G] while the CI pipeline is being built.

**"Compile anywhere" caveat:**
* V8 supports x64, arm64 and several server/RISC-V ISAs, but not every exotic platform.
* Building it needs Clang. gn defaults to Chromium's clang; GCC builds are less tested [G].
* This is the main portability cost compared with v0.1's pure-C QuickJS.

### 11.3 Binary size [G unless marked]

| Component | Size |
|---|---|
| V8 monolith, stripped, in the final executable | ~25–35 MB (compare: box `libnode.so` 55 MB [M], which also contains Node) |
| ICU data | ~10 MB with Chromium's full data, ~1–3 MB filtered; or 0 without `Intl` |
| Media, graphics and fonts | ~5–10 MB |
| **Total distributable** | **~40–55 MB** (versus QuickJS's ~5–10 MB, and NW.js at a couple of hundred MB) |

### 11.4 Build and packaging
* **Build:** CMake ≥3.20 with C11/C++17. Dependencies are vendored as pinned submodules or FetchContent tags; V8 comes as a CI tarball (§11.2).
  * Toolchains: clang (required for V8) for the host, plus gcc, MSVC/clang-cl or Xcode clang.
* **Packaging:** one binary plus ICU data, with an optional AppImage/Flatpak and a Steam launch wrapper.
  * **The repo contains no game files.** Tests point at a local Steam install at run time.
  * Source must accompany binaries (GPL-3).
* **Steam:** CaeF_PauseFix optionally looks for `window.greenworks`. The host leaves it undefined [V]. No other Steamworks use was found [M scan].

---

## 12. Milestones and exit criteria

| Milestone | Scope | Exit criteria (real game, unmodified) | Effort [G] |
|---|---|---|---|
| **M0 Logs** | V8 build pipeline (CI tarball; libnode bootstrap allowed), host_core (isolate, context, microtasks, platform pump), prelude (adapted Outsider shims), script loader, XHR/fs reads, timers, rAF loop, console; WebGL/Canvas/Audio as logging stubs; Effekseer stub allowed | `main.js` and all 57 plugins evaluate with **zero uncaught exceptions**. `Scene_Boot` loads all `data/*.json`. The log shows `$dataSystem.gameTitle`. The API calls in the first 3 s form the same set as the NW.js trace | 3–5 pw |
| **M1 Title** | WebGL2 binding (backend a), GLSL rewriter, image decode, Canvas2D with text, fonts, keyboard | The MOG_TitleCommands/TitleLayers title renders. The screenshot diff against NW.js at frame 300 meets the §10.2 thresholds. Arrow keys move the cursor | 7–13 pw |
| **M2 Map 92 walkable at 60 fps** | UltraMode7 path (custom shader, geometry, stencil), filters, tilemap, DOM paths hit by New Game, gamepad, mouse, real Effekseer via V8 wasm | New Game reaches Map 92 in Mode7, rendered correctly by screenshot diff. The player walks with keyboard and gamepad. The 2-minute logic-hash replay matches. **60 fps** (p95 frame ≤16.7 ms) on reference hardware (mid-range 2020+ x86, iGPU) | 7–12 pw |
| **M3 Audio** | WebAudio subset, Vorbis + Opus/WebM decoders, loop points, fades, pan, pitch | BGM/BGS/ME/SE play with correct LOOPSTART/LOOPLENGTH looping and fades. `police radio.ogg` (WebM/Opus) plays. Offline-render tests pass within tolerance. CaeF_PauseFix pause/resume works | 3.5–6 pw |
| **M4 Save/load** | fs subset, UTF-8 encoding, config/global | A save from v8mz loads in NW.js and vice versa. The save diff is byte-identical under replay. Saves land in `~/.local/share/HIROTONFA_AmericanRevolutionSmuggler/`. The read-only guard passes | 1–2 pw |
| **M5 All plugins + video** | NIJI_VideoPlayer and stock Video via **nestegg + libvpx (VP9) + libopus, no transcoding**; LightingEffects, MessageCore/busts, minigames, options incl. `ticker.maxFPS`, fullscreen, window close, external links | A long scripted route replay matches the logic hashes. **Both `.webm` movies play from the untouched game folder** with A/V sync within ±40 ms. The ~5 Show Animations render. No unimplemented-API warnings | 5–11 pw |
| **M6 Polish + packaging** | Profiling, optional native zlib, Fast API/code cache/inspector, ICU data choice, AppImage/Flatpak, Windows and macOS builds, docs | Map 92 and the 3 heaviest maps run at 60 fps p95 on an iGPU laptop. Warm startup <3 s. A 2-hour soak shows no leaks. CI on 3 OSes. GPL source and notices ship with binaries | 3.5–9.5 pw |
| **Total** | | | **≈30–58 pw** |

### 12.1 Per-module estimate [G]

| Module | New/adapted LOC | Effort (pw) | Drivers / hardest parts |
|---|---:|---:|---|
| host_core (V8 embedding, loader, event loop, binding glue) | 2.5–4.5k | 3–5 | Handle scopes and lifetimes, weak finalisers, exact task/microtask order, platform pump, error routing |
| V8 build pipeline (CI tarballs, CMake import) | 0.3–0.8k | 1.5–3 | gn args per OS; clang/libc++ ABI; ICU data |
| DOM/BOM/nw/process prelude | 1.5–3k | 1.5–3 | Long "must not throw" tail; ~40–60% adaptable from Outsider's shims [G] |
| WebGL2 binding + GLSL rewrite | 4–7k | 5–9 | Stencil/FBO semantics with VisuMZ's direct calls; texture upload premultiply/flip/dirty tracking; Mode7; driver differences |
| Canvas2D (CPU) | 3–6k | 4–8 | Compositing; gradients; exact un-premultiply. Outsider's `canvas2d.c` is a starting point |
| Text + fonts | 1.5–2.5k | 2–4 | `measureText` parity; `strokeText` outline; baseline metrics |
| Image decode | 0.4–0.8k | 0.5–1 | Colour-management parity (Q5) |
| Audio graph + decoders + fetch streaming | 2.5–4.5k | 3.5–6 | Sample-accurate automation; loop points; truncated decode; Opus-in-WebM |
| Video (nestegg + libvpx + libopus) | 1.5–3k | 3–5 | VP9 CPU cost; A/V sync; `VideoResource` readiness |
| Input | 0.8–1.5k | 0.5–1.5 | keyCode tables; gamepad mapping (adapted from Outsider) |
| Files/saves/XHR/IDB | 1.5–2.5k | 1–2 | Node UTF-8 semantics; XDG paths |
| Effekseer / wasm | 0.2–0.5k | 0.5–1 | Emscripten Node-path detection only |
| Optional fast paths (zlib) | 0–2k | 0–3 | Proving identical behaviour |
| Conformance harness | 2–4k | 2–4 | Adapted from Outsider `tools/evaluator` (CDP); deterministic NW.js reference |
| Build/packaging/CI | 1–2k | 2–3 | Vendoring, 3-OS CI, GPL notices |
| **Total** | **~23–45k** | **~30–58** | |

The total equals the milestone sum.

**What changed from v0.1 (38–73 pw):**
* Removed: the mandatory native PIXI fast paths (4–10 pw), WAMR (1–4) and the QuickJS polyfills.
* Added: the V8 build pipeline (1.5–3).
* Reduced: reuse of Outsider's shims, platform code and harness takes ~3–6 pw off several modules [G].

---

## 13. Prior art (links returned HTTP 200 this session unless noted)

### Outsider: the closest prior art
Outsider is General Arcade's open-source native runtime for RPG Maker MZ.
* **Links:** https://github.com/General-Arcade/outsider ; announcement https://www.reddit.com/r/RPGMakerMZ/comments/1wnxrij/outsider_an_opensource_native_runtime_for_rpg/ ; company page https://generalarcade.com/runtimes/
* **License:** `GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial` (per-file SPDX headers) [V].
* **Size:** tag v0.2.0; ~34.5k lines in `src/` plus ~22k in `tests/` [M].
* **Stack:** QuickJS-ng, SDL3 GPU (OpenGL 4.5 fallback), SoLoud (Ogg Vorbis + WAV), stb_image/stb_truetype, pl_mpeg (needs transcoding), and the native Effekseer SDK.
* **How it differs from this spec:**
  * PIXI is replaced by `src/shims/pixi_shim.js` (4,928 lines) [V].
  * It writes `.jsc` bytecode next to the game scripts (`src/engine/js_engine.c:437`).
  * Storage lives under a relative `save/` directory (`src/shims/storage_shim.js`) [V].
* It could not be built on the box (no GL headers, broken apt) [M].

### 13.1 What to reuse from Outsider (GPL-3.0 option) and what to replace

Outsider's **native code is QuickJS-bound only in `src/engine/js_engine.c` and `src/bindings/*.c`**. Everything else is plain C/C++ or engine-neutral JS. So the reuse plan is: keep its native back ends and JS shims, and rewrite the glue against V8.

| Outsider path (lines) | Verdict | Why / what changes |
|---|---|---|
| `src/shims/dom_shim.js` (1,922), `dom_overlay.js` (933), `navigator_shim.js` (90), `control_shim.js` (55) | **Reuse, adapt** | The DOM/BOM "must not throw" layer is engine-neutral JS. Align the UA, `process.versions` and the canvas/style plumbing with §5.10. Drop the QuickJS workarounds |
| `src/shims/xhr_shim.js` (278), the fetch-over-XHR part of `plugin_compat.js` | **Reuse, adapt** | Add a `fetch()` streaming body (`getReader`) that rmmz audio needs; `status` semantics (`< 400` check) [V] |
| `src/shims/storage_shim.js` (411) | **Adapt heavily** | Keep the localStorage/localforage structure. Replace its relative `save/` paths with real Node-like `fs` semantics, so the game's own Advanced_ChangeSaveLocation chooses the XDG path. Nothing is ever written in the game dir |
| `src/io/file_io.c` (534), `src/bindings/bind_io.c` (287) logic | **Reuse the C; rewrite the glue** | `writeFileSync` already writes JS strings as UTF-8 (`JS_ToCStringLen`) [V], which is the Node behaviour our saves need. The port uses `String::WriteUtf8` |
| `src/platform/platform_sdl3.c` (769), `src/input/input_manager.c` (73) + `.h` | **Reuse** | SDL3 window, events, gamepad. Check the keyCode/gamepad tables against §5.8 |
| `src/engine/script_loader.c` (631), `game_loop.c` (683), `error_handler.c` (225) | **Reuse the algorithms; rewrite for V8** | `<script>` insertion order, `currentScript`, the rAF/timer loop and error screens. The QuickJS API calls become V8 calls. Add the `PumpMessageLoop` + microtask checkpoints |
| `src/engine/js_engine.c` (614) | **Replace** | QuickJS-specific, and its `.jsc` cache writes into the game dir. Replaced by the V8 isolate setup + XDG code cache |
| `src/bindings/*.c` (24 files) | **Replace** | QuickJS C API glue. Re-expressed as V8 templates in `host_core/bind_*.cc` |
| `src/rendering/canvas2d.c` (1,053) + `src/shims/canvas2d_shim.js` (992) | **Reuse as the M1 starting point** | Paths, gradients and transforms work as-is. Text moves from stb_truetype to FreeType + HarfBuzz for `measureText`/`strokeText` parity (§5.3) |
| `src/rendering/font_manager.c` (233), `woff.c` (132), `src/shims/font_shim.js` (240) | **Partly reuse** | The per-glyph fallback logic and `FontFace` plumbing. `woff.c` is unnecessary if FreeType is built with zlib. The game ships 2 `.woff` fonts [V] |
| `src/rendering/glsl_translate.c` (395) | **Reuse ideas, adapt** | Same approach (rewrite only the declarations, redirect with `#define`). Its target is Vulkan GLSL with uniform blocks, while ours is GL 3.3/GLES 3.0, which is simpler |
| `src/rendering/image_loader.c` (297) | **Reuse** | PNG decode via stb_image. `apng.c` is not needed (no APNG plugin in this game [V]) |
| `src/shims/webaudio_shim.js` (600) | **Reuse the API skeleton; replace the back end** | The node classes exist, but parameters are pushed to SoLoud voices, which is not sample-accurate. It is backed by our miniaudio graph instead |
| `src/audio/audio_engine.cpp` (639), `soloud_sdl3_backend.cpp` (SoLoud) | **Replace** | Ogg Vorbis + WAV only (README), with **no Opus**, so `police radio.ogg` would fail. No WebAudio automation semantics |
| `src/video/video_player.c` (279) + `third_party/pl_mpeg`, `tools/resource_converter.py` (375) | **Replace** | They need MPEG-1 transcoding. Replaced by nestegg + libvpx + libopus with no transcoding (§5.7) |
| `src/shims/pixi_shim.js` (4,928), `tilemap_shim.js` (411), `bind_renderer.c`, `bind_filters.c`, `bind_tilemap.c`, `src/rendering/{renderer,renderer_gpu,sprite_batch,sprite_batch_gpu,filters,filters_gpu,gpu_backend,gpu_shader_runtime,tilemap}.c`, `shaders/*.hlsl` + `shaders_generated.h` (5,017) | **Replace** | We run the **real PIXI** over a WebGL2 shim. That is required for UltraMode7 and avoids PIXI-reimplementation drift |
| PIXI filter stubs in `plugin_compat.js` | **Drop** | Real PIXI provides the filters |
| Greenworks stub, legacy `RegExp.$1` and `Array.prototype.contains` in `plugin_compat.js` | **Drop most** | V8 has `RegExp` legacy statics natively. A Greenworks stub is not needed because CaeF_PauseFix feature-tests it [V]. Keep only polyfills a scan shows are used |
| `src/effects/effekseer_backend.cpp` (643) + `effekseer_shim.js` (285) | **Keep in reserve** | Not used on desktop (V8 runs `effekseer.wasm` as shipped). Only a fallback for jitless/no-wasm targets (§5.11) |
| `tools/evaluator/` (cdp.py, drivers.py, scenario.py, compare.py, report.py, prelude.js; ~1.7k [M]) | **Reuse, extend** | A CDP-driven reference and comparison harness. Extend it with seeded RNG, virtual time, state hashes and save `cmp` (§10) |
| `tools/build_game.py` (decrypt/convert/bundle) | **Don't use** | It modifies or repackages game assets, which conflicts with the read-only goal |

**Estimated reuse:** ~8–12k of Outsider's ~34.5k `src` lines, after adaptation [G].

**Obligations:**
* Keep the SPDX/copyright headers.
* Add "modified by" notices.
* Ship the source.
* Upstreaming is possible, but their contribution terms (CLA or not) were not checked [G].

### Other prior art
* **Ejecta** (MIT): JavaScriptCore plus native Canvas2D/WebGL/audio on iOS. It proves the "real JS game + native canvas/WebGL" model.
  * https://github.com/phoboslab/Ejecta , https://impactjs.com/ejecta
  * V8 port: https://github.com/godmodelabs/ejecta-v8
  * Android port: https://github.com/Wizcorp/Ejecta-X
* **headless-gl:** a 1:1 WebGL binding for Node on ANGLE.
  * https://github.com/stackgl/headless-gl (WebGL2 work in PR #310: https://github.com/stackgl/headless-gl/pull/310)
* **expo-gl:** a C++ WebGL2 over GLES.
  * https://github.com/expo/expo/blob/main/packages/expo-gl/common/EXWebGLRenderer.cpp
* **@pixi/node:** https://github.com/pixijs-userland/node , https://www.npmjs.com/package/@pixi/node
* **node-canvas-webgl:** https://github.com/akira-cn/node-canvas-webgl
* **canvas_ity:** https://github.com/a-e-k/canvas_ity
* **CocoonJS / Canvas+:** https://github.com/CocoonIO/cocoon-canvasplus
* **Exokit:** https://github.com/exokitXR/exokit
* **mkxp-z:** https://github.com/mkxp-z/mkxp-z
* **NW.js:** https://github.com/nwjs/nw.js
* **V8 references:**
  * embedding guide: https://v8.dev/docs/embed
  * gn build: https://v8.dev/docs/build-gn
  * jitless blog: https://v8.dev/blog/jitless
  * Node-API docs, for comparison: https://nodejs.org/api/n-api.html
  * React Native JSI, an engine-neutral C++ JS interface, for reference only: https://github.com/facebook/react-native/tree/main/packages/react-native/ReactCommon/jsi

---

## 14. Top 5 hardest parts (honest ranking, V8 edition)

1. **WebGL2 shim fidelity under real PIXI, UltraMode7 and Effekseer.**
   * Stencil and framebuffer-pool semantics, made harder by VisuMZ's direct stencil calls.
   * Texture-upload premultiply, flip and dirty tracking.
   * GLSL ES translation across Mesa, NVIDIA and Intel drivers.
   * Mode7's custom geometry.
2. **Canvas2D text parity with Chrome.**
   * `measureText` drives wrapping and layout; `strokeText` outlines appear on every UI label; baseline metrics must match.
   * It is also the most likely CPU hotspot left, since this is a CPU raster versus Skia.
3. **Media without transcoding.**
   * The WebAudio graph: automation, loop points, truncated-stream decode, Opus-in-WebM.
   * VP9 + Opus video via libvpx, with A/V sync and PIXI `VideoResource` readiness semantics.
4. **V8 embedding and build.**
   * A gn/ninja + clang pipeline per OS, packaged as CI tarballs.
   * libc++ ABI pitfalls.
   * Handle-scope and lifetime bugs in the binding glue, weak-callback finalisation of GL objects, and pumping the platform loop correctly.
   * A 40–55 MB binary.
5. **Event-loop and timing fidelity.**
   * Exact macrotask/microtask ordering, the async wasm instantiate, and script `onload` order.
   * PIXI Ticker / `determineRepeatNumber` pacing with the MultiTweaks/OptionsCore overrides.
   * Deterministic replay against NW.js across V8 versions.

QuickJS CPU speed, v0.1's number-one risk, is gone (PIXI is 0.56 ms per frame on V8).

---

## 15. Open questions

**Resolved:**
* **Q1 (resolved, the owner, 2026-10-30): the JS engine is V8 only.** QuickJS-ng is dropped entirely. There is no engine abstraction layer and no 45 fps gate.
* **Q3 (resolved, the owner, 2026-10-30): libvpx (BSD-3) for VP9, with libopus and nestegg; no transcode cache.**
* **Q6 (resolved, the owner, 2026-10-30): GPL-3.0-or-later.** Outsider code may be reused (§13.1). All listed dependencies are compatible (§11.1).

**Still open (minor or technical):**
* **Q2 Emscripten environment under NW.js.** Does effekseer.min.js take the web path or the Node path in the real game? Trace it under NW.js and match.
* **Q4 ANGLE.** Ship it on Linux, or use driver GL with ANGLE as a CI oracle only?
* **Q5 PNG colour.** Do any of the 2,313 PNGs carry gAMA/iCCP/cHRM chunks? A trivial scan is needed. If they do, match Chrome's colour handling.
* **Q7 Text tolerance.** What visual diff is acceptable for text?
* **Q8 Steam integration.** A launch-option wrapper or a Steam compatibility tool? Is the overlay needed?
* **Q9 Coverage route.** Is there a 100% save or route that can serve as the replay corpus?
* **Q10 Battles.** Does the game use the battle system at all? Coverage depends on it.
* **Q11 V8 version pin.** Use current stable, or match NW.js's 11.7 exactly for reference runs?
* **Q12 ICU.** Full Chromium data (~10 MB) or an en-only filtered file?

---

## Appendix A: evidence files (box, `/workspace/smuggler/hostspec/`)
* `scan/api_by_category.tsv`, `globals.tsv`, `webgl_calls.tsv`, `events_requires_exts.txt`: the static scan.
* `scan/dynamic_api_counts.tsv`: all 237 traced APIs, with call counts and sampled callers.
* `trace/run1`, `run2` (+ `shaders/00–33.glsl`), `run3_noshim` (frame timings), `save/` (save/config/load test).
* `census.js`: the Map 92 scene census.
  * 1,445 display objects: 940 visible, 899 renderable, max depth 5.
  * By type: Sprite 885, Sprite_Picture 255, Sprite_Character 115, Sprite_CharText 115, TilingSprite 15.
  * Filters: ColorFilter 116, AlphaFilter 14. Masks: 0.
  * Renderer plugins include `rpgtilemap` and `um7tilemap`.
* `bench/bench.js`, `render_bench.js`, `glconst.json`, `node.json`, `qjs.json`: benchmarks. `/tmp/pz.js` (copied to `bench/pz.js`): the pako vs zlib save re-measure on Node/V8.
* The game copy used for tests (`/workspace/smuggler/game`) exists only on the box and is never committed.
