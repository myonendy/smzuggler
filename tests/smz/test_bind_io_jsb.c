/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Checks that __native_io, ported to jsb by card M1-T016, keeps its JS API
 * byte-for-byte: property names, each function's typeof and length, result
 * shapes and argument errors.
 *
 * Why: --dump-surface lists no native modules and no `length`
 * (DEC-M0-T009b), so this test is what shows the port changed nothing
 * (DEC-M1-T014-JSB-ADOPT). Every expected string below matches the pre-port
 * QuickJS binary (bind_io_register on js_engine.c). It calls bind_io_define
 * on js_engine_get_jsb(): js_engine.c in the default engine mode,
 * smz_js_engine_v8 in v8 mode, so the port really runs on V8 there. All
 * files live in a fresh temporary game root; paths in messages are shown as
 * ROOT. Main thread only; see docs/js-binding.md for the handle rules.
 */

#include "bindings/bind_io.h"
#include "engine/js_engine.h"
#include "io/file_io.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Evaluates `script` and asserts its String() result is `expected`.
 *
 * Why: one helper keeps each check a single line and prints both sides on
 * a mismatch, so a failing check names what changed. Aborts on failure. */
static void expect(JSEngine *engine, const char *script, const char *expected)
{
    char *got = js_engine_eval_string(engine, script, "test_bind_io_jsb");
    if (!got || strcmp(got, expected) != 0) {
        fprintf(stderr, "FAIL: %s\n  expected: %s\n  got:      %s\n", script,
                expected, got ? got : "(exception)");
        assert(0);
    }
    js_engine_free_string(got);
}

/* Defines the JS helper `err(f)`, which runs `f` and returns the thrown
 * error as "name: message" with the game root replaced by ROOT. */
static void define_helpers(JSEngine *engine)
{
    expect(engine,
           "var io=__native_io;"
           "function err(f){try{f();return 'no error'}catch(e){"
           "return (e.name+': '+e.message).split(io.getGameRoot()).join('ROOT')}}"
           "typeof err",
           "function");
}

/* Checks the module's own properties and each function's typeof/length. */
static void check_shape(JSEngine *engine)
{
    expect(engine, "typeof __native_io", "object");
    expect(engine, "Object.getOwnPropertyNames(__native_io).join()",
           "readFileSync,readFileBinary,writeFileSync,writeFileBinary,"
           "mkdirSync,existsSync,isDirSync,readdirSync,unlinkSync,renameSync,"
           "getGameRoot,resolvePath");
    /* JS_CFUNC_DEF made the functions non-enumerable but writable and
     * configurable. */
    expect(engine, "Object.keys(__native_io).length", "0");
    expect(engine,
           "var d=Object.getOwnPropertyDescriptor(__native_io,'renameSync');"
           "[d.writable,d.enumerable,d.configurable].join()",
           "true,false,true");
    expect(engine,
           "Object.getOwnPropertyNames(io).map(function(n){var f=io[n];"
           "return typeof f+':'+f.length+':'+f.name}).join()",
           "function:1:readFileSync,function:1:readFileBinary,"
           "function:2:writeFileSync,function:2:writeFileBinary,"
           "function:1:mkdirSync,function:1:existsSync,function:1:isDirSync,"
           "function:1:readdirSync,function:1:unlinkSync,"
           "function:2:renameSync,function:0:getGameRoot,"
           "function:1:resolvePath");
}

/* Checks the result of every function on real files in the game root. */
static void check_results(JSEngine *engine, const char *root)
{
    char script[1200];
    snprintf(script, sizeof script, "io.getGameRoot()==='%s'", root);
    expect(engine, script, "true");
    expect(engine, "io.resolvePath('a/b%20c.txt').split(io.getGameRoot()).join('ROOT')",
           "ROOT/a/b c.txt");
    expect(engine, "io.mkdirSync('save')", "true");
    expect(engine, "[io.existsSync('save'),io.isDirSync('save'),"
           "io.existsSync('nope'),io.isDirSync('nope')].join()",
           "true,true,false,false");
    expect(engine, "String(io.writeFileSync('save/t.txt','h\\u00e9\\u0000x'))",
           "undefined");
    expect(engine, "var s=io.readFileSync('save/t.txt');[s.length,"
           "s.charCodeAt(1),s.charCodeAt(2)].join()", "4,233,0");
    /* ToString coercion of the data and of the path, as the old C-string read did. */
    expect(engine, "io.writeFileSync({toString:function(){return 'save/n.txt'}},"
           "12.5);io.readFileSync('save/n.txt')", "12.5");
    expect(engine, "var u=new Uint8Array([1,2,255]);"
           "String(io.writeFileBinary('save/b.bin',u.buffer))", "undefined");
    expect(engine, "var b=io.readFileBinary('save/b.bin');[b instanceof "
           "ArrayBuffer,b.byteLength,new Uint8Array(b).join('-')].join()",
           "true,3,1-2-255");
    expect(engine, "io.writeFileBinary('save/e.bin',new ArrayBuffer(0));"
           "io.readFileBinary('save/e.bin').byteLength", "0");
    expect(engine, "var l=io.readdirSync('save');[Array.isArray(l),"
           "l.slice().sort().join()].join()", "true,b.bin,e.bin,n.txt,t.txt");
    expect(engine, "[io.renameSync('save/n.txt','save/m.txt'),"
           "io.existsSync('save/m.txt'),io.renameSync('save/x','save/y')].join()",
           "true,true,false");
    expect(engine, "[io.unlinkSync('save/m.txt'),io.unlinkSync('save/m.txt')]"
           ".join()", "true,false");
}

/* Checks the errors for missing arguments: a TypeError per function. */
static void check_arity_errors(JSEngine *engine)
{
    expect(engine,
           "Object.getOwnPropertyNames(io).map(function(n){"
           "return err(function(){io[n]()})}).join('|')",
           "TypeError: readFileSync requires a path argument|"
           "TypeError: readFileBinary requires a path argument|"
           "TypeError: writeFileSync requires path and data arguments|"
           "TypeError: writeFileBinary requires path and data arguments|"
           "TypeError: mkdirSync requires a path argument|"
           "TypeError: existsSync requires a path argument|"
           "TypeError: isDirSync requires a path argument|"
           "TypeError: readdirSync requires a path argument|"
           "TypeError: unlinkSync requires a path argument|"
           "TypeError: renameSync requires oldPath and newPath arguments|"
           "no error|"
           "TypeError: resolvePath requires a path argument");
    expect(engine, "err(function(){io.writeFileSync('save/z.txt')})",
           "TypeError: writeFileSync requires path and data arguments");
}

/* Checks the errors for wrong arguments and failing file operations. The
 * Symbol message is the engine's own conversion error, so it differs. */
static void check_errors(JSEngine *engine)
{
    expect(engine, "err(function(){io.existsSync('../x')})",
           "TypeError: path resolution failed: ../x");
    expect(engine, "err(function(){io.resolvePath('/elsewhere')})",
           "TypeError: path resolution failed: /elsewhere");
    expect(engine, "err(function(){io.readFileSync('nope.txt')})",
           "InternalError: failed to read file: ROOT/nope.txt");
    expect(engine, "err(function(){io.readFileBinary('nope.bin')})",
           "InternalError: failed to read binary file: ROOT/nope.bin");
    expect(engine, "err(function(){io.readdirSync('nope')})",
           "InternalError: failed to read directory: ROOT/nope");
    expect(engine, "err(function(){io.writeFileSync('nope/a/b.txt','x')})",
           "InternalError: failed to write file: ROOT/nope/a/b.txt");
    expect(engine, "err(function(){io.writeFileBinary('nope/a/b.bin',"
           "new ArrayBuffer(1))})",
           "InternalError: failed to write binary file: ROOT/nope/a/b.bin");
    expect(engine,
           "[new Uint8Array(2),new DataView(new ArrayBuffer(2)),'abc',null]"
           ".map(function(v){return err(function(){"
           "io.writeFileBinary('save/v.bin',v)})}).join('|')",
           "TypeError: writeFileBinary: second argument must be an ArrayBuffer|"
           "TypeError: writeFileBinary: second argument must be an ArrayBuffer|"
           "TypeError: writeFileBinary: second argument must be an ArrayBuffer|"
           "TypeError: writeFileBinary: second argument must be an ArrayBuffer");
    expect(engine, "[io.existsSync('save/v.bin'),err(function(){"
           "io.readFileSync({toString:function(){throw new RangeError('ts')}})})]"
           ".join()", "false,RangeError: ts");
#ifdef SMZ_JS_ENGINE_V8
    const char *symbol_error = "TypeError: Cannot convert a Symbol value to a string";
#else
    const char *symbol_error = "TypeError: cannot convert symbol to string";
#endif
    expect(engine, "err(function(){io.existsSync(Symbol())})", symbol_error);
    expect(engine, "var e;try{io.readFileSync('nope.txt')}catch(x){e=x}"
           "[e instanceof Error,String(e).indexOf('InternalError: ')].join()",
           "true,0");
}

/* Checks the save-path guard: relative writes are refused with the error a
 * failed write gives, and the refused file is not created. */
static void check_write_guard(JSEngine *engine)
{
    file_io_set_write_guard(true);
    expect(engine, "err(function(){io.writeFileSync('save/g.txt','x')})",
           "InternalError: failed to write file: save/g.txt");
    expect(engine, "err(function(){io.mkdirSync('g')})",
           "InternalError: failed to write file: g");
    expect(engine, "err(function(){io.renameSync('save/t.txt','save/u.txt')})",
           "InternalError: failed to write file: save/t.txt");
    file_io_set_write_guard(false);
    expect(engine, "[io.existsSync('save/g.txt'),io.existsSync('g'),"
           "io.existsSync('save/t.txt')].join()", "false,false,true");
}

/* Removes the files the checks left in `root`. */
static void cleanup(const char *root)
{
    const char *names[] = { "save/t.txt", "save/b.bin", "save/e.bin", "save" };
    char path[1100];
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        snprintf(path, sizeof path, "%s/%s", root, names[i]);
        remove(path);
    }
    remove(root);
}

/* Runs the API checks on the main thread in a private temporary game root. */
int main(void)
{
    const char *tmp = getenv("TMPDIR");
    if (!tmp || !*tmp) tmp = "/tmp";
    char dir[1024];
    int length = snprintf(dir, sizeof dir, "%s/smz-bind-io-XXXXXX", tmp);
    assert(length > 0 && (size_t)length < sizeof dir);
    assert(mkdtemp(dir) != NULL);
    file_io_set_game_root(dir);
    char root[1024];
    snprintf(root, sizeof root, "%s", file_io_get_game_root());

    JSEngine *engine = js_engine_init();
    assert(engine);
    assert(bind_io_define(js_engine_get_jsb(engine)));
    define_helpers(engine);
    check_shape(engine);
    check_results(engine, root);
    check_arity_errors(engine);
    check_errors(engine);
    check_write_guard(engine);
    js_engine_shutdown(engine);
    cleanup(root);
    printf("bind_io_jsb: all checks passed\n");
    return 0;
}
