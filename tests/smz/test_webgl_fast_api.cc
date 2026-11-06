/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Proves the WebGL2 hot calls are registered through V8 Fast API/CFunction.
 *
 * Why: a slow jsb callback would also make `gl.bindBuffer(...)` work, so a
 * functional test cannot tell the two paths apart. This test reads the names
 * recorded where each v8::CFunction is installed and checks that every hot
 * call named by spec section 4.2 is there. It needs no GL device. Main thread
 * only; the engine owns every handle (docs/js-binding.md).
 */
#include "engine/js_engine.h"
#include "webgl/webgl.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Hot calls the card requires to use the fast path. */
static const char *const kRequiredFastCalls[] = {
    "bindBuffer",      "bufferSubData",   "drawElements",
    "bindTexture",     "bindVertexArray", "useProgram",
    "uniform1f",       "uniform1i",       "uniform4f",
    "uniform1fv",      "uniform4fv",      "uniformMatrix4fv",
};

/* Returns true when `name` is in the registered fast-call list. */
static bool has_fast_call(const char *name)
{
    for (size_t i = 0; i < webgl_fast_api_count(); i++) {
        const char *registered = webgl_fast_api_name(i);
        if (registered && strcmp(registered, name) == 0) return true;
    }
    return false;
}

/* Evaluates `script` and returns a newly allocated string, or NULL. */
static char *eval(JSEngine *engine, const char *script)
{
    return js_engine_eval_string(engine, script, "webgl_fast_api.js");
}

/* Checks that the C registry and the JS view agree on every hot call. */
static void check_registry(JSEngine *engine)
{
    assert(webgl_fast_api_count() > 0);
    for (size_t i = 0; i < sizeof(kRequiredFastCalls) /
                               sizeof(kRequiredFastCalls[0]);
         i++) {
        const char *name = kRequiredFastCalls[i];
        assert(has_fast_call(name));
    }

    char *js_count = eval(engine, "__native_webgl.fastApiInfo().count");
    assert(js_count);
    char wanted[32];
    snprintf(wanted, sizeof(wanted), "%zu", webgl_fast_api_count());
    assert(strcmp(js_count, wanted) == 0);
    js_engine_free_string(js_count);

    char *js_names = eval(engine,
                         "__native_webgl.fastApiInfo().names.join(',')");
    assert(js_names);
    for (size_t i = 0; i < sizeof(kRequiredFastCalls) /
                               sizeof(kRequiredFastCalls[0]);
         i++) {
        assert(strstr(js_names, kRequiredFastCalls[i]) != NULL);
    }
    js_engine_free_string(js_names);
}

/* Runs the checks; the fast registry exists even without a GL device. */
int main(void)
{
    JSEngine *engine = js_engine_init();
    assert(engine);
    assert(webgl_define(js_engine_get_jsb(engine)));

    check_registry(engine);
    js_engine_shutdown(engine);
    printf("webgl_fast_api: %zu fast calls registered\n",
           webgl_fast_api_count());
    return 0;
}
