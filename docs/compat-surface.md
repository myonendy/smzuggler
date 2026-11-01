<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 2026 The smzuggler contributors
Modified 2026-10-31: seed dom_shim.js inventory rows.
Modified 2026-10-31: seed pixi_shim.js inventory rows.
Compatibility inventory for browser, NW.js and Node replacements.
See ../ARCHITECTURE.md for service ownership. Keep exact runtime names and
shim attribution; statuses describe implementation, not conformance certification.
-->

# Compatibility surface

Every dumped member has a row; `pixi_shim.js` rows describe Outsider's
QuickJS-mode PIXI, which real-PIXI mode does not load (§4.2).
Absent APIs such as `PIXI.Circle` are recorded in the notes of their parent
row, because the table matches the dump exactly.
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
| `PIXI` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Namespace of the QuickJS-mode shim; no Circle, Ellipse, Polygon, RoundedRectangle or Mesh (§5.4). Inventory test only. |
| `PIXI.Application` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.render` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.render.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.screen` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.start` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.start.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.stop` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.stop.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.ticker` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.Application.prototype.view` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Wires renderer, stage and ticker. |
| `PIXI.BLEND_MODES` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.ADD` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.ADD_NPM` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.COLOR` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.COLOR_BURN` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.COLOR_DODGE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.DARKEN` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.DIFFERENCE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.DST_ATOP` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.DST_IN` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.DST_OUT` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.DST_OVER` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.ERASE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.EXCLUSION` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.HARD_LIGHT` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.HUE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.LIGHTEN` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.LUMINOSITY` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.MULTIPLY` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.NONE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.NORMAL` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.NORMAL_NPM` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.OVERLAY` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.SATURATION` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.SCREEN` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.SCREEN_NPM` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.SOFT_LIGHT` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.SRC_ATOP` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.SRC_IN` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.SRC_OUT` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.SRC_OVER` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.SUBTRACT` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BLEND_MODES.XOR` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; native blending covers a subset. |
| `PIXI.BaseRenderTexture` | partial | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Size/valid fields only; storage is created by the renderer. |
| `PIXI.BaseRenderTexture.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Size/valid fields only; storage is created by the renderer. |
| `PIXI.BaseRenderTexture.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Size/valid fields only; storage is created by the renderer. |
| `PIXI.BaseRenderTexture.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Size/valid fields only; storage is created by the renderer. |
| `PIXI.BaseRenderTexture.prototype.resize` | partial | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Size/valid fields only; storage is created by the renderer. |
| `PIXI.BaseRenderTexture.prototype.resize.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Size/valid fields only; storage is created by the renderer. |
| `PIXI.BaseTexture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.from` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.from.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.fromCanvas` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.fromCanvas.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype._ensureGLTexture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype._ensureGLTexture.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.dispose` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.dispose.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.setRealSize` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.setRealSize.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.setResource` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.setResource.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.setSize` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.setSize.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.setStyle` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.setStyle.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.update` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.BaseTexture.prototype.update.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native texture handle; image and canvas resources only. |
| `PIXI.Bounds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addBounds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addBounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addBoundsMask` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addBoundsMask.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addFrame` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addFrame.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addFrameMatrix` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addFrameMatrix.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addPoint` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addPoint.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addQuad` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.addQuad.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.clear` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.clear.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.getRectangle` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.getRectangle.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.isEmpty` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Bounds.prototype.isEmpty.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Axis-aligned bounds accumulator. |
| `PIXI.Buffer` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores buffer data only; no native vertex buffer (§4.2). |
| `PIXI.Buffer.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores buffer data only; no native vertex buffer (§4.2). |
| `PIXI.Buffer.prototype.destroy` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores buffer data only; no native vertex buffer (§4.2). |
| `PIXI.Buffer.prototype.destroy.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores buffer data only; no native vertex buffer (§4.2). |
| `PIXI.Buffer.prototype.dispose` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores buffer data only; no native vertex buffer (§4.2). |
| `PIXI.Buffer.prototype.dispose.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores buffer data only; no native vertex buffer (§4.2). |
| `PIXI.Buffer.prototype.update` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores buffer data only; no native vertex buffer (§4.2). |
| `PIXI.Buffer.prototype.update.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores buffer data only; no native vertex buffer (§4.2). |
| `PIXI.Container` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype._calculateBounds` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.Container.prototype._calculateBounds.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.Container.prototype._render` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.Container.prototype._render.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.Container.prototype.addChild` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.addChild.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.addChildAt` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.addChildAt.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.calculateBounds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.calculateBounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.containerUpdateTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.containerUpdateTransform.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.getChildAt` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.getChildAt.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.getChildByName` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.getChildByName.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.getChildIndex` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.getChildIndex.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.onChildrenChange` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.Container.prototype.onChildrenChange.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.Container.prototype.removeChild` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.removeChild.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.removeChildAt` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.removeChildAt.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.removeChildren` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.removeChildren.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.render` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.render.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.renderAdvanced` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.renderAdvanced.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.setChildIndex` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.setChildIndex.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.sortChildren` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.sortChildren.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.swapChildren` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.swapChildren.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.updateTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.updateTransform.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.Container.prototype.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph container; children drawn as quads. |
| `PIXI.DisplayObject` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype._recursivePostUpdateTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype._recursivePostUpdateTransform.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype._renderFiltered` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype._renderFiltered.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype._renderMasked` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype._renderMasked.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.calculateBounds` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.DisplayObject.prototype.calculateBounds.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.DisplayObject.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.displayObjectUpdateTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.displayObjectUpdateTransform.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.getBounds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.getBounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.getLocalBounds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.getLocalBounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.localTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.mask` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.pivot` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.position` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.render` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.DisplayObject.prototype.render.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.DisplayObject.prototype.rotation` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.scale` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.setParent` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.setParent.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.setTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.setTransform.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.skew` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.toGlobal` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.toGlobal.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.toLocal` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.toLocal.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.updateTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.updateTransform.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.worldTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.worldVisible` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.DisplayObject.prototype.zIndex` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Scene-graph node; drawn as quads, no WebGL state. |
| `PIXI.FORMATS` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.FORMATS.ALPHA` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.FORMATS.LUMINANCE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.FORMATS.LUMINANCE_ALPHA` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.FORMATS.RGB` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.FORMATS.RGBA` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.Filter` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | GLSL filter via the native filter compiler; fragment-only. |
| `PIXI.Filter.defaultFragmentSrc` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | GLSL filter via the native filter compiler; fragment-only. |
| `PIXI.Filter.defaultVertexSrc` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | GLSL filter via the native filter compiler; fragment-only. |
| `PIXI.Filter.prototype.apply` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | GLSL filter via the native filter compiler; fragment-only. |
| `PIXI.Filter.prototype.apply.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | GLSL filter via the native filter compiler; fragment-only. |
| `PIXI.Filter.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | GLSL filter via the native filter compiler; fragment-only. |
| `PIXI.Filter.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | GLSL filter via the native filter compiler; fragment-only. |
| `PIXI.Filter.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | GLSL filter via the native filter compiler; fragment-only. |
| `PIXI.Geometry` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Geometry.prototype.addAttribute` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Geometry.prototype.addAttribute.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Geometry.prototype.addIndex` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Geometry.prototype.addIndex.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Geometry.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Geometry.prototype.destroy` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Geometry.prototype.destroy.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Geometry.prototype.dispose` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Geometry.prototype.dispose.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores attributes only; no native mesh draw path (§4.2). |
| `PIXI.Graphics` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype._calculateBounds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype._calculateBounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype._fillSnapshot` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype._fillSnapshot.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.beginFill` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.beginFill.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.beginTextureFill` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.beginTextureFill.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.clear` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.clear.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.clone` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.clone.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.closePath` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.closePath.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.drawCircle` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.drawCircle.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.drawEllipse` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.drawEllipse.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.drawPolygon` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.drawPolygon.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.drawRect` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.drawRect.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.drawRoundedRect` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.drawRoundedRect.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.endFill` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.endFill.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.finishPoly` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.finishPoly.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.lineStyle` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.lineStyle.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.lineTo` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.lineTo.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.moveTo` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.moveTo.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.render` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.render.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.startPoly` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Graphics.prototype.startPoly.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Shape fills rasterised to quads; no tessellated meshes. |
| `PIXI.Loader` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader._plugins` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader._plugins.length` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype._finish` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype._finish.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype._loadResource` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype._loadResource.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.add` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.add.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.load` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.load.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.pre` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.pre.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.reset` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.reset.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.use` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.prototype.use.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.registerPlugin` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.registerPlugin.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.Loader.shared` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal resource loader over the XHR/Image shims. Inventory test only. |
| `PIXI.LoaderResource` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.LOAD_TYPE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.LOAD_TYPE.AUDIO` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.LOAD_TYPE.IMAGE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.LOAD_TYPE.VIDEO` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.LOAD_TYPE.XHR` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.STATUS_FLAGS` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.STATUS_FLAGS.COMPLETE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.STATUS_FLAGS.DATA_URL` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.STATUS_FLAGS.LOADING` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.STATUS_FLAGS.NONE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.TYPE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.TYPE.AUDIO` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.TYPE.IMAGE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.TYPE.JSON` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.TYPE.TEXT` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.TYPE.UNKNOWN` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.TYPE.VIDEO` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.TYPE.XML` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.XHR_RESPONSE_TYPE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.XHR_RESPONSE_TYPE.BLOB` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.XHR_RESPONSE_TYPE.BUFFER` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.XHR_RESPONSE_TYPE.DEFAULT` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.XHR_RESPONSE_TYPE.DOCUMENT` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.XHR_RESPONSE_TYPE.JSON` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.XHR_RESPONSE_TYPE.TEXT` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource._determineLoadType` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource._determineLoadType.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource._imageExts` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource._imageExts.bmp` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource._imageExts.gif` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource._imageExts.jpeg` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource._imageExts.jpg` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource._imageExts.png` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource._imageExts.svg` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource._imageExts.webp` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.prototype.abort` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.prototype.abort.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.prototype.complete` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.prototype.complete.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Minimal loader resource; constants follow resource-loader. Inventory test only. |
| `PIXI.LoaderResource.setExtensionLoadType` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.LoaderResource.setExtensionLoadType.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.LoaderResource.setExtensionXhrType` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.LoaderResource.setExtensionXhrType.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.Matrix` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.IDENTITY` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.IDENTITY.a` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.IDENTITY.b` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.IDENTITY.c` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.IDENTITY.d` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.IDENTITY.tx` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.IDENTITY.ty` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.TEMP_MATRIX` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.TEMP_MATRIX.a` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.TEMP_MATRIX.b` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.TEMP_MATRIX.c` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.TEMP_MATRIX.d` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.TEMP_MATRIX.tx` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.TEMP_MATRIX.ty` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.append` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.append.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.apply` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.apply.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.applyInverse` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.applyInverse.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.clone` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.clone.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.copyFrom` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.copyFrom.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.copyTo` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.copyTo.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.decompose` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.decompose.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.identity` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.identity.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.invert` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.invert.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.prepend` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.prepend.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.rotate` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.rotate.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.scale` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.scale.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.set` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.set.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.translate` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.Matrix.prototype.translate.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Affine matrix math as in PIXI 5.3.12. |
| `PIXI.ObjectRenderer` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Base class for plugin renderers; no native draw path. Inventory test only. |
| `PIXI.ObjectRenderer.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Base class for plugin renderers; no native draw path. Inventory test only. |
| `PIXI.ObjectRenderer.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Base class for plugin renderers; no native draw path. Inventory test only. |
| `PIXI.ObjectRenderer.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Base class for plugin renderers; no native draw path. Inventory test only. |
| `PIXI.ObjectRenderer.prototype.flush` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.ObjectRenderer.prototype.flush.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.ObjectRenderer.prototype.render` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.ObjectRenderer.prototype.render.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.ObjectRenderer.prototype.start` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.ObjectRenderer.prototype.start.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.ObjectRenderer.prototype.stop` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.ObjectRenderer.prototype.stop.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.ObservablePoint` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.clone` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.clone.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.copyFrom` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.copyFrom.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.copyTo` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.copyTo.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.equals` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.equals.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.set` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.set.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.ObservablePoint.prototype.y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Point with change callback. |
| `PIXI.Point` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.clone` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.clone.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.copyFrom` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.copyFrom.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.copyTo` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.copyTo.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.equals` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.equals.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.set` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Point.prototype.set.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Plain point; no observers. |
| `PIXI.Program` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores shader strings only; no program compilation (§4.2). |
| `PIXI.Program.from` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores shader strings only; no program compilation (§4.2). |
| `PIXI.Program.from.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores shader strings only; no program compilation (§4.2). |
| `PIXI.Program.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores shader strings only; no program compilation (§4.2). |
| `PIXI.Rectangle` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.EMPTY` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.EMPTY.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.EMPTY.type` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.EMPTY.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.EMPTY.x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.EMPTY.y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.bottom` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.ceil` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.ceil.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.clone` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.clone.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.contains` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.contains.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.copyFrom` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.copyFrom.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.copyTo` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.copyTo.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.enlarge` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.enlarge.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.fit` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.fit.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.left` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.pad` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.pad.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.right` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.Rectangle.prototype.top` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Rectangle math as in PIXI 5.3.12. |
| `PIXI.RenderTexture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native render target (FBO) via the renderer API. |
| `PIXI.RenderTexture.create` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native render target (FBO) via the renderer API. |
| `PIXI.RenderTexture.create.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native render target (FBO) via the renderer API. |
| `PIXI.RenderTexture.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native render target (FBO) via the renderer API. |
| `PIXI.RenderTexture.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native render target (FBO) via the renderer API. |
| `PIXI.RenderTexture.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native render target (FBO) via the renderer API. |
| `PIXI.RenderTexture.prototype.framebuffer` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native render target (FBO) via the renderer API. |
| `PIXI.RenderTexture.prototype.resize` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native render target (FBO) via the renderer API. |
| `PIXI.RenderTexture.prototype.resize.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Native render target (FBO) via the renderer API. |
| `PIXI.Renderer` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer._plugins` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype._extractCanvas` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype._extractCanvas.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype._extractPixels` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype._extractPixels.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype._updateBackgroundColor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype._updateBackgroundColor.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.clear` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.clear.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.render` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.render.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.reset` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.Renderer.prototype.reset.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.Renderer.prototype.resize` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.resize.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.prototype.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.registerPlugin` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.Renderer.registerPlugin.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Sprite-batch renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.SCALE_MODES` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; nearest/linear sampling. |
| `PIXI.SCALE_MODES.LINEAR` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; nearest/linear sampling. |
| `PIXI.SCALE_MODES.NEAREST` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Constant table; nearest/linear sampling. |
| `PIXI.Shader` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores program and uniforms only; never bound (§4.2). |
| `PIXI.Shader.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores program and uniforms only; never bound (§4.2). |
| `PIXI.Shader.prototype.destroy` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores program and uniforms only; never bound (§4.2). |
| `PIXI.Shader.prototype.destroy.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Stores program and uniforms only; never bound (§4.2). |
| `PIXI.Sprite` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.from` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.from.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype._calculateBounds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype._calculateBounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype._onAnchorUpdate` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype._onAnchorUpdate.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype._onTextureUpdate` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype._onTextureUpdate.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype._renderSelf` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype._renderSelf.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.anchor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.calculateTrimmedVertices` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.calculateTrimmedVertices.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.calculateVertices` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.calculateVertices.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.containsPoint` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.containsPoint.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.getLocalBounds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.getLocalBounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.render` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.render.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.texture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.tint` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.Sprite.prototype.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Drawn as a native quad (drawQuad*). |
| `PIXI.State` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Fields only; no GL state is applied (§4.2). |
| `PIXI.State.for2d` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Fields only; no GL state is applied (§4.2). |
| `PIXI.State.for2d.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Fields only; no GL state is applied (§4.2). |
| `PIXI.State.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_tilemap.c` | Fields only; no GL state is applied (§4.2). |
| `PIXI.TYPES` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.TYPES.FLOAT` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.TYPES.UNSIGNED_BYTE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.TYPES.UNSIGNED_SHORT` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.Text` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype._generateCanvas` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype._generateCanvas.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype._updateTexture` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype._updateTexture.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.getLocalBounds` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.getLocalBounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.render` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.render.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.resolution` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.style` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.text` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.updateText` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.Text.prototype.updateText.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Rendered through the Canvas2D shim, uploaded as a texture. Inventory test only. |
| `PIXI.TextStyle` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Subset of style fields mapped to a CSS font string. Inventory test only. |
| `PIXI.TextStyle.prototype.clone` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Subset of style fields mapped to a CSS font string. Inventory test only. |
| `PIXI.TextStyle.prototype.clone.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Subset of style fields mapped to a CSS font string. Inventory test only. |
| `PIXI.TextStyle.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Subset of style fields mapped to a CSS font string. Inventory test only. |
| `PIXI.TextStyle.prototype.toFontString` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Subset of style fields mapped to a CSS font string. Inventory test only. |
| `PIXI.TextStyle.prototype.toFontString.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Subset of style fields mapped to a CSS font string. Inventory test only. |
| `PIXI.Texture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.EMPTY` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.EMPTY._baseTexture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._glTexture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._glTextures` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._id` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._listeners` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._listeners.update` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._listeners.update.0` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._listeners.update.0.context` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._listeners.update.0.fn` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._listeners.update.0.fn.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._listeners.update.0.once` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture._listeners.update.length` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.alphaMode` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.cacheId` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.destroyed` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.dirtyId` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.dirtyStyleId` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.format` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.isPowerOfTwo` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.mipmap` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.realHeight` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.realWidth` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.resolution` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.resource` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.scaleMode` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.textureCacheIds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.textureCacheIds.length` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.type` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.uid` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.valid` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._baseTexture.wrapMode` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._frame` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._frame.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._frame.type` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._frame.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._frame.x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._frame.y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._id` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._listeners` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._rotate` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._updateID` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._uvs` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._uvs.x0` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._uvs.x1` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._uvs.x2` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._uvs.x3` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._uvs.y0` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._uvs.y1` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._uvs.y2` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY._uvs.y3` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.defaultAnchor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.defaultAnchor.x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.defaultAnchor.y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.destroy` | stub | `src/shims/pixi_shim.js` | - | No-op so the shared EMPTY texture survives destroy. |
| `PIXI.Texture.EMPTY.destroy.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op so the shared EMPTY texture survives destroy. |
| `PIXI.Texture.EMPTY.destroyed` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.emit` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.emit.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.noFrame` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.on` | stub | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Returns this and discards the listener; no event registration. |
| `PIXI.Texture.EMPTY.on.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Returns this and discards the listener; no event registration. |
| `PIXI.Texture.EMPTY.once` | stub | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Returns this and discards the listener; no event registration. |
| `PIXI.Texture.EMPTY.once.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Returns this and discards the listener; no event registration. |
| `PIXI.Texture.EMPTY.orig` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.orig.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.orig.type` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.orig.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.orig.x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.orig.y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.requiresUpdate` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.textureCacheIds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.textureCacheIds.length` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.trim` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.uid` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.uvMatrix` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.EMPTY.valid` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared EMPTY texture instance. |
| `PIXI.Texture.WHITE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.WHITE._baseTexture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture._glTexture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture._glTextures` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture._id` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture._listeners` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.alphaMode` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.cacheId` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.destroyed` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.dirtyId` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.dirtyStyleId` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.format` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.isPowerOfTwo` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.mipmap` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.realHeight` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.realWidth` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.resolution` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.resource` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.scaleMode` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.textureCacheIds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.textureCacheIds.length` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.type` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.uid` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.valid` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._baseTexture.wrapMode` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._frame` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._frame.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._frame.type` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._frame.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._frame.x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._frame.y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._id` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._listeners` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._rotate` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._updateID` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._uvs` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._uvs.x0` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._uvs.x1` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._uvs.x2` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._uvs.x3` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._uvs.y0` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._uvs.y1` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._uvs.y2` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE._uvs.y3` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.defaultAnchor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.defaultAnchor.x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.defaultAnchor.y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.destroy` | stub | `src/shims/pixi_shim.js` | - | No-op so the shared WHITE texture survives destroy. |
| `PIXI.Texture.WHITE.destroy.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op so the shared WHITE texture survives destroy. |
| `PIXI.Texture.WHITE.destroyed` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.emit` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.emit.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.noFrame` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.on` | stub | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Returns this and discards the listener; no event registration. |
| `PIXI.Texture.WHITE.on.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Returns this and discards the listener; no event registration. |
| `PIXI.Texture.WHITE.once` | stub | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Returns this and discards the listener; no event registration. |
| `PIXI.Texture.WHITE.once.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Returns this and discards the listener; no event registration. |
| `PIXI.Texture.WHITE.orig` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.orig.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.orig.type` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.orig.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.orig.x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.orig.y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.requiresUpdate` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.textureCacheIds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.textureCacheIds.length` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.trim` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.uid` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.uvMatrix` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.WHITE.valid` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Member of the shared WHITE texture instance. |
| `PIXI.Texture.addToCache` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.addToCache.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.from` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.from.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.fromCanvas` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.fromCanvas.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype._updateUvs` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype._updateUvs.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.baseTexture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.castToBaseTexture` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.castToBaseTexture.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.clone` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.clone.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.frame` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.onBaseTextureUpdated` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.onBaseTextureUpdated.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.rotate` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.update` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.update.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.prototype.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.removeFromCache` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Texture.removeFromCache.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Frame over a BaseTexture; no WebGL texture units. |
| `PIXI.Ticker` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.add` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.add.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.addOnce` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.addOnce.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.remove` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.remove.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.start` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.start.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.stop` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.stop.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.update` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.prototype.update.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.shared` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.Ticker.system` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | JS ticker driven by the host frame loop. |
| `PIXI.TilingSprite` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.from` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.from.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype._calculateBounds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype._calculateBounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.containsPoint` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.containsPoint.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.getLocalBounds` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.getLocalBounds.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.render` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.render.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.TilingSprite.prototype.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Tiled quads; no WebGL repeat sampler. |
| `PIXI.Transform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY._currentLocalID` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY._cx` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY._cy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY._localID` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY._parentID` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY._rotation` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY._sx` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY._sy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY._worldID` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.localTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.localTransform.a` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.localTransform.b` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.localTransform.c` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.localTransform.d` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.localTransform.tx` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.localTransform.ty` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.pivot` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.pivot._x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.pivot._y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.pivot.cb` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.pivot.cb.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.pivot.scope` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.position` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.position._x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.position._y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.position.cb` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.position.cb.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.position.scope` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.scale` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.scale._x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.scale._y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.scale.cb` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.scale.cb.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.scale.scope` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.skew` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.skew._x` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.skew._y` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.skew.cb` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.skew.cb.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.skew.scope` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.worldTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.worldTransform.a` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.worldTransform.b` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.worldTransform.c` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.worldTransform.d` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.worldTransform.tx` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.IDENTITY.worldTransform.ty` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.onChange` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.onChange.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.onSkewChange` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.onSkewChange.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.rotation` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.setFromMatrix` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.setFromMatrix.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.updateLocalTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.updateLocalTransform.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.updateTransform` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.Transform.prototype.updateTransform.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Local/world transform update. |
| `PIXI.VERSION` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Reports 5.3.12. Inventory test only. |
| `PIXI.WRAP_MODES` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.WRAP_MODES.CLAMP` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.WRAP_MODES.MIRRORED_REPEAT` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.WRAP_MODES.REPEAT` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Constant table only. Inventory test only. |
| `PIXI.WebGLRenderer` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer._plugins` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype._extractCanvas` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype._extractCanvas.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype._extractPixels` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype._extractPixels.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype._updateBackgroundColor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype._updateBackgroundColor.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.clear` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.clear.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.destroy` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.destroy.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.render` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.render.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.reset` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.WebGLRenderer.prototype.reset.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.WebGLRenderer.prototype.resize` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.resize.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.prototype.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.registerPlugin` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI.WebGLRenderer.registerPlugin.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Alias of Renderer; no renderTexture, filter, mask, context, buffer or plugins.extract (§2, §4.2). |
| `PIXI._renderSceneGraph` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Internal scene-graph walk used by the renderer. |
| `PIXI._renderSceneGraph.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Internal scene-graph walk used by the renderer. |
| `PIXI.filters` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.AlphaFilter` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.AlphaFilter.prototype.apply` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.AlphaFilter.prototype.apply.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.AlphaFilter.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.BlurFilter` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.BlurFilter.prototype.apply` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.BlurFilter.prototype.apply.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.BlurFilter.prototype.blur` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.BlurFilter.prototype.blurX` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.BlurFilter.prototype.blurY` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.BlurFilter.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.BlurFilterPass` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.BlurFilterPass.prototype.apply` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.filters.BlurFilterPass.prototype.apply.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.filters.BlurFilterPass.prototype.blur` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.BlurFilterPass.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype._copyMatrix` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype._copyMatrix.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype._loadMatrix` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype._loadMatrix.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype._multiply` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype._multiply.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.apply` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.apply.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.brightness` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.brightness.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.contrast` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.contrast.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.desaturate` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.desaturate.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.grayscale` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.grayscale.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.greyscale` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.greyscale.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.hue` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.hue.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.negative` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.negative.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.reset` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.reset.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.saturate` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.saturate.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.sepia` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.sepia.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.tint` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.filters.ColorMatrixFilter.prototype.tint.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_filters.c` | Built-in filters mapped to native filter passes. |
| `PIXI.groupD8` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.E` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.MIRROR_HORIZONTAL` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.MIRROR_VERTICAL` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.N` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.NE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.NW` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.S` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.SE` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.SW` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.W` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.add` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.add.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.byDirection` | stub | `src/shims/pixi_shim.js` | - | Fixed 0; direction lookup not implemented. |
| `PIXI.groupD8.byDirection.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | Fixed 0; direction lookup not implemented. |
| `PIXI.groupD8.inv` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.inv.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.isVertical` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.isVertical.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.matrixAppendRotationInv` | stub | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Empty body; ignores the matrix, rotation and translation arguments. |
| `PIXI.groupD8.matrixAppendRotationInv.prototype.constructor` | stub | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Empty body; ignores the matrix, rotation and translation arguments. |
| `PIXI.groupD8.rotate180` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.rotate180.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.sub` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.sub.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.uX` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.uX.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.uY` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.uY.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.vX` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.vX.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.vY` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.groupD8.vY.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | D8 rotation group tables and helpers. Inventory test only. |
| `PIXI.loader` | partial | `src/shims/pixi_shim.js` | `tests/smz/test_dump_surface.c` | Shared Loader instance. Inventory test only. |
| `PIXI.settings` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.CREATE_IMAGE_BITMAP` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.FILTER_RESOLUTION` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.GC_MAX_CHECK_COUNT` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.GC_MAX_IDLE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.GC_MODE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.MIPMAP_TEXTURES` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.PREFER_ENV` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS.antialias` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS.autoDensity` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS.backgroundColor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS.clearBeforeRender` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS.height` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS.preserveDrawingBuffer` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS.resolution` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS.transparent` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS.view` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RENDER_OPTIONS.width` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.RESOLUTION` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.ROUND_PIXELS` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.SCALE_MODE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.SPRITE_MAX_TEXTURES` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.STRICT_TEXTURE_CACHE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.TARGET_FPMS` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.settings.WRAP_MODE` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Settings object; few fields affect native rendering. |
| `PIXI.utils` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.BaseTextureCache` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.addListener` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.addListener.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.emit` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.emit.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.listenerCount` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.listenerCount.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.listeners` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.listeners.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.off` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.off.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.on` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.on.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.once` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.once.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.removeAllListeners` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.removeAllListeners.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.removeListener` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.EventEmitter.prototype.removeListener.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.TextureCache` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.createIndicesForQuads` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.createIndicesForQuads.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.destroyTextureCache` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.destroyTextureCache.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.hex2rgb` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.hex2rgb.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.hex2string` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.hex2string.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.isMobile` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.amazon` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.amazon.device` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.amazon.phone` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.amazon.tablet` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.android` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.android.device` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.android.phone` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.android.tablet` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.any` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.apple` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.apple.device` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.apple.ipod` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.apple.phone` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.apple.tablet` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.other` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.other.blackberry` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.other.blackberry10` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.other.chrome` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.other.device` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.other.firefox` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.other.opera` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.phone` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.tablet` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.windows` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.windows.device` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.windows.phone` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isMobile.windows.tablet` | stub | `src/shims/pixi_shim.js` | - | Fixed desktop answer; no user-agent parsing. |
| `PIXI.utils.isWebGLSupported` | stub | `src/shims/pixi_shim.js` | - | Fixed true; the renderer is not WebGL. |
| `PIXI.utils.isWebGLSupported.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | Fixed true; the renderer is not WebGL. |
| `PIXI.utils.premultiplyTint` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.premultiplyTint.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.premultiplyTintToRgba` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.premultiplyTintToRgba.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.rgb2hex` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.rgb2hex.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.sign` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.sign.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.skipHello` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.utils.skipHello.prototype.constructor` | stub | `src/shims/pixi_shim.js` | - | No-op. |
| `PIXI.utils.string2hex` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.string2hex.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.uid` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
| `PIXI.utils.uid.prototype.constructor` | partial | `src/shims/pixi_shim.js` | `tests/test_pixi_shim.c` | Helper subset of PIXI.utils. |
