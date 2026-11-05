/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Checks that __native_input, ported to jsb by card M1-T015, keeps its JS
 * API byte-for-byte: property names, each function's typeof and length,
 * result shapes and argument errors.
 *
 * Why: --dump-surface lists no native modules and no `length`
 * (DEC-M0-T009b), so this test is what shows the port changed nothing
 * (DEC-M1-T014-JSB-ADOPT). Every expected string below was printed by the
 * pre-port QuickJS binary (bind_input_register on js_engine.c). It calls
 * bind_input_define on js_engine_get_jsb(): the QuickJS engine in quickjs
 * mode, smz_js_engine_v8 in v8 mode, so the port really runs on V8 there.
 * Events and gamepads are fed through input_manager, so no device is needed.
 * Main thread only; see docs/js-binding.md for the handle rules.
 */
#include "bindings/bind_input.h"
#include "engine/js_engine.h"
#include "input/input_manager.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Evaluates `script` and asserts its String() result is `expected`.
 *
 * Why: one helper keeps each check a single line and prints both sides on
 * a mismatch, so a failing check names what changed. Aborts on failure. */
static void expect(JSEngine *engine, const char *script, const char *expected)
{
    char *got = js_engine_eval_string(engine, script, "test_bind_input_jsb");
    if (!got || strcmp(got, expected) != 0) {
        fprintf(stderr, "FAIL: %s\n  expected: %s\n  got:      %s\n", script,
                expected, got ? got : "(exception)");
        assert(0);
    }
    js_engine_free_string(got);
}

/* Checks the module's own properties and each function's typeof/length. */
static void check_shape(JSEngine *engine)
{
    expect(engine, "typeof __native_input", "object");
    expect(engine, "Object.getOwnPropertyNames(__native_input).join()",
           "pollEvents,getGamepad,getGamepadCount");
    /* JS_CFUNC_DEF made the functions non-enumerable but writable and
     * configurable. */
    expect(engine, "Object.keys(__native_input).length", "0");
    expect(engine,
           "var d=Object.getOwnPropertyDescriptor(__native_input,'getGamepad');"
           "[d.writable,d.enumerable,d.configurable].join()",
           "true,false,true");
    expect(engine,
           "['pollEvents','getGamepad','getGamepadCount'].map(function(n){"
           "var f=__native_input[n];return typeof f+':'+f.length+':'+f.name})"
           ".join()",
           "function:0:pollEvents,function:1:getGamepad,"
           "function:0:getGamepadCount");
}

/* Pushes one event of each kind with distinctive field values. */
static void push_events(void)
{
    InputEvent ev;
    memset(&ev, 0, sizeof ev);
    ev.type = INPUT_KEY_DOWN;
    ev.keyCode = 65;
    strcpy(ev.key, "a");
    strcpy(ev.code, "KeyA");
    ev.repeat = true;
    ev.shiftKey = true;
    ev.metaKey = true;
    input_manager_push_event(&ev);

    memset(&ev, 0, sizeof ev);
    ev.type = INPUT_WHEEL;
    ev.mouseX = 10;
    ev.mouseY = -3;
    ev.button = 2;
    ev.wheelDeltaX = 1.5;
    ev.wheelDeltaY = -120;
    input_manager_push_event(&ev);

    memset(&ev, 0, sizeof ev);
    ev.type = INPUT_TEXT;
    strcpy(ev.key, "\xC3\xA9");
    input_manager_push_event(&ev);

    memset(&ev, 0, sizeof ev);
    ev.type = INPUT_MOUSE_UP;
    ev.mouseX = 7;
    ev.mouseY = 8;
    input_manager_push_event(&ev);
}

/* Checks pollEvents(): empty queue, then one event of each field group. */
static void check_poll_events(JSEngine *engine)
{
    expect(engine, "JSON.stringify(__native_input.pollEvents())", "[]");
    expect(engine, "Array.isArray(__native_input.pollEvents())", "true");
    push_events();
    expect(engine, "JSON.stringify(__native_input.pollEvents(1, 'x'))",
           "[{\"type\":\"keydown\",\"keyCode\":65,\"key\":\"a\","
           "\"code\":\"KeyA\",\"repeat\":true,\"shiftKey\":true,"
           "\"ctrlKey\":false,\"altKey\":false,\"metaKey\":true},"
           "{\"type\":\"wheel\",\"clientX\":10,\"clientY\":-3,\"pageX\":10,"
           "\"pageY\":-3,\"screenX\":10,\"screenY\":-3,\"button\":2,"
           "\"deltaX\":1.5,\"deltaY\":-120,\"deltaZ\":0,\"deltaMode\":0},"
           "{\"type\":\"textinput\",\"text\":\"\xC3\xA9\"},"
           "{\"type\":\"mouseup\",\"clientX\":7,\"clientY\":8,\"pageX\":7,"
           "\"pageY\":8,\"screenX\":7,\"screenY\":8,\"button\":0}]");
    expect(engine, "__native_input.pollEvents().length", "0");
}

/* Connects gamepad slot 1 with distinctive button and axis values. */
static void connect_gamepad(void)
{
    GamepadState gp;
    memset(&gp, 0, sizeof gp);
    gp.connected = true;
    strcpy(gp.id, "Test Pad");
    gp.index = 1;
    gp.buttons[0].pressed = true;
    gp.buttons[0].value = 1.0;
    gp.buttons[16].value = 0.25;
    gp.axes[0] = -0.5;
    gp.axes[3] = 1.0;
    input_manager_set_gamepad(1, &gp);
}

/* Checks getGamepad() and getGamepadCount() results and coercion. */
static void check_gamepads(JSEngine *engine)
{
    expect(engine, "String(__native_input.getGamepad())", "null");
    expect(engine, "String(__native_input.getGamepad(0))", "null");
    expect(engine, "String(__native_input.getGamepad(-1))", "null");
    expect(engine, "String(__native_input.getGamepad(99))", "null");
    expect(engine, "__native_input.getGamepadCount('ignored')", "4");
    connect_gamepad();
    expect(engine, "Object.keys(__native_input.getGamepad(1)).join()",
           "id,index,connected,mapping,timestamp,buttons,axes");
    expect(engine,
           "var g=__native_input.getGamepad(1);JSON.stringify([g.id,g.index,"
           "g.connected,g.mapping,g.timestamp,g.buttons.length,g.buttons[0],"
           "g.buttons[16],g.axes])",
           "[\"Test Pad\",1,true,\"standard\",0,17,"
           "{\"pressed\":true,\"value\":1,\"touched\":true},"
           "{\"pressed\":false,\"value\":0.25,\"touched\":false},"
           "[-0.5,0,0,1]]");
    /* ToInt32 coercion, as JS_ToInt32 did. */
    expect(engine, "__native_input.getGamepad('1').id", "Test Pad");
    expect(engine, "__native_input.getGamepad(1.9).id", "Test Pad");
    expect(engine, "__native_input.getGamepad({valueOf:function(){return 1}})"
           ".index", "1");
    expect(engine, "__native_input.getGamepad(4294967297).id", "Test Pad");
    expect(engine, "String(__native_input.getGamepad(undefined))", "null");
    input_manager_clear_gamepad(1);
    expect(engine, "String(__native_input.getGamepad(1))", "null");
}

/* Checks the errors getGamepad raises for arguments ToInt32 rejects. The
 * Symbol message is the engine's own conversion error, so it differs. */
static void check_errors(JSEngine *engine)
{
    expect(engine,
           "try{__native_input.getGamepad({valueOf:function(){"
           "throw new RangeError('from valueOf')}})}"
           "catch(e){e.name+': '+e.message}",
           "RangeError: from valueOf");
#ifdef SMZ_JS_ENGINE_V8
    const char *symbol_error = "TypeError: Cannot convert a Symbol value to a number";
#else
    const char *symbol_error = "TypeError: cannot convert symbol to number";
#endif
    expect(engine,
           "try{__native_input.getGamepad(Symbol())}"
           "catch(e){e.name+': '+e.message}",
           symbol_error);
}

int main(void)
{
    input_manager_init();
    JSEngine *engine = js_engine_init();
    assert(engine);
    assert(bind_input_define(js_engine_get_jsb(engine)));
    check_shape(engine);
    check_poll_events(engine);
    check_gamepads(engine);
    check_errors(engine);
    js_engine_shutdown(engine);
    input_manager_shutdown();
    printf("bind_input_jsb: all checks passed\n");
    return 0;
}
