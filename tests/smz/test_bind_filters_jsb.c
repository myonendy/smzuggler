/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Pins the __native_filters binding to its pre-port JS API without a device.
 * See docs/js-binding.md: handles are call-scoped; no buffer bytes are kept.
 * Runs on the selected engine: the adopted QuickJS runtime or V8.
 */
#include "bindings/bind_filters.h"
#include "engine/js_engine.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Compare a script result with the pre-port capture; free the owned string.
 * Abort with the script and both values on a mismatch or an exception. */
static void expect(JSEngine *engine, const char *script, const char *expected)
{
    char *got = js_engine_eval_string(engine, script, "bind_filters_jsb");
    if (!got || strcmp(got, expected)) {
        fprintf(stderr, "%s\nexpected: %s\ngot: %s\n", script, expected,
                got ? got : "(exception)");
        assert(0);
    }
    js_engine_free_string(got);
}

/* Pin upstream's own property names, function arities and attributes. */
static void check_shape(JSEngine *engine)
{
    expect(engine, "var m=__native_filters;var names=Object.getOwnPropertyNames(m);names.join()",
           "init,shutdown,compileShader,deleteShader,beginFilter,setUniform1f,"
           "setUniform2f,setUniformTexture,setUniform4f,setUniformMat4,drawQuad,"
           "endFilter,colorMatrixShader,maskShader,blurShader,alphaShader,"
           "colorFilterShader");
    expect(engine, "names.slice(0,12).map(function(n){return m[n].length}).join()",
           "0,0,2,1,4,3,4,4,6,3,0,0");
    expect(engine, "names.slice(0,12).every(function(n){"
                   "var d=Object.getOwnPropertyDescriptor(m,n);"
                   "return typeof m[n]==='function'&&m[n].name===n&&d.writable&&"
                   "!d.enumerable&&d.configurable})", "true");
    expect(engine, "names.slice(0,12).every(function(n){"
                   "return Object.getOwnPropertyNames(m[n]).join()==='length,name'})",
           "true");
    expect(engine, "Object.keys(m).join()",
           "colorMatrixShader,maskShader,blurShader,alphaShader,colorFilterShader");
    expect(engine, "names.slice(12).every(function(n){"
                   "var d=Object.getOwnPropertyDescriptor(m,n);"
                   "return m[n]===0&&d.writable&&d.enumerable&&d.configurable})",
           "true");
}

/* No graphics device is needed for the no-op results upstream returns. */
static void check_results(JSEngine *engine)
{
    expect(engine, "m.init()", "false");
    expect(engine, "names.slice(12).every(function(n){return m[n]===0})", "true");
    expect(engine, "m.shutdown()===undefined", "true");
    expect(engine, "m.compileShader(null,'void main(){}')", "0");
    expect(engine, "m.compileShader(undefined,'void main(){}')", "0");
    expect(engine, "m.compileShader({},{})", "0");
    expect(engine, "m.deleteShader(7)===undefined&&m.deleteShader()===undefined", "true");
    expect(engine, "m.beginFilter(0,0,16,16)===undefined", "true");
    expect(engine, "m.setUniform1f(0,'u',1.5)===undefined", "true");
    expect(engine, "m.setUniform2f(0,'u',1.5,2.5)===undefined", "true");
    expect(engine, "m.setUniformTexture(0,'u',0,1)===undefined", "true");
    expect(engine, "m.setUniform4f(0,'u',1,2,3,4)===undefined", "true");
    expect(engine, "m.setUniformMat4(0,'u',new Float32Array(16))===undefined", "true");
    expect(engine, "m.setUniformMat4(0,'u',new Float32Array(new ArrayBuffer(72),8,16))===undefined", "true");
    expect(engine, "m.setUniformMat4(0,'u',new ArrayBuffer(64))===undefined", "true");
    expect(engine, "m.setUniformMat4(0,'u',[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16])===undefined", "true");
    expect(engine, "m.setUniformMat4(0,'u',null)===undefined&&"
                   "m.setUniformMat4(0,'u',{})===undefined&&"
                   "m.setUniformMat4(0,'u',new DataView(new ArrayBuffer(64)))===undefined", "true");
    expect(engine, "m.drawQuad()===undefined&&m.endFilter()===undefined", "true");
}

/* Pin binding-owned messages and the coercion failures upstream propagated. */
static void check_errors(JSEngine *engine)
{
    expect(engine, "try{m.compileShader()}catch(e){e.name+': '+e.message}",
           "TypeError: compileShader requires vertSrc and fragSrc");
    expect(engine, "try{m.compileShader(null)}catch(e){e.name+': '+e.message}",
           "TypeError: compileShader requires vertSrc and fragSrc");
    expect(engine, "try{m.compileShader(null,Symbol())}catch(e){e.name+': '+e.message}",
           "TypeError: compileShader: fragSrc required");
    expect(engine, "try{m.beginFilter()}catch(e){e.name+': '+e.message}",
           "TypeError: beginFilter requires shaderId, inputTexture, w, h");
    expect(engine, "try{m.compileShader(null,undefined)}catch(e){e.name+': '+e.message}",
           "0");
    expect(engine, "try{m.compileShader(Symbol(),'void main(){}')}catch(e){e.name+': '+e.message}",
           "0");
    expect(engine, "try{m.beginFilter(0,0,1)}catch(e){e.name+': '+e.message}",
           "TypeError: beginFilter requires shaderId, inputTexture, w, h");
    expect(engine, "['deleteShader','setUniform1f','setUniform2f','setUniformTexture',"
                   "'setUniform4f','setUniformMat4'].every(function(n){"
                   "try{m[n](Symbol(),'u',0,0,0,0);return false}catch(e){"
                   "return e instanceof TypeError}})", "true");
    expect(engine, "['beginFilter','setUniformTexture'].every(function(n){"
                   "try{m[n](0,0,Symbol(),0,0,0);return false}catch(e){"
                   "return e instanceof TypeError}})", "true");
    expect(engine, "['setUniform1f','setUniform2f','setUniform4f','setUniformTexture',"
                   "'setUniformMat4'].every(function(n){"
                   "try{m[n](0,Symbol(),0,0,0,0,0);return false}catch(e){"
                   "return e instanceof TypeError}})", "true");
    expect(engine, "var bad={valueOf:function(){throw new RangeError('number')}};"
                   "try{m.setUniform1f(0,'u',bad)}catch(e){e.name+': '+e.message}",
           "RangeError: number");
    expect(engine, "var bad={toString:function(){throw new Error('name')}};"
                   "try{m.setUniform1f(0,bad,1)}catch(e){e.name+': '+e.message}",
           "Error: name");
    expect(engine, "var a=[1,2,3];Object.defineProperty(a,'0',{get:function(){"
                   "throw new Error('element')}});"
                   "try{m.setUniformMat4(0,'u',a)}catch(e){e.name+': '+e.message}",
           "Error: element");
}

/* Own the engine; no device, game assets or retained handles are needed.
 * `js_engine_get_jsb` is the adopted jsb runtime in the default engine mode and the
 * V8 backend in v8 mode, so the port itself runs on the selected engine. */
int main(void)
{
    JSEngine *engine = js_engine_init();
    assert(engine);
    assert(bind_filters_define(js_engine_get_jsb(engine)));
    check_shape(engine);
    check_results(engine);
    check_errors(engine);
    js_engine_shutdown(engine);
    puts("bind_filters_jsb passed");
    return 0;
}
