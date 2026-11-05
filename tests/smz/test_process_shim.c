/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Game-free checks of the §5.3 process surface and throwing require.
 * See docs/shims.md and docs/compat-surface.md. Main-thread only; the
 * engine, IO root and temporary directory are owned by this process.
 */
#include "bindings/bind_io.h"
#include "bindings/bind_platform.h"
#include "engine/js_engine.h"
#include "io/file_io.h"
#include "test_paths.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static JSEngine *s_js;

/** True when SCRIPT evaluates to the string "true"; prints the value on mismatch. */
static int js_true(const char *script)
{
    char *result = js_engine_eval_string(s_js, script, "<process-shim>");
    int ok = result && strcmp(result, "true") == 0;
    if (!ok) fprintf(stderr, "process_shim: got %s for %s\n",
                     result ? result : "(null)", script);
    js_engine_free_string(result);
    return ok;
}

/** Load DOM then storage shims against ROOT; caller must js_engine_shutdown. */
static void setup(const char *root)
{
    s_js = js_engine_init();
    assert(s_js);
    bind_io_define(js_engine_get_jsb(s_js));
    bind_platform_define_process(js_engine_get_jsb(s_js));
    file_io_set_game_root(root);
    assert(js_engine_eval_file(s_js, "src/shims/dom_shim.js"));
    assert(js_engine_execute_pending_jobs(s_js) >= 0);
    assert(js_engine_eval_file(s_js, "src/shims/storage_shim.js"));
    assert(js_engine_execute_pending_jobs(s_js) >= 0);
}

/** Release the engine and IO root; ROOT is deleted by the caller. */
static void teardown(void)
{
    js_engine_shutdown(s_js);
    s_js = NULL;
    file_io_set_game_root("");
}

/** Assert argv, cwd, mainModule, exit, and require contracts without SDL. */
int main(void)
{
    const char *root = test_tmp_path("smz-process-shim");
    test_rm_rf(root);
    assert(test_mkdir_p(root));
    setup(root);

    assert(js_true("process.argv instanceof Array && process.argv.length === 2"));
    assert(js_true("process.argv[0] === 'chrome'"));
    assert(js_true("process.argv[1] === 'index.html'"));
    assert(js_true("process.cwd() === __native_io.getGameRoot()"));
    assert(js_true(
        "process.mainModule.filename === __native_io.getGameRoot() + '/index.html'"
    ));
    assert(js_true(
        "nw.Window.get().on('close', function() {});"
        "process.exit(7);"
        "process.exitCode === 7 && __nw_hostClose() === true"
    ));
    assert(js_true("typeof require('fs').existsSync === 'function'"));
    assert(js_true("typeof require('path').join === 'function'"));
    assert(js_true("require('nw.gui') === nw"));
    assert(js_true(
        "(function(){"
        "  try { require('nope'); return false; }"
        "  catch (e) {"
        "    return e instanceof Error && e.code === 'MODULE_NOT_FOUND' &&"
        "      String(e.message).indexOf('Cannot find module') >= 0;"
        "  }"
        "})()"
    ));
    assert(js_true("typeof require('fs').readFileSync === 'function'"));

    teardown();
    test_rm_rf(root);
    return 0;
}
