/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Pins __native_canvas2d's JS API (names, lengths, results, errors) to the
 * pre-port binary's, through bind_canvas2d_define on the selected engine's
 * jsb runtime (DEC-M1-T014-JSB-ADOPT); --dump-surface cannot show it.
 * Main thread only; engine strings are freed here (docs/js-binding.md).
 */
#include "bindings/bind_canvas2d.h"
#include "engine/js_engine.h"
#include "rendering/canvas2d.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Compare baseline output on the main thread; frees the engine-owned copy. */
static void expect(JSEngine *engine, const char *script, const char *expected)
{
    char *got = js_engine_eval_string(engine, script, "bind_canvas2d_jsb");
    if (!got || strcmp(got, expected)) {
        fprintf(stderr, "expected: %s\ngot: %s\nscript: %s\n", expected,
                got ? got : "(exception)", script);
        assert(0);
    }
    js_engine_free_string(got);
}

/* Pin all 27 names, arities and property descriptors. */
static void check_shape(JSEngine *engine)
{
    expect(engine, "typeof __native_canvas2d",
           "object");
    expect(engine, "Object.getOwnPropertyNames(__native_canvas2d).join()",
           "create,destroy,resize,getSize,setFillColor,setStrokeColor,setGlobalAlpha,setCompositeOp,setLineWidth,setTextAlign,setTextBaseline,save,restore,fillRect,clearRect,strokeRect,drawImage,drawImageHandle,getImageData,putImageData,getPixels,loadFont,loadFontFile,setFont,fillText,strokeText,measureText");
    expect(engine, "Object.getOwnPropertyNames(__native_canvas2d).map(function(n){var f=__native_canvas2d[n],d=Object.getOwnPropertyDescriptor(__native_canvas2d,n);return [typeof f,f.length,f.name,d.writable,d.enumerable,d.configurable].join(':')}).join()",
           "function:2:create:true:false:true,function:1:destroy:true:false:true,function:3:resize:true:false:true,function:1:getSize:true:false:true,function:5:setFillColor:true:false:true,function:5:setStrokeColor:true:false:true,function:2:setGlobalAlpha:true:false:true,function:2:setCompositeOp:true:false:true,function:2:setLineWidth:true:false:true,function:2:setTextAlign:true:false:true,function:2:setTextBaseline:true:false:true,function:1:save:true:false:true,function:1:restore:true:false:true,function:5:fillRect:true:false:true,function:5:clearRect:true:false:true,function:5:strokeRect:true:false:true,function:12:drawImage:true:false:true,function:10:drawImageHandle:true:false:true,function:5:getImageData:true:false:true,function:6:putImageData:true:false:true,function:1:getPixels:true:false:true,function:2:loadFont:true:false:true,function:2:loadFontFile:true:false:true,function:3:setFont:true:false:true,function:4:fillText:true:false:true,function:4:strokeText:true:false:true,function:2:measureText:true:false:true");
    expect(engine, "Object.getOwnPropertyNames(__native_canvas2d.create).join()",
           "length,name");
}

/* Pin exact binding-authored missing-argument errors. */
static void check_missing(JSEngine *engine)
{
    expect(engine, "Object.getOwnPropertyNames(__native_canvas2d).map(function(n){try{__native_canvas2d[n]()}catch(e){return e.name+': '+e.message}}).join('|')",
           "TypeError: create requires width and height|TypeError: destroy requires a handle|TypeError: resize requires handle, width, height|TypeError: getSize requires a handle|TypeError: setFillColor requires handle,r,g,b,a|TypeError: setStrokeColor requires handle,r,g,b,a|TypeError: setGlobalAlpha requires handle, alpha|TypeError: setCompositeOp requires handle, op|TypeError: setLineWidth requires handle, width|TypeError: setTextAlign requires handle, align|TypeError: setTextBaseline requires handle, baseline|TypeError: save requires a handle|TypeError: restore requires a handle|TypeError: fillRect requires handle,x,y,w,h|TypeError: clearRect requires handle,x,y,w,h|TypeError: strokeRect requires handle,x,y,w,h|TypeError: drawImage requires 12 arguments|TypeError: drawImageHandle requires 10 arguments|TypeError: getImageData requires handle,x,y,w,h|TypeError: putImageData requires handle,data,x,y,w,h|TypeError: getPixels requires a handle|TypeError: loadFont requires name and ArrayBuffer|TypeError: loadFontFile requires name and path|TypeError: setFont requires handle, name, pixelSize|TypeError: fillText requires handle,text,x,y|TypeError: strokeText requires handle,text,x,y|TypeError: measureText requires handle and text");
}

/* Check device-free handles, result object order, copied pixels and fonts. */
static void check_results(JSEngine *engine)
{
    expect(engine, "var c=__native_canvas2d,h=c.create('2','3');typeof h",
           "number");
    expect(engine, "JSON.stringify(c.getSize(h))",
           "{\"width\":2,\"height\":3}");
    expect(engine, "c.getPixels(h) instanceof ArrayBuffer",
           "true");
    expect(engine, "c.getPixels(h).byteLength",
           "24");
    expect(engine, "c.getImageData(h,0,0,1,1) instanceof ArrayBuffer",
           "true");
    expect(engine, "Array.from(new Uint8Array(c.getImageData(0,0,0,1,1))).join()",
           "0,0,0,0");
    expect(engine, "c.getImageData(h,0,0,0,1)===null",
           "true");
    expect(engine, "c.getSize(0)===null && c.getPixels(0)===null",
           "true");
    expect(engine, "c.loadFont('missing',new ArrayBuffer(4))",
           "false");
    expect(engine, "c.loadFontFile('missing','missing-font.ttf')",
           "false");
    expect(engine, "c.measureText(0,'abc')",
           "0");
}

/* Every remaining native operation returns undefined without a device. */
static void check_void(JSEngine *engine)
{
    expect(engine, "[typeof c.destroy(0),typeof c.resize(h,'2','3'),typeof c.setFont(h,'missing',12),typeof c.fillText(0,'abc',0,0),typeof c.strokeText(0,'abc',0,0),typeof c.drawImage(0,new ArrayBuffer(4),1,1,0,0,1,1,0,0,1,1),typeof c.drawImageHandle(0,0,0,0,1,1,0,0,1,1),typeof c.putImageData(h,new Uint8Array([1,2,3,4]).buffer,0,0,1,1),typeof c.setFillColor(0,0,0,0,0),typeof c.setStrokeColor(0,0,0,0,0),typeof c.setGlobalAlpha(0,0),typeof c.setCompositeOp(0,0),typeof c.setLineWidth(0,0),typeof c.setTextAlign(0,0),typeof c.setTextBaseline(0,0),typeof c.save(0),typeof c.restore(0),typeof c.fillRect(0,0,0,0,0),typeof c.clearRect(0,0,0,0,0),typeof c.strokeRect(0,0,0,0,0)].join()",
           "undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined,undefined");
    expect(engine, "Array.from(new Uint8Array(c.getImageData(h,0,0,1,1))).join()",
           "1,2,3,4");
}

/* Keep dimension limits and mismatch errors byte for byte. */
static void check_ranges(JSEngine *engine)
{
    expect(engine, "try{c.create(0,1)}catch(e){e.name+': '+e.message}",
           "RangeError: create: dimensions out of range 0x1");
    expect(engine, "try{c.resize(h,1,0)}catch(e){e.name+': '+e.message}",
           "RangeError: resize: dimensions out of range 1x0");
    expect(engine, "try{c.getImageData(h,0,0,20000,1)}catch(e){e.name+': '+e.message}",
           "RangeError: getImageData: dimensions too large 20000x1");
    expect(engine, "try{c.getImageData(h,0,0,8192,8192)}catch(e){e.name+': '+e.message}",
           "RangeError: getImageData: total size too large (268435456 bytes)");
    expect(engine, "try{c.drawImage(h,new ArrayBuffer(4),2,2,0,0,1,1,0,0,1,1)}catch(e){e.name+': '+e.message}",
           "RangeError: drawImage: buffer size mismatch for 2x2");
    expect(engine, "try{c.putImageData(h,new ArrayBuffer(4),0,0,2,2)}catch(e){e.name+': '+e.message}",
           "RangeError: putImageData: buffer size mismatch for 2x2");
}

/* Reject views and spoofed buffers; preserve conversion exception ordering. */
static void check_buffers(JSEngine *engine)
{
    expect(engine, "[null,{},new Uint8Array(4),new DataView(new ArrayBuffer(4))].map(function(b){try{c.loadFont('x',b)}catch(e){return e.name+': '+e.message}}).join('|')",
           "TypeError: loadFont: second arg must be ArrayBuffer|TypeError: loadFont: second arg must be ArrayBuffer|TypeError: loadFont: second arg must be ArrayBuffer|TypeError: loadFont: second arg must be ArrayBuffer");
    expect(engine, "[null,{},new Uint8Array(4)].map(function(b){try{c.drawImage(0,b,1,1,0,0,1,1,0,0,1,1)}catch(e){return e.name+': '+e.message}}).join('|')",
           "TypeError: drawImage: second arg must be ArrayBuffer|TypeError: drawImage: second arg must be ArrayBuffer|TypeError: drawImage: second arg must be ArrayBuffer");
    expect(engine, "[null,{},new Uint8Array(4)].map(function(b){try{c.putImageData(0,b,0,0,1,1)}catch(e){return e.name+': '+e.message}}).join('|')",
           "TypeError: putImageData: second arg must be ArrayBuffer|TypeError: putImageData: second arg must be ArrayBuffer|TypeError: putImageData: second arg must be ArrayBuffer");
    expect(engine, "var bad={valueOf:function(){throw new Error('coercion')}};try{c.drawImage(0,null,bad,1,0,0,1,1,0,0,1,1)}catch(e){e.name+': '+e.message}",
           "Error: coercion");
    expect(engine, "try{c.fillText(h,'x',bad,0)}catch(e){e.name+': '+e.message}",
           "Error: coercion");
    expect(engine, "try{c.setFont(h,'x',bad)}catch(e){e.name+': '+e.message}",
           "Error: coercion");
}

/* Each numeric-handle method propagates user coercion errors; no handles escape. */
static void check_conversions(JSEngine *engine)
{
    expect(engine,
           "Object.getOwnPropertyNames(c).filter(function(n){return n!=='loadFont'&&n!=='loadFontFile'})"
           ".map(function(n){var args=Array(c[n].length).fill(1);args[0]=bad;"
           "try{c[n].apply(c,args)}catch(e){return e.name+': '+e.message}}).join('|')",
           "Error: coercion|Error: coercion|Error: coercion|Error: coercion|Error: coercion|"
           "Error: coercion|Error: coercion|Error: coercion|Error: coercion|Error: coercion|"
           "Error: coercion|Error: coercion|Error: coercion|Error: coercion|Error: coercion|"
           "Error: coercion|Error: coercion|Error: coercion|Error: coercion|Error: coercion|"
           "Error: coercion|Error: coercion|Error: coercion|Error: coercion|Error: coercion");
    expect(engine,
           "var badString={toString:function(){throw new RangeError('string')}};"
           "['loadFont','loadFontFile'].map(function(n){try{c[n](badString,null)}"
           "catch(e){return e.name+': '+e.message}}).join('|')",
           "RangeError: string|RangeError: string");
    expect(engine,
           "try{c.loadFontFile('missing',badString)}catch(e){e.name+': '+e.message}",
           "RangeError: string");
    expect(engine,
           "try{c.setFont(h,badString,12)}catch(e){e.name+': '+e.message}",
           "RangeError: string");
    expect(engine,
           "try{c.measureText(h,badString)}catch(e){e.name+': '+e.message}",
           "RangeError: string");
    expect(engine, "typeof c.drawImageHandle(0,0,bad,0,1,1,0,0,1,1)", "undefined");
    expect(engine,
           "var calls=0;var once={valueOf:function(){calls++;return 1}};"
           "c.putImageData(0,new ArrayBuffer(4),0,0,once,1);calls", "1");
    expect(engine,
           "var copy=c.getPixels(h);new Uint8Array(copy)[0]=99;"
           "new Uint8Array(c.getPixels(h))[0]", "1");
}

/* Own the selected engine and CPU canvas state; no device or game is needed. */
int main(void)
{
    canvas2d_init();
    JSEngine *engine = js_engine_init();
    assert(engine);
    bool defined = bind_canvas2d_define(js_engine_get_jsb(engine));
    assert(defined);
    check_shape(engine);
    check_missing(engine);
    check_results(engine);
    check_void(engine);
    check_ranges(engine);
    check_buffers(engine);
    check_conversions(engine);
    js_engine_shutdown(engine);
    canvas2d_shutdown();
    puts("bind_canvas2d_jsb passed");
    return 0;
}
