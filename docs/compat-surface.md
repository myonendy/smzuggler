<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-10-31: seed dom_shim.js inventory rows.
Compatibility inventory for browser, NW.js and Node replacements.
See ../ARCHITECTURE.md for service ownership. Keep exact runtime names and
shim attribution; statuses describe implementation, not conformance certification.
-->

# Compatibility surface

`pixi_shim.js` is not seeded except for the required known gaps (M0-T010).
Post-core/control shims currently install no members in the game-free dump.
Names include aliases, internal members and constructor back-references
retained by DEC-M0-T009b.

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
The literal `name` member participates in matching; only the complete five-cell
header is metadata. `tests/tools/test_compat_name_cli.py` checks both a matching
dump and rejection of a stale `name` row through the CLI.

| name | status | shim file | test | notes |
| --- | --- | --- | --- | --- |
| `Array.prototype.contains` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `Array.prototype.contains.prototype.constructor` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `Audio` | stub | `src/shims/dom_shim.js` | `-` | Alias of HTMLAudioElement; no decoder or mixer. |
| `Audio.prototype.canPlayType` | stub | `src/shims/dom_shim.js` | `-` | Fixed probably for ogg/wav/mp3; ignores the build decoder (U11). |
| `Audio.prototype.canPlayType.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Fixed probably for ogg/wav/mp3; ignores the build decoder (U11). |
| `Audio.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Alias of HTMLAudioElement; no decoder or mixer. |
| `Audio.prototype.load` | stub | `src/shims/dom_shim.js` | `-` | No-op; no media fetch. |
| `Audio.prototype.load.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; no media fetch. |
| `Audio.prototype.pause` | stub | `src/shims/dom_shim.js` | `-` | No-op; nothing is playing. |
| `Audio.prototype.pause.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; nothing is playing. |
| `Audio.prototype.play` | stub | `src/shims/dom_shim.js` | `-` | Resolves immediately without audio output. |
| `Audio.prototype.play.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Resolves immediately without audio output. |
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
| `Blob` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | In-memory parts and size; no backing file. |
| `Blob.prototype.arrayBuffer` | stub | `src/shims/dom_shim.js` | `-` | Resolves to a zeroed buffer of this.size. |
| `Blob.prototype.arrayBuffer.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Resolves to a zeroed buffer of this.size. |
| `Blob.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | In-memory parts and size; no backing file. |
| `Blob.prototype.slice` | stub | `src/shims/dom_shim.js` | `-` | Returns an empty Blob; bytes are not copied. |
| `Blob.prototype.slice.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Returns an empty Blob; bytes are not copied. |
| `Blob.prototype.text` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Concatenates string parts only; binary parts are skipped. |
| `Blob.prototype.text.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Concatenates string parts only; binary parts are skipped. |
| `CSS` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `CSS.supports` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `CSS.supports.prototype.constructor` | stub | `src/shims/plugin_compat.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `CSSStyleDeclaration` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | CamelCase property bag; no cascade or computed values. |
| `CSSStyleDeclaration.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | CamelCase property bag; no cascade or computed values. |
| `CSSStyleDeclaration.prototype.getPropertyValue` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Looks up the camelCase field; missing names are empty. |
| `CSSStyleDeclaration.prototype.getPropertyValue.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Looks up the camelCase field; missing names are empty. |
| `CSSStyleDeclaration.prototype.setProperty` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Writes the camelCase field; no shorthand parsing. |
| `CSSStyleDeclaration.prototype.setProperty.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Writes the camelCase field; no shorthand parsing. |
| `ErrorEvent` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | Wraps Event with message, filename, lineno, colno and error. |
| `ErrorEvent.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | Wraps Event with message, filename, lineno, colno and error. |
| `Event` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | type/bubbles/cancelable/target; no phase or composed path. |
| `Event.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | type/bubbles/cancelable/target; no phase or composed path. |
| `Event.prototype.preventDefault` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Sets defaultPrevented when cancelable. |
| `Event.prototype.preventDefault.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Sets defaultPrevented when cancelable. |
| `Event.prototype.stopImmediatePropagation` | stub | `src/shims/dom_shim.js` | `-` | No-op; remaining listeners still run. |
| `Event.prototype.stopImmediatePropagation.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; remaining listeners still run. |
| `Event.prototype.stopPropagation` | stub | `src/shims/dom_shim.js` | `-` | No-op; the shim does not bubble. |
| `Event.prototype.stopPropagation.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; the shim does not bubble. |
| `EventTarget` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Own listener map; no capture or bubbling. |
| `EventTarget.prototype.addEventListener` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Deduped listeners; capture/once/passive ignored. |
| `EventTarget.prototype.addEventListener.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Deduped listeners; capture/once/passive ignored. |
| `EventTarget.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Own listener map; no capture or bubbling. |
| `EventTarget.prototype.dispatchEvent` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Calls listeners and the on* handler; no parent propagation. |
| `EventTarget.prototype.dispatchEvent.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Calls listeners and the on* handler; no parent propagation. |
| `EventTarget.prototype.removeEventListener` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Removes one matching listener; options ignored. |
| `EventTarget.prototype.removeEventListener.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Removes one matching listener; options ignored. |
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
| `HTMLAudioElement` | stub | `src/shims/dom_shim.js` | `-` | AUDIO element stub; MZ playback uses WebAudio. |
| `HTMLAudioElement.prototype.canPlayType` | stub | `src/shims/dom_shim.js` | `-` | Fixed probably for ogg/wav/mp3; ignores the build decoder (U11). |
| `HTMLAudioElement.prototype.canPlayType.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Fixed probably for ogg/wav/mp3; ignores the build decoder (U11). |
| `HTMLAudioElement.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | AUDIO element stub; MZ playback uses WebAudio. |
| `HTMLAudioElement.prototype.load` | stub | `src/shims/dom_shim.js` | `-` | No-op; no media fetch. |
| `HTMLAudioElement.prototype.load.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; no media fetch. |
| `HTMLAudioElement.prototype.pause` | stub | `src/shims/dom_shim.js` | `-` | No-op; nothing is playing. |
| `HTMLAudioElement.prototype.pause.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; nothing is playing. |
| `HTMLAudioElement.prototype.play` | stub | `src/shims/dom_shim.js` | `-` | Resolves immediately without audio output. |
| `HTMLAudioElement.prototype.play.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Resolves immediately without audio output. |
| `HTMLCanvasElement` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Canvas element; 2d context comes from canvas2d_shim when loaded. |
| `HTMLCanvasElement.prototype._destroyNativeContext` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Frees the native 2d handle on the current context. |
| `HTMLCanvasElement.prototype._destroyNativeContext.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Frees the native 2d handle on the current context. |
| `HTMLCanvasElement.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Canvas element; 2d context comes from canvas2d_shim when loaded. |
| `HTMLCanvasElement.prototype.getContext` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | 2d via native or stub context; webgl is a parameter stub. |
| `HTMLCanvasElement.prototype.getContext.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | 2d via native or stub context; webgl is a parameter stub. |
| `HTMLCanvasElement.prototype.height` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Resizes the native 2d surface when a handle exists. |
| `HTMLCanvasElement.prototype.toBlob` | stub | `src/shims/dom_shim.js` | `-` | Callback with an empty PNG Blob. |
| `HTMLCanvasElement.prototype.toBlob.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Callback with an empty PNG Blob. |
| `HTMLCanvasElement.prototype.toDataURL` | stub | `src/shims/dom_shim.js` | `-` | Empty data:image/png;base64, string. |
| `HTMLCanvasElement.prototype.toDataURL.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Empty data:image/png;base64, string. |
| `HTMLCanvasElement.prototype.transferControlToOffscreen` | stub | `src/shims/dom_shim.js` | `-` | Returns this; no OffscreenCanvas. |
| `HTMLCanvasElement.prototype.transferControlToOffscreen.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Returns this; no OffscreenCanvas. |
| `HTMLCanvasElement.prototype.width` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Resizes the native 2d surface when a handle exists. |
| `HTMLElement` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | In-memory element; no layout, CSSOM or live NodeList. |
| `HTMLElement.prototype.appendChild` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Reparents and runs an inserted script when connected. |
| `HTMLElement.prototype.appendChild.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Reparents and runs an inserted script when connected. |
| `HTMLElement.prototype.blur` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.blur.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.click` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.click.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | In-memory element; no layout, CSSOM or live NodeList. |
| `HTMLElement.prototype.elements` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.firstChild` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | First of childNodes; no text-node splitting. |
| `HTMLElement.prototype.focus` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.focus.prototype.constructor` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.getAttribute` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | id/class plus an attributes map; no namespace. |
| `HTMLElement.prototype.getAttribute.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | id/class plus an attributes map; no namespace. |
| `HTMLElement.prototype.getBoundingClientRect` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Origin 0,0 with offsetWidth/offsetHeight; no layout. |
| `HTMLElement.prototype.getBoundingClientRect.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Origin 0,0 with offsetWidth/offsetHeight; no layout. |
| `HTMLElement.prototype.getElementsByTagName` | stub | `src/shims/dom_shim.js` | `-` | Always []; subtree queries are U2. |
| `HTMLElement.prototype.getElementsByTagName.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Always []; subtree queries are U2. |
| `HTMLElement.prototype.hasAttribute` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | id/class plus an attributes map; no namespace. |
| `HTMLElement.prototype.hasAttribute.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | id/class plus an attributes map; no namespace. |
| `HTMLElement.prototype.innerHTML` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Stored string; setter clears children without parsing. |
| `HTMLElement.prototype.innerText` | partial | `src/shims/dom_overlay.js` | `tests/smz/test_dump_surface.c` | Overlay layout/input subset; inventory test only. |
| `HTMLElement.prototype.insertAdjacentElement` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | beforebegin/afterbegin/beforeend/afterend on the in-memory tree. |
| `HTMLElement.prototype.insertAdjacentElement.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | beforebegin/afterbegin/beforeend/afterend on the in-memory tree. |
| `HTMLElement.prototype.insertAdjacentHTML` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Wraps the string in a SPAN; markup is not parsed. |
| `HTMLElement.prototype.insertAdjacentHTML.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Wraps the string in a SPAN; markup is not parsed. |
| `HTMLElement.prototype.insertBefore` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Reparents before a sibling; runs inserted scripts. |
| `HTMLElement.prototype.insertBefore.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Reparents before a sibling; runs inserted scripts. |
| `HTMLElement.prototype.lastChild` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Last of childNodes; no text-node splitting. |
| `HTMLElement.prototype.mozRequestFullScreen` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of requestFullscreen. |
| `HTMLElement.prototype.mozRequestFullScreen.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of requestFullscreen. |
| `HTMLElement.prototype.querySelector` | stub | `src/shims/dom_shim.js` | `-` | Always null; subtree selectors are U2. |
| `HTMLElement.prototype.querySelector.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Always null; subtree selectors are U2. |
| `HTMLElement.prototype.querySelectorAll` | stub | `src/shims/dom_shim.js` | `-` | Always []; subtree selectors are U2. |
| `HTMLElement.prototype.querySelectorAll.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Always []; subtree selectors are U2. |
| `HTMLElement.prototype.remove` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | removeChild on parentNode when present. |
| `HTMLElement.prototype.remove.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | removeChild on parentNode when present. |
| `HTMLElement.prototype.removeAttribute` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Deletes from the attributes map. |
| `HTMLElement.prototype.removeAttribute.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Deletes from the attributes map. |
| `HTMLElement.prototype.removeChild` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Unlinks and drops the element from the registry. |
| `HTMLElement.prototype.removeChild.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Unlinks and drops the element from the registry. |
| `HTMLElement.prototype.requestFullScreen` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of requestFullscreen. |
| `HTMLElement.prototype.requestFullScreen.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of requestFullscreen. |
| `HTMLElement.prototype.requestFullscreen` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Native setFullscreen and records fullscreenElement. |
| `HTMLElement.prototype.requestFullscreen.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Native setFullscreen and records fullscreenElement. |
| `HTMLElement.prototype.setAttribute` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | id/class plus an attributes map; no namespace. |
| `HTMLElement.prototype.setAttribute.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | id/class plus an attributes map; no namespace. |
| `HTMLElement.prototype.textContent` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of the innerHTML string. |
| `HTMLElement.prototype.webkitRequestFullScreen` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of requestFullscreen. |
| `HTMLElement.prototype.webkitRequestFullScreen.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of requestFullscreen. |
| `HTMLElement.prototype.webkitRequestFullscreen` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of requestFullscreen. |
| `HTMLElement.prototype.webkitRequestFullscreen.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of requestFullscreen. |
| `HTMLImageElement` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | IMG element; native decode on src, blob: and data: URLs. |
| `HTMLImageElement.prototype._releaseNativeImage` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Frees native pixels and GL texture; in-flight loads abort. |
| `HTMLImageElement.prototype._releaseNativeImage.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Frees native pixels and GL texture; in-flight loads abort. |
| `HTMLImageElement.prototype.complete` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | True after load or error. |
| `HTMLImageElement.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | IMG element; native decode on src, blob: and data: URLs. |
| `HTMLImageElement.prototype.decode` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Promise that resolves on load and rejects on error. |
| `HTMLImageElement.prototype.decode.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Promise that resolves on load and rejects on error. |
| `HTMLImageElement.prototype.height` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Intrinsic size after load; writable display size. |
| `HTMLImageElement.prototype.naturalHeight` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Native decode height; 0 until load. |
| `HTMLImageElement.prototype.naturalWidth` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Native decode width; 0 until load. |
| `HTMLImageElement.prototype.src` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Loads via native image, blob: store or data: URL. |
| `HTMLImageElement.prototype.width` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Intrinsic size after load; writable display size. |
| `HTMLVideoElement` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | VIDEO element backed by the native MPEG-1 player. |
| `HTMLVideoElement.prototype._closeNative` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Closes the native handle and drops the active list. |
| `HTMLVideoElement.prototype._closeNative.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Closes the native handle and drops the active list. |
| `HTMLVideoElement.prototype._dispatchLoadedOnce` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Fires loadedmetadata/loadeddata/canplay once. |
| `HTMLVideoElement.prototype._dispatchLoadedOnce.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Fires loadedmetadata/loadeddata/canplay once. |
| `HTMLVideoElement.prototype._finishAsync` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Delayed loadeddata then ended so wait-mode does not hang. |
| `HTMLVideoElement.prototype._finishAsync.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Delayed loadeddata then ended so wait-mode does not hang. |
| `HTMLVideoElement.prototype._openNative` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Tries .mpg then original src; false when unplayable. |
| `HTMLVideoElement.prototype._openNative.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Tries .mpg then original src; false when unplayable. |
| `HTMLVideoElement.prototype.canPlayType` | stub | `src/shims/dom_shim.js` | `-` | maybe for webm/mp4/ogg when the player exists; not Chrome 117 (U11). |
| `HTMLVideoElement.prototype.canPlayType.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | maybe for webm/mp4/ogg when the player exists; not Chrome 117 (U11). |
| `HTMLVideoElement.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | VIDEO element backed by the native MPEG-1 player. |
| `HTMLVideoElement.prototype.load` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Opens native or schedules the missing-file ended path. |
| `HTMLVideoElement.prototype.load.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Opens native or schedules the missing-file ended path. |
| `HTMLVideoElement.prototype.loop` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Forwarded to the native player when open. |
| `HTMLVideoElement.prototype.pause` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Pauses the native handle and fires pause. |
| `HTMLVideoElement.prototype.pause.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Pauses the native handle and fires pause. |
| `HTMLVideoElement.prototype.play` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Opens .mpg (or src); missing files finish after a delay. |
| `HTMLVideoElement.prototype.play.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Opens .mpg (or src); missing files finish after a delay. |
| `HTMLVideoElement.prototype.src` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Closes the native handle and resets ended/paused. |
| `HTMLVideoElement.prototype.volume` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Forwarded to the native player when open. |
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
| `Image` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of HTMLImageElement; native decode on src. |
| `Image.prototype._releaseNativeImage` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Frees native pixels and GL texture; in-flight loads abort. |
| `Image.prototype._releaseNativeImage.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Frees native pixels and GL texture; in-flight loads abort. |
| `Image.prototype.complete` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | True after load or error. |
| `Image.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of HTMLImageElement; native decode on src. |
| `Image.prototype.decode` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Promise that resolves on load and rejects on error. |
| `Image.prototype.decode.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Promise that resolves on load and rejects on error. |
| `Image.prototype.height` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Intrinsic size after load; writable display size. |
| `Image.prototype.naturalHeight` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Native decode height; 0 until load. |
| `Image.prototype.naturalWidth` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Native decode width; 0 until load. |
| `Image.prototype.src` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Loads via native image, blob: store or data: URL. |
| `Image.prototype.width` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Intrinsic size after load; writable display size. |
| `KeyboardEvent` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | key/code/keyCode plus modifiers; no composition. |
| `KeyboardEvent.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | key/code/keyCode plus modifiers; no composition. |
| `KeyboardEvent.prototype.getModifierState` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Shift/Control/Alt/Meta only. |
| `KeyboardEvent.prototype.getModifierState.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Shift/Control/Alt/Meta only. |
| `MouseEvent` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | client/page/screen coordinates and button; no relatedTarget. |
| `MouseEvent.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | client/page/screen coordinates and button; no relatedTarget. |
| `MouseEvent.prototype.getModifierState` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Shift/Control/Alt/Meta only. |
| `MouseEvent.prototype.getModifierState.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Shift/Control/Alt/Meta only. |
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
| `PromiseRejectionEvent` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | Wraps Event with reason and promise. |
| `PromiseRejectionEvent.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | Wraps Event with reason and promise. |
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
| `URL` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | createObjectURL/revokeObjectURL over an in-memory map. |
| `URL.createObjectURL` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | blob:internal/N id stored until revoke. |
| `URL.createObjectURL.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | blob:internal/N id stored until revoke. |
| `URL.revokeObjectURL` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Deletes the blob:internal id. |
| `URL.revokeObjectURL.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Deletes the blob:internal id. |
| `WheelEvent` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | MouseEvent plus deltaX/Y/Z and deltaMode. |
| `WheelEvent.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | MouseEvent plus deltaX/Y/Z and deltaMode. |
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
| `__dom_flushInputEvents` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Polls native input into Keyboard/Mouse/Wheel events on document. |
| `__dom_flushInputEvents.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Polls native input into Keyboard/Mouse/Wheel events on document. |
| `__dom_flushTimers` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Runs due setTimeout/setInterval callbacks once per frame. |
| `__dom_flushTimers.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Runs due setTimeout/setInterval callbacks once per frame. |
| `__dom_getElementRegistryLength` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Count of created elements still in the registry. |
| `__dom_getElementRegistryLength.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Count of created elements still in the registry. |
| `__dom_initDisplayInfo` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Copies native display size and scale into screen/devicePixelRatio. |
| `__dom_initDisplayInfo.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Copies native display size and scale into screen/devicePixelRatio. |
| `__dom_noteScriptLoaded` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Marks a path so a later script src does not re-eval. |
| `__dom_noteScriptLoaded.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Marks a path so a later script src does not re-eval. |
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
| `__dom_removeFromElementRegistry` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Drops an element so GC can reclaim it. |
| `__dom_removeFromElementRegistry.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Drops an element so GC can reclaim it. |
| `__dom_setCurrentScript` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Installs document.currentScript for plugin eval. |
| `__dom_setCurrentScript.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Installs document.currentScript for plugin eval. |
| `__dom_setWindowSize` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Updates inner/outer size, letterbox viewport and fires resize. |
| `__dom_setWindowSize.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Updates inner/outer size, letterbox viewport and fires resize. |
| `__dom_updateGamepads` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Pushes native pads into the navigator shim. |
| `__dom_updateGamepads.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Pushes native pads into the navigator shim. |
| `__dom_updateVideos` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Mirrors native MPEG-1 time/ended and visibility from opacity. |
| `__dom_updateVideos.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Mirrors native MPEG-1 time/ended and visibility from opacity. |
| `__error_handler_autoSave` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | DataManager.saveGame(0) when the MZ objects exist. |
| `__error_handler_autoSave.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | DataManager.saveGame(0) when the MZ objects exist. |
| `__error_handler_dispatch` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | window.onerror then an error Event. |
| `__error_handler_dispatch.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | window.onerror then an error Event. |
| `__error_handler_dispatchRejection` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | onunhandledrejection then unhandledrejection Event. |
| `__error_handler_dispatchRejection.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | onunhandledrejection then unhandledrejection Event. |
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
| `_listeners` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Internal map for window addEventListener. |
| `addEventListener` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Window listener list; capture/once/passive ignored. |
| `addEventListener.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Window listener list; capture/once/passive ignored. |
| `alert` | stub | `src/shims/dom_shim.js` | `-` | Logs [alert] to console. |
| `alert.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Logs [alert] to console. |
| `atob` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | JS base64; uses native __atob when present. |
| `atob.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | JS base64; uses native __atob when present. |
| `blur` | stub | `src/shims/dom_shim.js` | `-` | No-op; the SDL window is not focused from JS. |
| `blur.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; the SDL window is not focused from JS. |
| `btoa` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | JS base64; uses native __btoa when present. |
| `btoa.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | JS base64; uses native __btoa when present. |
| `cancelAnimationFrame` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Drops a pending rAF callback. |
| `cancelAnimationFrame.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Drops a pending rAF callback. |
| `clearInterval` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Marks the interval cleared. |
| `clearInterval.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Marks the interval cleared. |
| `clearTimeout` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Marks the timeout cleared. |
| `clearTimeout.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Marks the timeout cleared. |
| `close` | stub | `src/shims/dom_shim.js` | `-` | No-op; does not quit the process. |
| `close.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; does not quit the process. |
| `confirm` | stub | `src/shims/dom_shim.js` | `-` | Always returns true. |
| `confirm.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Always returns true. |
| `devicePixelRatio` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Defaults to 1; overwritten from native display scale. |
| `dispatchEvent` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Window listeners plus the on* handler; no bubbling. |
| `dispatchEvent.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Window listeners plus the on* handler; no bubbling. |
| `document` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Minimal Document; readyState complete, no browsing context. |
| `document._listeners` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Internal map for document events. |
| `document.addEventListener` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Document listener list; input events land here. |
| `document.addEventListener.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Document listener list; input events land here. |
| `document.body` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Pre-created BODY; instance members are not dumped (DEC-M0-T009b). |
| `document.cancelFullScreen` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Alias of exitFullscreen. |
| `document.cancelFullScreen.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Alias of exitFullscreen. |
| `document.createDocumentFragment` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | HTMLElement with nodeType 11. |
| `document.createDocumentFragment.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | HTMLElement with nodeType 11. |
| `document.createElement` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | canvas/img/video/audio or generic HTMLElement. |
| `document.createElement.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | canvas/img/video/audio or generic HTMLElement. |
| `document.createElementNS` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Ignores the namespace and delegates to createElement. |
| `document.createElementNS.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Ignores the namespace and delegates to createElement. |
| `document.createEvent` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | new Event(type); typed event interfaces are ignored. |
| `document.createEvent.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | new Event(type); typed event interfaces are ignored. |
| `document.createTextNode` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | HTMLElement with nodeType 3 holding the text. |
| `document.createTextNode.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | HTMLElement with nodeType 3 holding the text. |
| `document.currentScript` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Plugin path during eval; null at rest. |
| `document.dispatchEvent` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Calls document listeners; no parent propagation. |
| `document.dispatchEvent.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Calls document listeners; no parent propagation. |
| `document.documentElement` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Pre-created HTML element; instance members are not dumped. |
| `document.exitFullscreen` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Clears fullscreen flags and calls native setFullscreen(false). |
| `document.exitFullscreen.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Clears fullscreen flags and calls native setFullscreen(false). |
| `document.exitPointerLock` | stub | `src/shims/dom_shim.js` | `-` | No-op; pointer lock is not implemented. |
| `document.exitPointerLock.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; pointer lock is not implemented. |
| `document.fonts` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts._fonts` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts._fonts.length` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts._listeners` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts._readyResolve` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts.ready` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fonts.status` | partial | `src/shims/font_shim.js` | `tests/test_font.c` | Native font loading; simplified family parsing and set semantics. |
| `document.fullScreenElement` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Set by requestFullscreen; null after exit. |
| `document.fullscreenElement` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Set by requestFullscreen; null after exit. |
| `document.getComputedStyle` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of window.getComputedStyle. |
| `document.getComputedStyle.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of window.getComputedStyle. |
| `document.getElementById` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Scans the created-element registry. |
| `document.getElementById.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Scans the created-element registry. |
| `document.getElementsByClassName` | stub | `src/shims/dom_shim.js` | `-` | Always []; class queries are U2. |
| `document.getElementsByClassName.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Always []; class queries are U2. |
| `document.getElementsByTagName` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Registry scan; head/body/html are missing (U1). |
| `document.getElementsByTagName.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Registry scan; head/body/html are missing (U1). |
| `document.head` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Pre-created HEAD; omitted from getElementsByTagName (U1). |
| `document.hidden` | stub | `src/shims/dom_shim.js` | `-` | Always false. |
| `document.mozCancelFullScreen` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Alias of exitFullscreen. |
| `document.mozCancelFullScreen.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Alias of exitFullscreen. |
| `document.mozFullScreen` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Boolean alias of fullscreen state. |
| `document.nodeType` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | 9 (DOCUMENT_NODE). |
| `document.querySelector` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | body/html/head or #id; no class/compound selectors (U2). |
| `document.querySelector.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | body/html/head or #id; no class/compound selectors (U2). |
| `document.querySelectorAll` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Wraps querySelector in a one-element array. |
| `document.querySelectorAll.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Wraps querySelector in a one-element array. |
| `document.readyState` | stub | `src/shims/dom_shim.js` | `-` | Always complete; no loading lifecycle. |
| `document.removeEventListener` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Removes one document listener. |
| `document.removeEventListener.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Removes one document listener. |
| `document.title` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Forwards to native setWindowTitle. |
| `document.visibilityState` | stub | `src/shims/dom_shim.js` | `-` | Always visible. |
| `document.webkitCancelFullScreen` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Alias of exitFullscreen. |
| `document.webkitCancelFullScreen.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Alias of exitFullscreen. |
| `document.webkitExitFullscreen` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Alias of exitFullscreen. |
| `document.webkitExitFullscreen.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Alias of exitFullscreen. |
| `document.webkitFullscreenElement` | partial | `src/shims/dom_shim.js` | `tests/smz/test_dump_surface.c` | Set by requestFullscreen; null after exit. |
| `effekseer` | partial | `src/shims/effekseer_shim.js` | `tests/smz/test_dump_surface.c` | Native effects adapter namespace; inventory test only. |
| `fetch` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `fetch.prototype.constructor` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `focus` | stub | `src/shims/dom_shim.js` | `-` | No-op; the SDL window is not focused from JS. |
| `focus.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; the SDL window is not focused from JS. |
| `getComputedStyle` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Returns el.style or a fresh CSSStyleDeclaration. |
| `getComputedStyle.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Returns el.style or a fresh CSSStyleDeclaration. |
| `getSelection` | stub | `src/shims/dom_shim.js` | `-` | Dummy with no-op removeAllRanges/addRange. |
| `getSelection.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Dummy with no-op removeAllRanges/addRange. |
| `indexedDB` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `indexedDB.open` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `indexedDB.open.prototype.constructor` | stub | `src/shims/storage_shim.js` | `-` | No-op / fixed compatibility response; no host semantics. |
| `innerHeight` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Logical window height; updated by __dom_setWindowSize. |
| `innerWidth` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Logical window width; updated by __dom_setWindowSize. |
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
| `location` | stub | `src/shims/dom_shim.js` | `-` | Fixed file:///game/index.html; no navigation. |
| `location.assign` | stub | `src/shims/dom_shim.js` | `-` | No-op; the page is not replaced. |
| `location.assign.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; the page is not replaced. |
| `location.hash` | stub | `src/shims/dom_shim.js` | `-` | Always empty; no fragment navigation. |
| `location.host` | stub | `src/shims/dom_shim.js` | `-` | Always empty on the file URL. |
| `location.hostname` | stub | `src/shims/dom_shim.js` | `-` | Always empty on the file URL. |
| `location.href` | stub | `src/shims/dom_shim.js` | `-` | Hard-coded file:///game/index.html. |
| `location.origin` | stub | `src/shims/dom_shim.js` | `-` | Hard-coded file://. |
| `location.pathname` | stub | `src/shims/dom_shim.js` | `-` | Hard-coded /game/index.html. |
| `location.port` | stub | `src/shims/dom_shim.js` | `-` | Always empty. |
| `location.protocol` | stub | `src/shims/dom_shim.js` | `-` | Hard-coded file:. |
| `location.reload` | stub | `src/shims/dom_shim.js` | `-` | No-op; the page is not reloaded. |
| `location.reload.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; the page is not reloaded. |
| `location.replace` | stub | `src/shims/dom_shim.js` | `-` | No-op; the page is not replaced. |
| `location.replace.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; the page is not replaced. |
| `location.search` | stub | `src/shims/dom_shim.js` | `-` | Always empty; no query string. |
| `matchMedia` | stub | `src/shims/dom_shim.js` | `-` | matches is always false. |
| `matchMedia.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | matches is always false. |
| `moveBy` | stub | `src/shims/dom_shim.js` | `-` | No-op; window position is not changed from JS. |
| `moveBy.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; window position is not changed from JS. |
| `moveTo` | stub | `src/shims/dom_shim.js` | `-` | No-op; window position is not changed from JS. |
| `moveTo.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; window position is not changed from JS. |
| `name` | stub | `src/shims/dom_shim.js` | `-` | Empty string so a bare name read is not a ReferenceError. |
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
| `onerror` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | Host exceptions call this with the browser arity. |
| `onunhandledrejection` | partial | `src/shims/dom_shim.js` | `tests/test_error_handler.c` | Host rejections call this with a reason object. |
| `open` | stub | `src/shims/dom_shim.js` | `-` | Always returns null; no second window. |
| `open.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Always returns null; no second window. |
| `outerHeight` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Logical window height; updated by __dom_setWindowSize. |
| `outerWidth` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Logical window width; updated by __dom_setWindowSize. |
| `performance` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | now() is the timer clock; the timeline is empty. |
| `performance.clearMarks` | stub | `src/shims/dom_shim.js` | `-` | No-op; no performance timeline. |
| `performance.clearMarks.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; no performance timeline. |
| `performance.clearMeasures` | stub | `src/shims/dom_shim.js` | `-` | No-op; no performance timeline. |
| `performance.clearMeasures.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; no performance timeline. |
| `performance.getEntries` | stub | `src/shims/dom_shim.js` | `-` | Always []; no performance timeline. |
| `performance.getEntries.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Always []; no performance timeline. |
| `performance.getEntriesByName` | stub | `src/shims/dom_shim.js` | `-` | Always []; no performance timeline. |
| `performance.getEntriesByName.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Always []; no performance timeline. |
| `performance.getEntriesByType` | stub | `src/shims/dom_shim.js` | `-` | Always []; no performance timeline. |
| `performance.getEntriesByType.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Always []; no performance timeline. |
| `performance.mark` | stub | `src/shims/dom_shim.js` | `-` | No-op; no performance timeline. |
| `performance.mark.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; no performance timeline. |
| `performance.measure` | stub | `src/shims/dom_shim.js` | `-` | No-op; no performance timeline. |
| `performance.measure.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; no performance timeline. |
| `performance.now` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Milliseconds since shim load, same clock as timers. |
| `performance.now.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Milliseconds since shim load, same clock as timers. |
| `print` | stub | `src/shims/dom_shim.js` | `-` | No-op. |
| `print.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op. |
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
| `prompt` | stub | `src/shims/dom_shim.js` | `-` | Always returns null. |
| `prompt.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | Always returns null. |
| `removeEventListener` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Removes one window listener. |
| `removeEventListener.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Removes one window listener. |
| `requestAnimationFrame` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Queued until the game loop flushes the frame. |
| `requestAnimationFrame.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Queued until the game loop flushes the frame. |
| `require` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `require.prototype.constructor` | partial | `src/shims/plugin_compat.js` | `tests/smz/test_dump_surface.c` | Compatibility polyfill; inventory test only. |
| `resizeBy` | stub | `src/shims/dom_shim.js` | `-` | No-op; window size is not changed from JS. |
| `resizeBy.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; window size is not changed from JS. |
| `resizeTo` | stub | `src/shims/dom_shim.js` | `-` | No-op; window size is not changed from JS. |
| `resizeTo.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; window size is not changed from JS. |
| `screen` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Size from window, then native display; no monitor list. |
| `screen.availHeight` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Copied from window then native display size. |
| `screen.availWidth` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Copied from window then native display size. |
| `screen.colorDepth` | stub | `src/shims/dom_shim.js` | `-` | Hard-coded 32. |
| `screen.height` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Copied from window then native display size. |
| `screen.orientation` | stub | `src/shims/dom_shim.js` | `-` | Fixed landscape-primary; no change events. |
| `screen.orientation.addEventListener` | stub | `src/shims/dom_shim.js` | `-` | No-op; orientation never changes. |
| `screen.orientation.addEventListener.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; orientation never changes. |
| `screen.orientation.angle` | stub | `src/shims/dom_shim.js` | `-` | Always 0. |
| `screen.orientation.removeEventListener` | stub | `src/shims/dom_shim.js` | `-` | No-op; orientation never changes. |
| `screen.orientation.removeEventListener.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; orientation never changes. |
| `screen.orientation.type` | stub | `src/shims/dom_shim.js` | `-` | Always landscape-primary. |
| `screen.pixelDepth` | stub | `src/shims/dom_shim.js` | `-` | Hard-coded 32. |
| `screen.width` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Copied from window then native display size. |
| `scrollTo` | stub | `src/shims/dom_shim.js` | `-` | No-op; there is no document scroll offset. |
| `scrollTo.prototype.constructor` | stub | `src/shims/dom_shim.js` | `-` | No-op; there is no document scroll offset. |
| `self` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of window/globalThis. |
| `setInterval` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Queued until __dom_flushTimers; delay is frame-based. |
| `setInterval.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Queued until __dom_flushTimers; delay is frame-based. |
| `setTimeout` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Queued until __dom_flushTimers; delay is frame-based. |
| `setTimeout.prototype.constructor` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Queued until __dom_flushTimers; delay is frame-based. |
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
| `window` | partial | `src/shims/dom_shim.js` | `tests/test_dom_shim.c` | Alias of globalThis. |
| `PIXI.Geometry` | stub | `src/shims/pixi_shim.js` | - | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Buffer` | stub | `src/shims/pixi_shim.js` | - | Stores buffer data only; no native vertex buffer (§4.2). |
| `PIXI.Program` | stub | `src/shims/pixi_shim.js` | - | Stores shader strings only; no program compilation (§4.2). |
| `PIXI.Circle` | missing | `src/shims/pixi_shim.js` | - | Absent constructor (§5.4). |
