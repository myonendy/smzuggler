/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Enumerates shim additions without browser devices or game startup.
 * See docs/shims.md. The calling thread owns the isolated engine; descriptors
 * are inspected without invoking getters or any native device bindings.
 */
#include "smzuggler/dump_surface.h"
#include "engine/js_engine.h"
#include "engine/script_loader.h"
#include "bindings/bind_io.h"
#include "io/file_io.h"
#include <stdlib.h>
#include <string.h>

/* Keep the observer in a closure so its state cannot pollute the inventory.
 * Snapshot data descriptors as references, allowing replaced members to be
 * attributed to the last shim that installs them. Accessors are never called.
 * The global alias and ancestor checks prevent window/document cycles.
 * Spec §15.4 counts only API members (DEC-M0-T009b): a function's intrinsic
 * `length`, `name` and `prototype` get no row (the prototype's members still
 * do), and DOM elements reached from `document` (own string `tagName`) get one
 * row each without their members, which repeat HTMLElement.prototype. */
static const char observer[] =
    "globalThis.__smz_surface = (function(){"
    "const root=globalThis, rows=new Map(); let before;"
    "const intrinsic=['length','name','prototype'];"
    "/** True for a DOM element under document; reads a descriptor, never a getter. */"
    "function element(v,path){if(path!=='document'&&!path.startsWith('document.'))return false;"
    "const t=Object.getOwnPropertyDescriptor(v,'tagName');"
    "return !!t&&('value' in t)&&typeof t.value==='string';}"
    "/** Snapshot own descriptors without executing browser getters. */"
    "function snapshot(){const out=new Map();"
    "/** Follow namespaces and constructor prototypes; ancestors break cycles. */"
    "function walk(obj,prefix,ancestors){"
    "for(const key of Reflect.ownKeys(obj).sort((a,b)=>String(a)<String(b)?-1:String(a)>String(b)?1:0)){"
    "if(key==='__smz_surface')continue;"
    "const d=Object.getOwnPropertyDescriptor(obj,key);"
    "const label=typeof key==='symbol'?'['+String(key)+']':key;"
    "const path=prefix?prefix+'.'+label:label;"
    "const v=d.value, kind=('value' in d)?typeof v:'accessor';"
    "const skip=typeof obj==='function'&&intrinsic.includes(key);"
    "if(!skip)out.set(path,{kind:kind,value:v,get:d.get,set:d.set});"
    "if(v && (typeof v==='object'||typeof v==='function') && v!==root && !ancestors.includes(v)"
    " && (!skip||key==='prototype') && !element(v,path))"
    "walk(v,path,ancestors.concat([v]));"
    "}}walk(root,'',[root]);return out;}"
    "before=snapshot();"
    "/** Attribute additions and replacements after one shim evaluation. */"
    "return function(file){if(file===null)return Array.from(rows.keys()).sort()"
    ".map(k=>k+'\\t'+rows.get(k).kind+'\\t'+rows.get(k).file).join('\\n')+'\\n';"
    "const after=snapshot();for(const [k,d] of after){const b=before.get(k);"
    "if(!b||b.kind!==d.kind||!Object.is(b.value,d.value)||b.get!==d.get||b.set!==d.set)"
    "rows.set(k,{kind:d.kind,file:file});}"
    "for(const k of rows.keys())if(!after.has(k))rows.delete(k);"
    "before=after;return '';};})();";

/** Keep shim diagnostics off the machine-readable stream; borrowed arguments
 * are consumed synchronously on the engine thread and errors go to stderr. */
static void console_message(const char *level, const char *message, void *userdata)
{
    (void)userdata;
    fprintf(stderr, "dump-surface: %s: %s\n", level, message);
}

/** Evaluate the loader's ordered shim names and observe each successful load.
 * All paths are temporary owned allocations; false propagates engine errors. */
static bool load_observed(JSEngine *engine, const char *dir)
{
    const char *const *files = script_loader_shim_files();
    for (size_t i = 0; files[i]; ++i) {
        size_t size = strlen(dir) + strlen(files[i]) + 2;
        char *path = malloc(size);
        if (!path) return false;
        snprintf(path, size, "%s/%s", dir, files[i]);
        bool ok = js_engine_eval_file(engine, path);
        free(path);
        if (!ok || js_engine_execute_pending_jobs(engine) < 0) return false;
        char code[256];
        int n = snprintf(code, sizeof(code), "__smz_surface('%s')", files[i]);
        if (n < 0 || (size_t)n >= sizeof(code) ||
            !js_engine_eval(engine, code, "<surface-observe>")) return false;
    }
    return true;
}

/** Build an isolated inventory on the caller thread, borrowing the stream and
 * directory; restore the host file root even when shim evaluation fails. */
bool dump_surface(FILE *output, const char *shim_dir)
{
    if (!output || !shim_dir) return false;
    JSEngine *engine = js_engine_init();
    if (!engine) return false;
    char *old_root = strdup(file_io_get_game_root());
    if (!old_root) {
        js_engine_shutdown(engine);
        return false;
    }
    file_io_set_game_root(shim_dir);
    bind_io_define(js_engine_get_jsb(engine));
    js_engine_set_console_callback(engine, console_message, NULL);
    bool ok = js_engine_eval(engine, observer, "<surface-observer>") &&
              load_observed(engine, shim_dir);
    char *text = ok ? js_engine_eval_string(engine, "__smz_surface(null)",
                                           "<surface-result>") : NULL;
    ok = text && fputs(text, output) >= 0 && !ferror(output);
    js_engine_free_string(text);
    js_engine_shutdown(engine);
    file_io_set_game_root(old_root);
    free(old_root);
    return ok;
}

/** Own only a requested output file; report failures without starting devices. */
int dump_surface_run(const char *path, const char *shim_dir)
{
    FILE *output = path ? fopen(path, "w") : stdout;
    if (!output) {
        perror("dump-surface: open output");
        return 1;
    }
    bool ok = dump_surface(output, shim_dir);
    if (path ? fclose(output) != 0 : fflush(output) != 0) ok = false;
    if (!ok) fprintf(stderr, "dump-surface: inventory failed\n");
    return ok ? 0 : 1;
}
