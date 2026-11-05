/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * TRANSITIONAL, private: lets the QuickJS jsb backend wrap the engine's own
 * QuickJS context. Removed by card M1-T027 with QuickJS itself.
 *
 * Why: until M1-T027 the runtime runs on js_engine.c (QuickJS) in both
 * engine modes, and bindings ported to jsb must define their modules in
 * the game's global object, not in a second context of their own
 * (docs/decisions DEC-M1-T014-JSB-ADOPT, docs/js-binding.md "Adoption").
 *
 * Rules a reader must not break:
 * - Only src/jsb/, src/engine/js_engine.c and code guarded by
 *   `#ifdef SMZ_JSB_ADOPT` (defined by the smz_jsb_quickjs library) include
 *   this header; the V8 backend has no equivalent.
 * - An adopted runtime never owns the context: jsb_runtime_destroy on it
 *   frees jsb's own state only, and must run before the context is freed.
 * - jsb keeps its runtime in the QuickJS *runtime* opaque; the context
 *   opaque belongs to js_engine.c. Main thread only.
 */
#ifndef SMZ_JSB_ADOPT_H
#define SMZ_JSB_ADOPT_H

#include "jsb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct JSContext;

/* Wraps the existing context `js` in a jsb runtime without owning it.
 *
 * Why: js_engine_init adopts its context so js_engine_get_jsb() shares the
 * game's globals. Takes the DataView getters and, with "--expose-gc", adds
 * gc(), as jsb_runtime_create does. Returns NULL (with the reason on
 * stderr) when `js` is NULL, its JSRuntime already has a jsb runtime or an
 * other runtime opaque, or memory runs out. Free with jsb_runtime_destroy
 * before JS_FreeContext(js). Main thread only. */
jsb_runtime *jsb_runtime_adopt(struct JSContext *js);

/* Returns the jsb runtime of `js` (adopted or created), or NULL when it has
 * none or `js` is NULL.
 *
 * Why: the port cards' transitional bind_<x>_register(JSContext *) wrappers
 * reach their bind_<x>_define(jsb_runtime *) through it. Borrowed. Main
 * thread only. */
jsb_runtime *jsb_runtime_of(struct JSContext *js);

#ifdef __cplusplus
}
#endif

#endif /* SMZ_JSB_ADOPT_H */
