/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Prove recursion stays catchable on the process main thread's native stack.
 * See docs/js-binding.md. The engine and its values stay on this thread.
 */
#include "engine/js_engine.h"
#include <assert.h>

/** On the process main thread, check repeated overflow and recovery.
 * No pointers are retained; returns zero after releasing the engine. */
static int exercise(void)
{
    JSEngine *engine = js_engine_init();
    assert(engine);
    for (int i = 0; i < 3; ++i) {
        assert(js_engine_eval(engine,
            "var caught = false; try { (function recurse() { recurse(); })(); }"
            "catch (e) { caught = e instanceof RangeError && "
            "e.message === 'Maximum call stack size exceeded'; }"
            "if (!caught) throw new Error('overflow was not catchable');",
            "recursion.js"));
    }
    assert(js_engine_eval(engine, "if (6 * 7 !== 42) throw Error('recovery');", "recovery.js"));
    js_engine_shutdown(engine);
    return 0;
}

/** Run on the entry thread; returns zero after all overflow checks pass. */
int main(void)
{
    return exercise();
}
