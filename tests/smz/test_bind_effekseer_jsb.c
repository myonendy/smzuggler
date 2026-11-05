/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Pins the Effekseer shim bridge to its pre-port API without a graphics device.
 * See docs/js-binding.md: main thread only; the engine owns all JS handles.
 */
#include "bindings/bind_effekseer.h"
#include "engine/js_engine.h"
#include "io/file_io.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Compare with a pre-port result; free the engine-owned result string. */
static void expect(JSEngine *engine, const char *script, const char *expected)
{
    char *got = js_engine_eval_string(engine, script, "bind_effekseer_jsb");
    if (!got || strcmp(got, expected)) {
        fprintf(stderr, "%s\nexpected: %s\ngot: %s\n", script, expected,
                got ? got : "(exception)");
        assert(0);
    }
    js_engine_free_string(got);
}

/* Pin every own name, function type, arity and descriptor in upstream order. */
static void check_shape(JSEngine *engine)
{
    expect(engine, "var m=__native_effekseer;var names=Object.getOwnPropertyNames(m);names.join()",
           "init,shutdown,isInitialized,load,release,isLoaded,play,stop,stopRoot,stopAll,setPosition,setRotation,setScale,setSpeed,exists,setProjectionMatrix,setCameraMatrix,update,beginDraw,drawHandle,endDraw,setRestorationOfStatesFlag,setBackground,resetBackground,getLoadedCount,getPlayingCount");
    expect(engine, "names.map(function(n){return m[n].length}).join()",
           "3,0,0,2,1,1,4,1,1,0,4,4,4,2,1,1,1,1,0,1,0,1,1,0,0,0");
    expect(engine, "names.every(function(n){var d=Object.getOwnPropertyDescriptor(m,n);"
                   "return typeof m[n]==='function'&&m[n].name===n&&d.writable&&!d.enumerable&&d.configurable})", "true");
}

/* All 26 functions have deterministic results with the tracking backend idle. */
static void check_results(JSEngine *engine)
{
    expect(engine, "[m.isInitialized(),m.isLoaded(),m.isLoaded('0'),m.exists(),m.exists('0')].every(function(v){return v===false})", "true");
    expect(engine, "[m.load('../outside.efkefc'),m.load('missing.efkefc'),m.getLoadedCount(),m.getPlayingCount()].every(function(v){return v===0})", "true");
    expect(engine, "m.play()===-1&&m.play('0',null,'0',false)===-1", "true");
    expect(engine, "['release','stop','stopRoot','stopAll','setPosition','setRotation','setScale','setSpeed',"
                   "'update','beginDraw','drawHandle','endDraw','setRestorationOfStatesFlag','setBackground',"
                   "'resetBackground','shutdown'].every(function(n){return m[n]()===undefined&&m[n]('0','0','0','0')===undefined})", "true");
    expect(engine, "['setProjectionMatrix','setCameraMatrix'].every(function(n){"
                   "var reads=[];var a=Array.from({length:16},function(_,i){return {valueOf:function(){reads.push(i);return String(i)}}});"
                   "return m[n](a)===undefined&&reads.join()==='0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15'&&m[n]([])===undefined})", "true");
    expect(engine, "m.init('816','624','8000')===true&&m.isInitialized()===true&&m.shutdown()===undefined&&m.isInitialized()===false", "true");
}

/* Preserve binding-owned messages and exceptions raised by user coercion. */
static void check_errors(JSEngine *engine)
{
    expect(engine, "try{m.load()}catch(e){e.name+': '+e.message}", "TypeError: load requires a path argument");
    expect(engine, "['setProjectionMatrix','setCameraMatrix'].every(function(n){return [undefined,null,{},new Float32Array(16)].every(function(a){"
                   "try{m[n](a)}catch(e){return e instanceof TypeError&&e.message===n+' expects a 16-element array'}})})", "true");
    expect(engine, "var bad={valueOf:function(){throw new RangeError('number')},toString:function(){throw new Error('path')}};"
                   "m.load(bad)===0&&m.load(Symbol())===0", "true");
    expect(engine, "['init','release','isLoaded','play','stop','stopRoot','setPosition','setRotation','setScale','setSpeed','exists','update','drawHandle','setBackground'].every(function(n){"
                   "try{m[n](bad,0,0,0)}catch(e){return e instanceof RangeError&&e.message==='number'}})", "true");
    expect(engine, "['setProjectionMatrix','setCameraMatrix'].every(function(n){try{m[n]([bad])}catch(e){return e instanceof RangeError&&e.message==='number'}})", "true");
    expect(engine, "['init','play','setPosition','setRotation','setScale','setSpeed'].every(function(n){"
                   "try{m[n](0,bad,0,0)}catch(e){return e instanceof RangeError&&e.message==='number'}})", "true");
    expect(engine, "try{m.load('missing.efkefc',bad)}catch(e){e.name+': '+e.message}", "RangeError: number");
    expect(engine, "['setProjectionMatrix','setCameraMatrix'].every(function(n){var a=[];Object.defineProperty(a,'0',{get:function(){throw new Error('element')}});"
                   "try{m[n](a)}catch(e){return e.name==='Error'&&e.message==='element'}})", "true");
}

/* Own the engine and synthetic root; no game assets or device are needed. */
int main(void)
{
    file_io_set_game_root("effekseer-jsb-fixture");
    JSEngine *engine = js_engine_init();
    assert(engine);
    bind_effekseer_register(js_engine_get_context(engine));
    check_shape(engine);
    check_results(engine);
    check_errors(engine);
    js_engine_shutdown(engine);
    file_io_set_game_root(NULL);
    puts("bind_effekseer_jsb passed");
    return 0;
}
