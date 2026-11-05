/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Pins __native_font to the pre-port binary's surface and device-free results.
 * Unlike --dump-surface this checks native names, arities and argument errors.
 * Runs on the selected engine's context; see docs/js-binding.md for lifetimes.
 * All strings returned by the engine are freed here on the main thread.
 */
#include "bindings/bind_font.h"
#include "engine/js_engine.h"
#include "rendering/font_manager.h"
#include "rendering/canvas2d.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Compare a JS result with the captured baseline, naming failures; main thread.
 * The engine owns its context; this helper frees the returned UTF-8 string. */
static void expect(JSEngine *engine, const char *script, const char *expected)
{
    char *got = js_engine_eval_string(engine, script, "bind_font_jsb");
    if (!got || strcmp(got, expected)) {
        fprintf(stderr, "%s\nexpected: %s\ngot: %s\n", script, expected,
                got ? got : "(exception)");
        assert(0);
    }
    js_engine_free_string(got);
}

/* Preserve names, order, descriptors and all seven callable arities. */
static void check_shape(JSEngine *engine)
{
    expect(engine, "typeof __native_font", "object");
    expect(engine, "Object.getOwnPropertyNames(__native_font).join()",
           "loadFont,loadFontFile,hasFont,getMetrics,measureText,fontCount,fontNames");
    expect(engine, "Object.keys(__native_font).length", "0");
    expect(engine, "Object.getOwnPropertyNames(__native_font.loadFont).join()", "length,name");
    expect(engine,
           "Object.getOwnPropertyNames(__native_font).map(function(n){"
           "var f=__native_font[n],d=Object.getOwnPropertyDescriptor(__native_font,n);"
           "return [typeof f,f.length,f.name,d.writable,d.enumerable,d.configurable].join(':')"
           "}).join()",
           "function:2:loadFont:true:false:true,function:2:loadFontFile:true:false:true,"
           "function:1:hasFont:true:false:true,function:2:getMetrics:true:false:true,"
           "function:3:measureText:true:false:true,function:0:fontCount:true:false:true,"
           "function:0:fontNames:true:false:true");
}

/* Exercise each device-free result and the original string/number coercions. */
static void check_results(JSEngine *engine)
{
    expect(engine, "__native_font.loadFont('missing',new ArrayBuffer(4))", "false");
    expect(engine, "__native_font.loadFontFile('missing','missing-font.ttf')", "false");
    expect(engine, "__native_font.hasFont({toString:function(){return 'missing'}})", "false");
    expect(engine, "__native_font.hasFont(undefined)", "false");
    expect(engine, "__native_font.hasFont('x'.repeat(8192))", "false");
    expect(engine, "__native_font.loadFont('missing',new ArrayBuffer(0))", "false");
    expect(engine, "__native_font.getMetrics('missing','20')===null", "true");
    expect(engine, "__native_font.measureText('missing','10',12345)", "30.000001907348633");
    expect(engine, "__native_font.fontCount('ignored')", "0");
    expect(engine, "JSON.stringify(__native_font.fontNames())", "[]");
    expect(engine, "Array.isArray(__native_font.fontNames())", "true");
}

/* Build an original, empty-glyph sfnt in JS: no game or system font needed.
 * A 1000-unit em and fixed metrics make successful result shapes reproducible. */
static void check_loaded(JSEngine *engine)
{
    expect(engine,
           "var font=new ArrayBuffer(608),v=new DataView(font);"
           "v.setUint32(0,65536);v.setUint16(4,7);"
           "[['cmap',160,274],['head',448,54],['hhea',512,36],['hmtx',560,4],"
           "['loca',568,4],['glyf',576,12],['maxp',592,6]].forEach(function(t,i){"
           "for(var j=0;j<4;j++)v.setUint8(12+16*i+j,t[0].charCodeAt(j));"
           "v.setUint32(20+16*i,t[1]);v.setUint32(24+16*i,t[2])});"
           "v.setUint16(162,1);v.setUint16(164,3);v.setUint16(166,1);"
           "v.setUint32(168,12);v.setUint16(174,262);"
           "v.setUint16(466,1000);v.setInt16(516,800);v.setInt16(518,-200);"
           "v.setInt16(520,100);v.setUint16(546,1);v.setUint16(560,500);"
           "v.setUint16(596,1);__native_font.loadFont('Synthetic',font)", "true");
    expect(engine, "__native_font.hasFont('Synthetic')", "true");
    expect(engine, "__native_font.hasFont('Synthetic\\0suffix')", "true");
    expect(engine, "__native_font.fontCount()", "1");
    expect(engine, "JSON.stringify(__native_font.fontNames())", "[\"Synthetic\"]");
    expect(engine, "Object.keys(__native_font.getMetrics('Synthetic',1000)).join()",
           "ascent,descent,lineGap,lineHeight");
    expect(engine, "JSON.stringify(__native_font.getMetrics('Synthetic','1000'))",
           "{\"ascent\":800,\"descent\":-200,\"lineGap\":100,\"lineHeight\":1100}");
    expect(engine, "__native_font.measureText('Synthetic',1000,'abc')", "1500");
}

/* Assert binding-authored errors byte for byte, plus conversion propagation.
 * Engine-authored Symbol error wording differs; user-thrown errors do not. */
static void check_errors(JSEngine *engine)
{
    expect(engine,
           "['loadFont','loadFontFile','hasFont','getMetrics','measureText'].map(function(n){"
           "try{__native_font[n]()}catch(e){return e.name+': '+e.message}}).join('|')",
           "TypeError: loadFont requires family and ArrayBuffer|"
           "TypeError: loadFontFile requires family and path|"
           "TypeError: hasFont requires family name|"
           "TypeError: getMetrics requires family and pixelSize|"
           "TypeError: measureText requires family, pixelSize, text");
    expect(engine,
           "[null,{[Symbol.toStringTag]:'ArrayBuffer'},'x',new Uint8Array(4),new DataView(new ArrayBuffer(4))].map(function(b){"
           "try{__native_font.loadFont('missing',b)}catch(e){return e.name+': '+e.message}}).join('|')",
           "TypeError: loadFont: second arg must be ArrayBuffer|"
           "TypeError: loadFont: second arg must be ArrayBuffer|"
           "TypeError: loadFont: second arg must be ArrayBuffer|"
           "TypeError: loadFont: second arg must be ArrayBuffer|"
           "TypeError: loadFont: second arg must be ArrayBuffer");
    expect(engine,
           "var bad={toString:function(){throw new RangeError('family')}};"
           "try{__native_font.loadFont(bad,null)}catch(e){e.name+': '+e.message}",
           "RangeError: family");
    expect(engine,
           "try{__native_font.getMetrics('missing',{valueOf:function(){throw new Error('size')}})}"
           "catch(e){e.name+': '+e.message}", "Error: size");
    expect(engine,
           "try{__native_font.measureText('missing',10,bad)}catch(e){e.name+': '+e.message}",
           "RangeError: family");
    expect(engine,
           "try{__native_font.loadFontFile('missing',bad)}catch(e){e.name+': '+e.message}",
           "RangeError: family");
    expect(engine,
           "['loadFont','loadFontFile','hasFont','getMetrics','measureText'].map(function(n){"
           "try{__native_font[n](Symbol(),new ArrayBuffer(4),'text')}"
           "catch(e){return e instanceof TypeError}}).join()",
           "true,true,true,true,true");
}

/* Own an engine and font cache without a device; abort on any API regression. */
int main(void)
{
    font_manager_init();
    JSEngine *engine = js_engine_init();
    assert(engine);
    assert(bind_font_define(js_engine_get_jsb(engine)));
    check_shape(engine);
    check_results(engine);
    check_errors(engine);
    check_loaded(engine);
    js_engine_shutdown(engine);
    font_manager_shutdown();
    canvas2d_shutdown();
    puts("bind_font_jsb passed");
    return 0;
}
