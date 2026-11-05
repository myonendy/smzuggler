/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Pins the native Web Audio shim bridge to the pre-port API without a device.
 * See docs/js-binding.md: engines own handles; tests free returned strings.
 * All calls run on the main thread and retain no JS handles.
 */
#include "bindings/bind_audio.h"
#include "engine/js_engine.h"
#include "audio/audio_engine.h"
#include "io/file_io.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Compare a script result with the pre-port capture; free the owned string.
 * Abort with context on a mismatch or an unexpected exception. */
static void expect(JSEngine *engine, const char *script, const char *expected)
{
    char *got = js_engine_eval_string(engine, script, "bind_audio_jsb");
    if (!got || strcmp(got, expected)) {
        fprintf(stderr, "%s\nexpected: %s\ngot: %s\n", script, expected,
                got ? got : "(exception)");
        assert(0);
    }
    js_engine_free_string(got);
}

/* Pin own names, all callable arities, types and property descriptors. */
static void check_shape(JSEngine *engine)
{
    expect(engine, "Object.getOwnPropertyNames(__native_audio).join()",
           "init,shutdown,isInitialized,load,loadStream,loadFromMemory,freeSource,play,stop,stopBus,stopAll,pause,resume,pauseBus,resumeBus,setVolume,setPitch,setPan,setBusVolume,fadeVolume,fadeBusVolume,isPlaying,isPaused,getVolume,getPosition,seek,getDuration,getActiveVoiceCount,BUS_BGM,BUS_BGS,BUS_ME,BUS_SE");
    expect(engine, "var m=__native_audio;var names=Object.getOwnPropertyNames(m).slice(0,28);"
                   "names.map(function(n){return m[n].length}).join()",
           "0,0,0,1,1,1,1,6,1,1,0,1,1,1,1,2,2,2,2,3,3,1,1,1,1,2,1,0");
    expect(engine, "names.every(function(n){var d=Object.getOwnPropertyDescriptor(m,n);"
                   "return typeof m[n]==='function'&&m[n].name===n&&d.writable&&!d.enumerable&&d.configurable})",
           "true");
    expect(engine, "Object.keys(m).join()", "BUS_BGM,BUS_BGS,BUS_ME,BUS_SE");
    expect(engine, "[m.BUS_BGM,m.BUS_BGS,m.BUS_ME,m.BUS_SE].every(function(v,i){return v===i})", "true");
    expect(engine, "names.every(function(n){return Object.getOwnPropertyNames(m[n]).join()==='length,name'})", "true");
}

/* Every operation except real device init has a deterministic idle result.
 * Disabled init exercises that entry point without opening an audio device. */
static void check_results(JSEngine *engine)
{
    expect(engine, "m.isInitialized()===false", "true");
    expect(engine, "[m.load('missing.ogg'),m.loadStream('missing.ogg'),"
                   "m.loadFromMemory(new ArrayBuffer(4)),m.loadFromMemory(new ArrayBuffer(0)),"
                   "m.play('0','0','1','1','0',{}),m.getVolume('0'),m.getPosition(null),"
                   "m.getDuration(undefined),m.getActiveVoiceCount()].every(function(v){return v===0})",
           "true");
    expect(engine, "[m.isPlaying('0'),m.isPaused(null),m.isPlaying(),m.isPaused()].every(function(v){return v===false})", "true");
    expect(engine, "[m.getVolume(),m.getPosition(),m.getDuration()].every(function(v){return v===0})", "true");
    expect(engine, "['freeSource','stop','stopBus','stopAll','pause','resume','pauseBus',"
                   "'resumeBus','setVolume','setPitch','setPan','setBusVolume','fadeVolume',"
                   "'fadeBusVolume','seek','shutdown'].every(function(n){"
                   "return m[n]()===undefined&&m[n]('0','0','0')===undefined})",
           "true");
    audio_engine_set_disabled(true);
    expect(engine, "m.init()===true", "true");
    expect(engine, "m.isInitialized()===false", "true");
    audio_engine_set_disabled(false);
}

/* Binding errors are byte-for-byte; conversion exceptions retain identity.
 * Native Symbol conversion wording is engine-specific, so check its type. */
static void check_errors(JSEngine *engine)
{
    expect(engine, "['load','loadStream','loadFromMemory','play'].map(function(n){"
                   "try{m[n]()}catch(e){return e.name+': '+e.message}}).join('|')",
           "TypeError: load requires a path argument|TypeError: loadStream requires a path argument|"
           "TypeError: loadFromMemory requires an ArrayBuffer|TypeError: play requires 6 arguments");
    expect(engine, "[null,{},'x',new Uint8Array(4),new DataView(new ArrayBuffer(4)),"
                   "{[Symbol.toStringTag]:'ArrayBuffer'}].every(function(b){try{m.loadFromMemory(b)}"
                   "catch(e){return e instanceof TypeError&&e.message==='argument must be an ArrayBuffer'}})",
           "true");
    expect(engine, "['play','stopBus','pauseBus','resumeBus','setBusVolume','fadeBusVolume'].every(function(n){"
                   "return [-1,4].every(function(b){try{if(n==='play')m[n](0,b,1,1,0,false);"
                   "else m[n](b,1,1)}catch(e){return e instanceof RangeError&&e.message==='invalid audio bus: '+b}})})",
           "true");
    expect(engine, "['load','loadStream'].map(function(n){try{m[n]('../outside.ogg')}"
                   "catch(e){return e.name+': '+e.message}}).join('|')",
           "TypeError: audio path resolution failed: ../outside.ogg|TypeError: audio path resolution failed: ../outside.ogg");
    expect(engine, "var bad={valueOf:function(){throw new RangeError('number')},"
                   "toString:function(){throw new Error('path')}};"
                   "['load','loadStream'].every(function(n){try{m[n](bad)}"
                   "catch(e){return e.name==='Error'&&e.message==='path'}})",
           "true");
    expect(engine, "names.filter(function(n){return !['init','shutdown','isInitialized',"
                   "'load','loadStream','loadFromMemory','stopAll','getActiveVoiceCount'].includes(n)})"
                   ".every(function(n){try{m[n](bad,0,1,1,0,false)}"
                   "catch(e){return e.name==='RangeError'&&e.message==='number'}})",
           "true");
    expect(engine, "['setVolume','setPitch','setPan','setBusVolume','fadeVolume',"
                   "'fadeBusVolume','seek'].every(function(n){try{m[n](0,bad,1)}"
                   "catch(e){return e.name==='RangeError'&&e.message==='number'}})",
           "true");
    expect(engine, "['load','loadStream','stop'].every(function(n){try{m[n](Symbol())}"
                   "catch(e){return e instanceof TypeError}})",
           "true");
}

/* Own the selected engine; exercise the module with no device or game data. */
int main(void)
{
    file_io_set_game_root("audio-jsb-fixture");
    JSEngine *engine = js_engine_init();
    assert(engine);
    assert(bind_audio_define(js_engine_get_jsb(engine)));
    check_shape(engine);
    check_results(engine);
    check_errors(engine);
    js_engine_shutdown(engine);
    file_io_set_game_root(NULL);
    puts("bind_audio_jsb passed");
    return 0;
}
