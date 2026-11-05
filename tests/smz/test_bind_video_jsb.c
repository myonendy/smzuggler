/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Pins the native HTML video shim bridge to the pre-port API without a device.
 * See docs/js-binding.md: engines own handles; tests free returned strings.
 * All calls run on the main thread and retain no JS handles.
 */
#include "bindings/bind_video.h"
#include "engine/js_engine.h"
#include "video/video_player.h"
#include "io/file_io.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Compare a script result with the pre-port capture; free the owned string.
 * Abort with context on a mismatch or an unexpected exception. */
static void expect(JSEngine *engine, const char *script, const char *expected)
{
    char *got = js_engine_eval_string(engine, script, "bind_video_jsb");
    if (!got || strcmp(got, expected)) {
        fprintf(stderr, "%s\nexpected: %s\ngot: %s\n", script, expected,
                got ? got : "(exception)");
        assert(0);
    }
    js_engine_free_string(got);
}

/* Pin every method and descriptor against the original native module. */
static void check_shape(JSEngine *engine)
{
    expect(engine, "var m=__native_video;var names=Object.getOwnPropertyNames(m);names.join()",
           "open,close,play,pause,setVolume,setLoop,setVisible,getState");
    expect(engine, "names.map(function(n){return m[n].length}).join()", "1,1,1,1,2,2,2,1");
    expect(engine, "names.every(function(n){var d=Object.getOwnPropertyDescriptor(m,n);"
                   "return typeof m[n]==='function'&&m[n].name===n&&d.writable&&!d.enumerable&&d.configurable})", "true");
    expect(engine, "Object.keys(m).join()", "");
    expect(engine, "names.every(function(n){return Object.getOwnPropertyNames(m[n]).join()==='length,name'})", "true");
}

/* No device or media is needed for invalid handles and failed opens. */
static void check_results(JSEngine *engine)
{
    expect(engine, "[m.open('missing.mpg'),m.open(null),m.open(undefined)].every(function(v){return v===0})", "true");
    expect(engine, "['close','play','pause','setVolume','setLoop','setVisible'].every(function(n){"
                   "return m[n]('0','0')===undefined&&m[n](null,{})===undefined})", "true");
    expect(engine, "[m.getState(0),m.getState('0'),m.getState(null),m.getState(-1)].every(function(v){return v===null})", "true");
    expect(engine, "var h=m.open('tests/fixtures/test_video.mpg');h>0", "true");
    expect(engine, "var st=m.getState(h);Object.getOwnPropertyNames(st).join()",
           "playing,paused,ended,time,duration,width,height");
    expect(engine, "JSON.stringify([st.playing,st.paused,st.ended,st.time,typeof st.duration,st.width,st.height])",
           "[false,false,false,0,\"number\",64,48]");
    expect(engine, "st.duration>0.8&&st.duration<1.2", "true");
    expect(engine, "m.play(h)===undefined&&m.getState(h).playing", "true");
    expect(engine, "m.pause(h)===undefined&&m.getState(h).paused&&!m.getState(h).playing", "true");
    expect(engine, "m.setVolume(h,'0.5')===undefined&&m.setLoop(h,{})===undefined&&"
                   "m.setVisible(h,0)===undefined", "true");
    expect(engine, "m.close(h)===undefined&&m.getState(h)===null", "true");
}

/* Preserve explicit binding errors, missing-argument throws and coercions. */
static void check_errors(JSEngine *engine)
{
    expect(engine, "try{m.open()}catch(e){e.name+': '+e.message}", "TypeError: open requires a path");
    expect(engine, "names.slice(1).every(function(n){try{m[n]()}catch(e){return typeof e==='unknown'&&String(e)==='[uninitialized]'}})", "true");
    expect(engine, "['setVolume','setLoop','setVisible'].every(function(n){try{m[n](0)}catch(e){return typeof e==='unknown'&&String(e)==='[uninitialized]'}})", "true");
    expect(engine, "var bad={valueOf:function(){throw new RangeError('number')},"
                   "toString:function(){throw new Error('path')}};"
                   "try{m.open(bad)}catch(e){e.name+': '+e.message}", "Error: path");
    expect(engine, "names.slice(1).every(function(n){try{m[n](bad,1)}"
                   "catch(e){return e.name==='RangeError'&&e.message==='number'}})", "true");
    expect(engine, "try{m.setVolume(0,bad)}catch(e){e.name+': '+e.message}", "RangeError: number");
    expect(engine, "m.setLoop(0,bad)===undefined&&m.setVisible(0,bad)===undefined", "true");
    expect(engine, "names.every(function(n){try{m[n](Symbol(),1)}catch(e){return e instanceof TypeError}})", "true");
    expect(engine, "var order=[];m.setVolume({valueOf:function(){order.push('handle');return 0}},"
                   "{valueOf:function(){order.push('volume');return 1}});order.join()", "handle,volume");
}

/* Own the main-thread engine and headless player; retain no JS handles. */
int main(void)
{
    file_io_set_game_root(".");
    video_player_init(NULL);
    JSEngine *engine = js_engine_init();
    assert(engine);
    assert(bind_video_define(js_engine_get_jsb(engine)));
    check_shape(engine);
    check_results(engine);
    check_errors(engine);
    js_engine_shutdown(engine);
    video_player_shutdown();
    file_io_set_game_root(NULL);
    puts("bind_video_jsb passed");
    return 0;
}
