/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Prove recursion stays catchable on the host's reserved runtime stack.
 * See docs/js-binding.md. The engine and its values stay on this thread.
 */
#include "engine/js_engine.h"
#include <SDL3/SDL.h>
#include <assert.h>

/** On the runtime thread, check repeated overflow and subsequent recovery.
 * No pointers are retained; returns zero after releasing the engine. */
static int SDLCALL exercise(void *unused)
{
    (void)unused;
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

/** Reserve the same stack as the host; join before returning its exit code. */
int main(void)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    assert(props);
    assert(SDL_SetPointerProperty(props, SDL_PROP_THREAD_CREATE_ENTRY_FUNCTION_POINTER, exercise));
    assert(SDL_SetStringProperty(props, SDL_PROP_THREAD_CREATE_NAME_STRING, "stack-test"));
    assert(SDL_SetNumberProperty(props, SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER, SMZ_RUNTIME_STACK_BYTES));
    SDL_Thread *thread = SDL_CreateThreadWithProperties(props);
    SDL_DestroyProperties(props);
    assert(thread);
    int result = 1;
    SDL_WaitThread(thread, &result);
    return result;
}
