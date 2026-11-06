/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Proves the WebGL2 hot calls are registered through V8 Fast API/CFunction.
 *
 * Why: a slow jsb callback would also work, so this reads the names recorded
 * where each v8::CFunction is installed and checks every spec section 4.2 hot
 * call. It needs no GL device. Main thread only; the engine owns the handles.
 */
#include "engine/js_engine.h"
#include "webgl/webgl.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Hot calls the card requires on the fast path. */
static const char *const kRequiredFastCalls[] = {
    "bindBuffer",      "bufferSubData",   "drawElements",
    "bindTexture",     "bindVertexArray", "useProgram",
    "uniform1f",       "uniform1i",       "uniform4f",
    "uniform1fv",      "uniform4fv",      "uniformMatrix4fv",
};

/* Reports `message` and exits; explicit so Release's NDEBUG cannot skip it. */
static void check(bool ok, const char *message)
{
    assert(message != NULL);
    if (ok) return;
    fprintf(stderr, "webgl_fast_api: %s\n", message);
    assert(ok);
    exit(1);
}

/* Returns true when `name` is in the registered fast-call list. */
static bool has_fast_call(const char *name)
{
    for (size_t i = 0; i < webgl_fast_api_count(); i++) {
        const char *registered = webgl_fast_api_name(i);
        if (registered && strcmp(registered, name) == 0) return true;
    }
    return false;
}

/* Checks that the C registry and the JS view agree on every hot call. */
static void check_registry(JSEngine *engine)
{
    check(webgl_fast_api_count() > 0, "no fast calls registered");
    for (size_t i = 0; i < sizeof(kRequiredFastCalls) /
                               sizeof(kRequiredFastCalls[0]); i++) {
        char message[96];
        snprintf(message, sizeof(message), "%s is not registered as a fast call",
                 kRequiredFastCalls[i]);
        check(has_fast_call(kRequiredFastCalls[i]), message);
    }
    char *js_count = js_engine_eval_string(engine,
                                           "__native_webgl.fastApiInfo().count",
                                           "webgl_fast_api.js");
    char wanted[32];
    snprintf(wanted, sizeof(wanted), "%zu", webgl_fast_api_count());
    check(js_count && strcmp(js_count, wanted) == 0,
          "fastApiInfo().count differs from the C registry");
    js_engine_free_string(js_count);
    char *js_names = js_engine_eval_string(
        engine, "__native_webgl.fastApiInfo().names.join(',')",
        "webgl_fast_api.js");
    check(js_names != NULL, "fastApiInfo().names is not available");
    for (size_t i = 0; i < sizeof(kRequiredFastCalls) /
                               sizeof(kRequiredFastCalls[0]); i++) {
        char message[96];
        snprintf(message, sizeof(message), "the JS view is missing %s",
                 kRequiredFastCalls[i]);
        check(strstr(js_names, kRequiredFastCalls[i]) != NULL, message);
    }
    js_engine_free_string(js_names);
}

/* Runs the checks; the fast registry exists even without a GL device. */
int main(void)
{
    JSEngine *engine = js_engine_init();
    check(engine != NULL, "js_engine_init failed");
    check(webgl_define(js_engine_get_jsb(engine)), "webgl_define failed");
    check_registry(engine);
    printf("webgl_fast_api: %zu fast calls registered\n",
           webgl_fast_api_count());
    webgl_shutdown();
    js_engine_shutdown(engine);
    return 0;
}
