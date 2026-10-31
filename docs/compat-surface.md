<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Compatibility inventory for browser, NW.js and Node replacements.
See ../ARCHITECTURE.md for service ownership. Keep exact runtime names and
shim attribution; statuses describe implementation, not conformance certification.
-->

# Compatibility surface

`dom_shim.js` is not seeded yet (M0-T010b); `pixi_shim.js` is not seeded
except for the required known gaps (M0-T010). Post-core/control shims currently
install no members in the game-free dump. Names include aliases, internal
members and constructor back-references retained by DEC-M0-T009b.

This card uses its required five columns and `real`/`partial`/`stub`/`missing`
statuses, rather than the older §15.4 column and status vocabulary.
`partial` means useful behavior with incomplete browser semantics; `stub` means
no-op, fixed response or simulated behavior. No row claims full conformance.
The dump test proves inventory presence only, not semantic correctness.
Existing subsystem tests are listed where available; other rows cite that
inventory test explicitly. Instance-only methods are outside this dump's reach.
For example, `effekseer` has prototype methods but only its namespace is dumped.

Generate with `./build/smzuggler-rt --dump-surface=build/surface.tsv`.
Run `python3 tools/doclint/compat_check.py build/surface.tsv --shim FILE ...`
from the repository root. FILE accepts loader basenames or repository paths.
The checker compares exact (name, shim) pairs, including stub/missing rows;
known missing rows therefore remain visible as full-check failures until fixed.

| name | status | shim file | test | notes |
| --- | --- | --- | --- | --- |
| `Array.prototype.contains` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `Array.prototype.contains.prototype.constructor` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `AudioBuffer` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBuffer.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBuffer.prototype.getChannelData` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBuffer.prototype.getChannelData.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBufferSourceNode` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBufferSourceNode.prototype._applyFade` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBufferSourceNode.prototype._applyFade.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBufferSourceNode.prototype._applyParam` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBufferSourceNode.prototype._applyParam.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBufferSourceNode.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBufferSourceNode.prototype.start` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBufferSourceNode.prototype.start.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBufferSourceNode.prototype.stop` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioBufferSourceNode.prototype.stop.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.close` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `AudioContext.prototype.close.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `AudioContext.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createAnalyser` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createAnalyser.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createBiquadFilter` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createBiquadFilter.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createBuffer` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createBuffer.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createBufferSource` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createBufferSource.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createChannelMerger` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createChannelMerger.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createChannelSplitter` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createChannelSplitter.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createConvolver` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createConvolver.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createDelay` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createDelay.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createDynamicsCompressor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createDynamicsCompressor.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createGain` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createGain.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createPanner` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createPanner.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createStereoPanner` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createStereoPanner.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.createWaveShaper` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.createWaveShaper.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `AudioContext.prototype.currentTime` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.decodeAudioData` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.decodeAudioData.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioContext.prototype.resume` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `AudioContext.prototype.resume.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `AudioContext.prototype.suspend` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `AudioContext.prototype.suspend.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `AudioDestinationNode` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioDestinationNode.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioNode` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioNode.prototype.connect` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioNode.prototype.connect.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioNode.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioNode.prototype.disconnect` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioNode.prototype.disconnect.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype._applyToNative` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype._applyToNative.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.cancelScheduledValues` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.cancelScheduledValues.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.exponentialRampToValueAtTime` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.exponentialRampToValueAtTime.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.linearRampToValueAtTime` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.linearRampToValueAtTime.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.setTargetAtTime` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.setTargetAtTime.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.setValueAtTime` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `AudioParam.prototype.setValueAtTime.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `CSS` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `CSS.supports` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `CSS.supports.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `FontFace` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFace.prototype._loadFromBuffer` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFace.prototype._loadFromBuffer.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFace.prototype._loadFromUrl` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFace.prototype._loadFromUrl.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFace.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFace.prototype.load` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFace.prototype.load.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype._dispatchEvent` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype._dispatchEvent.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype._updateStatus` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype._updateStatus.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.add` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.add.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.addEventListener` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.addEventListener.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.check` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.check.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.clear` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.clear.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.delete` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.delete.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.forEach` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.forEach.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.has` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.has.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.load` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.load.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.removeEventListener` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.removeEventListener.prototype.constructor` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `FontFaceSet.prototype.size` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `GainNode` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `GainNode.prototype._applyFade` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `GainNode.prototype._applyFade.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `GainNode.prototype._applyParam` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `GainNode.prototype._applyParam.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `GainNode.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `HTMLElement.prototype.blur` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.blur.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.click` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.click.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.elements` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.focus` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.focus.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.innerText` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `Html5Audio` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio._audioElement` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio._hasError` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio._initialized` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio._loadListeners` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio._loadListeners.length` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio._unlocked` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio._volume` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.addLoadListener` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.addLoadListener.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.clear` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.clear.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.fadeIn` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.fadeIn.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.fadeOut` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.fadeOut.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.initialize` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.initialize.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.isError` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.isError.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.isPlaying` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.isPlaying.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.isReady` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.isReady.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.play` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.play.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.seek` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.seek.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.setStaticSe` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.setStaticSe.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.setup` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.setup.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.stop` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.stop.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.url` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `Html5Audio.volume` | stub | `src/shims/webaudio_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `MutationObserver` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `MutationObserver.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `MutationObserver.prototype.disconnect` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `MutationObserver.prototype.disconnect.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `MutationObserver.prototype.observe` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `MutationObserver.prototype.observe.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `MutationObserver.prototype.takeRecords` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `MutationObserver.prototype.takeRecords.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.AdjustmentFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.AdjustmentFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.AdjustmentFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.AdvancedBloomFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.AdvancedBloomFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.AdvancedBloomFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.AsciiFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.AsciiFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.AsciiFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.BevelFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.BevelFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.BevelFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.BulgePinchFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.BulgePinchFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.BulgePinchFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.CRTFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.CRTFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.CRTFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.CrossHatchFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.CrossHatchFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.CrossHatchFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.DisplacementFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.DisplacementFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.DisplacementFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.DotFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.DotFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.DotFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.EmbossFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.EmbossFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.EmbossFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.GlitchFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.GlitchFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.GlitchFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.GlowFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.GlowFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.GlowFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.GodrayFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.GodrayFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.GodrayFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.KawaseBlurFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.KawaseBlurFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.KawaseBlurFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.MotionBlurFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.MotionBlurFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.MotionBlurFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.NoiseFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.NoiseFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.NoiseFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.OldFilmFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.OldFilmFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.OldFilmFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.PixelateFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.PixelateFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.PixelateFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.RGBSplitFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.RGBSplitFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.RGBSplitFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.RadialBlurFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.RadialBlurFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.RadialBlurFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.ReflectionFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.ReflectionFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.ReflectionFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.ShockwaveFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.ShockwaveFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.ShockwaveFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.TiltShiftFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.TiltShiftFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.TiltShiftFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.TwistFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.TwistFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.TwistFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.ZoomBlurFilter` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.ZoomBlurFilter.displayName` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PIXI.filters.ZoomBlurFilter.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `PannerNode` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `PannerNode.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `PannerNode.prototype.setOrientation` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `PannerNode.prototype.setOrientation.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `PannerNode.prototype.setPosition` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `PannerNode.prototype.setPosition.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `RegExp.$&` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$'` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$+` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$1` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$2` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$3` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$4` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$5` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$6` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$7` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$8` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$9` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.$_` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| RegExp.$` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.input` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.lastMatch` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.lastParen` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.leftContext` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.prototype.exec` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.prototype.exec.prototype.constructor` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `RegExp.rightContext` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `ResizeObserver` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `ResizeObserver.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `ResizeObserver.prototype.disconnect` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `ResizeObserver.prototype.disconnect.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `ResizeObserver.prototype.observe` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `ResizeObserver.prototype.observe.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `ResizeObserver.prototype.unobserve` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `ResizeObserver.prototype.unobserve.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `XMLHttpRequest` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.DONE` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.HEADERS_RECEIVED` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.LOADING` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.OPENED` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.UNSENT` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype._createEvent` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype._createEvent.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype._fireEvent` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype._fireEvent.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype._fireReadyStateChange` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype._fireReadyStateChange.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.abort` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.abort.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.addEventListener` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.addEventListener.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.getAllResponseHeaders` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.getAllResponseHeaders.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.getResponseHeader` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.getResponseHeader.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.open` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.open.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.overrideMimeType` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.overrideMimeType.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.removeEventListener` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.removeEventListener.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.send` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.send.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.setRequestHeader` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `XMLHttpRequest.prototype.setRequestHeader.prototype.constructor` | partial | `src/shims/xhr_shim.js` | `tests/test_xhr.c` | File-backed XHR subset; synthetic HTTP headers. |
| `__Canvas2DContext` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._ctmIsAxisAligned` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._ctmIsAxisAligned.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._ctmIsIdentity` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._ctmIsIdentity.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._ctmScale` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._ctmScale.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._destroy` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._destroy.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._deviceRect` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._deviceRect.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._ensureNative` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._ensureNative.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._fillGradientRect` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._fillGradientRect.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._gradientToDevice` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._gradientToDevice.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._mapDrawRect` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._mapDrawRect.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._rasterizePath` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._rasterizePath.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._syncFillColor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._syncFillColor.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._syncStrokeColor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._syncStrokeColor.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._tx` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._tx.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._withTempPath` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype._withTempPath.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.arc` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.arc.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.arcTo` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Approximates the corner with lineTo; radius ignored. |
| `__Canvas2DContext.prototype.arcTo.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Approximates the corner with lineTo; radius ignored. |
| `__Canvas2DContext.prototype.beginPath` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.beginPath.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.bezierCurveTo` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.bezierCurveTo.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.clearRect` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.clearRect.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.clip` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.clip.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.closePath` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.closePath.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.createImageData` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.createImageData.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.createLinearGradient` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.createLinearGradient.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.createPattern` | stub | `src/shims/canvas2d_shim.js` | - | No native effect; empty result or simulated state only. |
| `__Canvas2DContext.prototype.createPattern.prototype.constructor` | stub | `src/shims/canvas2d_shim.js` | - | No native effect; empty result or simulated state only. |
| `__Canvas2DContext.prototype.createRadialGradient` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.createRadialGradient.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.drawImage` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.drawImage.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.ellipse` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.ellipse.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.fill` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.fill.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.fillRect` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.fillRect.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.fillStyle` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.fillText` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.fillText.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.font` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.getContextAttributes` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.getContextAttributes.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.getImageData` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.getImageData.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.getTransform` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.getTransform.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.globalAlpha` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.globalCompositeOperation` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.isPointInPath` | stub | `src/shims/canvas2d_shim.js` | `-` | Always false; no hit testing. |
| `__Canvas2DContext.prototype.isPointInPath.prototype.constructor` | stub | `src/shims/canvas2d_shim.js` | `-` | Always false; no hit testing. |
| `__Canvas2DContext.prototype.lineTo` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.lineTo.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.lineWidth` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.measureText` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.measureText.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.moveTo` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.moveTo.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.putImageData` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.putImageData.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.quadraticCurveTo` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.quadraticCurveTo.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.rect` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.rect.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.resetTransform` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.resetTransform.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.restore` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.restore.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.rotate` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.rotate.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.save` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.save.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.scale` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.scale.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.setTransform` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.setTransform.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.stroke` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.stroke.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.strokeRect` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.strokeRect.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.strokeStyle` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.strokeText` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.strokeText.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.textAlign` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.textBaseline` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.transform` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.transform.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.translate` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__Canvas2DContext.prototype.translate.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__WB_pmw` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `__WB_pmw.prototype.constructor` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `_____WB$wombat$assign$function_____` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `_____WB$wombat$assign$function_____.prototype.constructor` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `__dom_flushAnimationFrames` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_flushAnimationFrames.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.canvas` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.ctx` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.fireBubbling` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.fireBubbling.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.focused` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.frame` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.handleRawEvent` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.handleRawEvent.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.hover` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.isOverlayElement` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.isOverlayElement.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.lastRoot` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.layout` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.layout.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.measureCanvas` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.measureCtx` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.pressed` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.render` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.render.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.roots` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.roots.length` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.texH` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.texW` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.texture` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.updateLayouts` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__dom_overlay.updateLayouts.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `__fontFaceSet` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `__fontFaceSet._fonts` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `__fontFaceSet._fonts.length` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `__fontFaceSet._listeners` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `__fontFaceSet._readyResolve` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `__fontFaceSet.ready` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `__fontFaceSet.status` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `__navigator_clearGamepad` | partial | `src/shims/navigator_shim.js` | `tests/smz/test_dump_surface.c` | Fixed browser identity; native gamepad slots only. |
| `__navigator_clearGamepad.prototype.constructor` | partial | `src/shims/navigator_shim.js` | `tests/smz/test_dump_surface.c` | Fixed browser identity; native gamepad slots only. |
| `__navigator_setGamepad` | partial | `src/shims/navigator_shim.js` | `tests/smz/test_dump_surface.c` | Fixed browser identity; native gamepad slots only. |
| `__navigator_setGamepad.prototype.constructor` | partial | `src/shims/navigator_shim.js` | `tests/smz/test_dump_surface.c` | Fixed browser identity; native gamepad slots only. |
| `__parseColor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__parseColor.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__parseFont` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `__parseFont.prototype.constructor` | partial | `src/shims/canvas2d_shim.js` | `tests/test_canvas2d.c` | Native Canvas2D subset; approximated paths and gradients. |
| `document.fonts` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts._fonts` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts._fonts.length` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts._listeners` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts._readyResolve` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts.ready` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts.status` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `effekseer` | partial | `src/shims/effekseer_shim.js` | `tests/smz/test_dump_surface.c` | Native effects adapter namespace; inventory test only. |
| `fetch` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `fetch.prototype.constructor` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `indexedDB` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `indexedDB.open` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `indexedDB.open.prototype.constructor` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `localStorage` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.clear` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.clear.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.getItem` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.getItem.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.key` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.key.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.length` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.removeItem` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.removeItem.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.setItem` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localStorage.setItem.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.clear` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.clear.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.getItem` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.getItem.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.keys` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.keys.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.length` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.length.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.removeItem` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.removeItem.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.setItem` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `localforage.setItem.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `navigator` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.appCodeName` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.appName` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.appVersion` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.clipboard` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.clipboard.readText` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.clipboard.readText.prototype.constructor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.clipboard.writeText` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.clipboard.writeText.prototype.constructor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.cookieEnabled` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.geolocation` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.getGamepads` | partial | `src/shims/navigator_shim.js` | `tests/smz/test_dump_surface.c` | Fixed browser identity; native gamepad slots only. |
| `navigator.getGamepads.prototype.constructor` | partial | `src/shims/navigator_shim.js` | `tests/smz/test_dump_surface.c` | Fixed browser identity; native gamepad slots only. |
| `navigator.hardwareConcurrency` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.language` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.languages` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.languages.0` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.languages.1` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.languages.length` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.maxTouchPoints` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.mediaDevices` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.mediaDevices.enumerateDevices` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.mediaDevices.enumerateDevices.prototype.constructor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.mediaDevices.getUserMedia` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.mediaDevices.getUserMedia.prototype.constructor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.onLine` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.permissions` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.permissions.query` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.permissions.query.prototype.constructor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.platform` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.product` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.productSub` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.sendBeacon` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.sendBeacon.prototype.constructor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.serviceWorker` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.standalone` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.storage` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.storage.estimate` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.storage.estimate.prototype.constructor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.storage.persist` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.storage.persist.prototype.constructor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.storage.persisted` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.storage.persisted.prototype.constructor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.userAgent` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.vendor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.vendorSub` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.vibrate` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `navigator.vibrate.prototype.constructor` | stub | `src/shims/navigator_shim.js` | `-` | Fixed identity/capability response; no browser service. |
| `nw` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `nw.App` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `nw.App.argv` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `nw.App.argv.length` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `nw.App.quit` | partial | `src/shims/storage_shim.js` | `tests/smz/test_dump_surface.c` | Calls native quit; no close-event handshake (§5.2); inventory test only. |
| `nw.App.quit.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/smz/test_dump_surface.c` | Calls native quit; no close-event handshake (§5.2); inventory test only. |
| `nw.Window` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `nw.Window.get` | stub | `src/shims/storage_shim.js` | `-` | Returns an instance with no-op events/window controls (§5.2). |
| `nw.Window.get.prototype.constructor` | stub | `src/shims/storage_shim.js` | `-` | Returns an instance with no-op events/window controls (§5.2). |
| `process` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `process.argv` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `process.argv.length` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `process.cwd` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `process.cwd.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `process.env` | stub | `src/shims/storage_shim.js` | `-` | Empty object; no host environment snapshot (§5.3). |
| `process.exit` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `process.exit.prototype.constructor` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `process.mainModule` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `process.mainModule.filename` | partial | `src/shims/storage_shim.js` | `tests/test_storage.c` | File-backed storage / limited Node and NW.js compatibility. |
| `process.platform` | stub | `src/shims/storage_shim.js` | `-` | Hard-coded linux; does not report host OS (§5.3). |
| `require` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `require.prototype.constructor` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `webkitAudioContext` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.close` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `webkitAudioContext.prototype.close.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `webkitAudioContext.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createAnalyser` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createAnalyser.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createBiquadFilter` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createBiquadFilter.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createBuffer` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createBuffer.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createBufferSource` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createBufferSource.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createChannelMerger` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createChannelMerger.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createChannelSplitter` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createChannelSplitter.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createConvolver` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createConvolver.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createDelay` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createDelay.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createDynamicsCompressor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createDynamicsCompressor.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createGain` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createGain.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createPanner` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createPanner.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createStereoPanner` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createStereoPanner.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.createWaveShaper` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.createWaveShaper.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | `-` | Pass-through graph node; DSP is not implemented. |
| `webkitAudioContext.prototype.currentTime` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.decodeAudioData` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.decodeAudioData.prototype.constructor` | partial | `src/shims/webaudio_shim.js` | `tests/test_webaudio.c` | Native mixer subset; automation and graph semantics limited. |
| `webkitAudioContext.prototype.resume` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `webkitAudioContext.prototype.resume.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `webkitAudioContext.prototype.suspend` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `webkitAudioContext.prototype.suspend.prototype.constructor` | stub | `src/shims/webaudio_shim.js` | - | No native effect; empty result or simulated state only. |
| `PIXI.Geometry` | stub | `src/shims/pixi_shim.js` | - | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Buffer` | stub | `src/shims/pixi_shim.js` | - | Stores buffer data only; no native vertex buffer (§4.2). |
| `PIXI.Program` | stub | `src/shims/pixi_shim.js` | - | Stores shader strings only; no program compilation (§4.2). |
| `PIXI.Circle` | missing | `src/shims/pixi_shim.js` | - | Absent constructor (§5.4). |
