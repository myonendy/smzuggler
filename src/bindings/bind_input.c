/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 * Copyright (C) 2026 The smzuggler contributors
 * Modified 2026-11-04: ported from QuickJS to the jsb seam (spec section 4.1).
 * Modified 2026-11-05: remove the transitional QuickJS wrapper.
 * Modified 2026-11-05: rely on jsb_set_fn_n for non-enumerable functions (card M1-T026b).
 *
 * __native_input: hands queued keyboard, mouse, wheel and text events and
 * the polled gamepad state to dom_shim.js, which turns them into DOM events
 * and navigator.getGamepads() results (docs/js-binding.md, "bind_input").
 *
 * smzuggler: the port keeps Outsider's structure line by line so upstream
 * fixes re-apply by hand (docs/UPSTREAM.md). Every jsb_val here is a
 * call-scoped handle, never kept past the native call that made it (spec
 * section 15.8, docs/js-binding.md "Handle and lifetime rules"). Main
 * thread only.
 */

#include "bindings/bind_input.h"
#include "input/input_manager.h"

#include <stdio.h>
#include <string.h>

/* Build a DOM-style JS event object from an InputEvent.
 *
 * Why: dom_shim.js dispatches these as KeyboardEvent, MouseEvent, WheelEvent
 * and a text input event; the field names follow UI Events
 * (https://w3c.github.io/uievents/). `ev` is borrowed. Returns a call-scoped
 * handle. Property-set failures are ignored, as upstream ignored them. */
static jsb_val *input_event_to_js(jsb_ctx *ctx, const InputEvent *ev)
{
    jsb_val *obj = jsb_new_object(ctx);

    /* Common: event type string */
    const char *type_str;
    switch (ev->type) {
        case INPUT_KEY_DOWN:   type_str = "keydown";     break;
        case INPUT_KEY_UP:     type_str = "keyup";       break;
        case INPUT_MOUSE_DOWN: type_str = "mousedown";   break;
        case INPUT_MOUSE_UP:   type_str = "mouseup";     break;
        case INPUT_MOUSE_MOVE: type_str = "mousemove";   break;
        case INPUT_WHEEL:      type_str = "wheel";       break;
        case INPUT_TEXT:       type_str = "textinput";   break;
        default:               type_str = "unknown";     break;
    }
    jsb_set_prop(ctx, obj, "type", jsb_new_str(ctx, type_str));

    if (ev->type == INPUT_TEXT) {
        jsb_set_prop(ctx, obj, "text", jsb_new_str(ctx, ev->key));
    }

    /* Keyboard fields */
    if (ev->type == INPUT_KEY_DOWN || ev->type == INPUT_KEY_UP) {
        jsb_set_prop(ctx, obj, "keyCode",  jsb_new_i32(ctx, ev->keyCode));
        jsb_set_prop(ctx, obj, "key",      jsb_new_str(ctx, ev->key));
        jsb_set_prop(ctx, obj, "code",     jsb_new_str(ctx, ev->code));
        jsb_set_prop(ctx, obj, "repeat",   jsb_new_bool(ctx, ev->repeat));
        jsb_set_prop(ctx, obj, "shiftKey", jsb_new_bool(ctx, ev->shiftKey));
        jsb_set_prop(ctx, obj, "ctrlKey",  jsb_new_bool(ctx, ev->ctrlKey));
        jsb_set_prop(ctx, obj, "altKey",   jsb_new_bool(ctx, ev->altKey));
        jsb_set_prop(ctx, obj, "metaKey",  jsb_new_bool(ctx, ev->metaKey));
    }

    /* Mouse fields */
    if (ev->type == INPUT_MOUSE_DOWN || ev->type == INPUT_MOUSE_UP ||
        ev->type == INPUT_MOUSE_MOVE || ev->type == INPUT_WHEEL) {
        jsb_set_prop(ctx, obj, "clientX", jsb_new_i32(ctx, ev->mouseX));
        jsb_set_prop(ctx, obj, "clientY", jsb_new_i32(ctx, ev->mouseY));
        jsb_set_prop(ctx, obj, "pageX",   jsb_new_i32(ctx, ev->mouseX));
        jsb_set_prop(ctx, obj, "pageY",   jsb_new_i32(ctx, ev->mouseY));
        jsb_set_prop(ctx, obj, "screenX", jsb_new_i32(ctx, ev->mouseX));
        jsb_set_prop(ctx, obj, "screenY", jsb_new_i32(ctx, ev->mouseY));
        jsb_set_prop(ctx, obj, "button",  jsb_new_i32(ctx, ev->button));
    }

    /* Wheel fields */
    if (ev->type == INPUT_WHEEL) {
        jsb_set_prop(ctx, obj, "deltaX", jsb_new_f64(ctx, ev->wheelDeltaX));
        jsb_set_prop(ctx, obj, "deltaY", jsb_new_f64(ctx, ev->wheelDeltaY));
        jsb_set_prop(ctx, obj, "deltaZ", jsb_new_f64(ctx, 0.0));
        jsb_set_prop(ctx, obj, "deltaMode", jsb_new_i32(ctx, 0));
    }

    return obj;
}

/* pollEvents() -> Array of event objects
 *
 * Why: dom_shim.js drains the input_manager queue once per frame and
 * dispatches each event to its listeners. Arguments are ignored. At most
 * INPUT_MAX_EVENTS events are read, so the call-scoped handles made here
 * stay bounded without a scope per iteration. */
static void js_input_poll_events(jsb_ctx *ctx)
{
    InputEvent events[INPUT_MAX_EVENTS];
    int count = input_manager_poll_events(events, INPUT_MAX_EVENTS);

    jsb_val *arr = jsb_new_array(ctx);
    for (int i = 0; i < count; i++) {
        jsb_set_index(ctx, arr, (uint32_t)i,
                      input_event_to_js(ctx, &events[i]));
    }
    jsb_ret_val(ctx, arr);
}

/* Build the Gamepad-like object for `gp` (borrowed, connected).
 *
 * Why: mirrors the W3C Gamepad interface behind the shim's
 * navigator.getGamepads() (https://w3c.github.io/gamepad/). As upstream had
 * it, `timestamp` is always 0 and `touched` follows `pressed`. Returns a
 * call-scoped handle. */
static jsb_val *gamepad_to_js(jsb_ctx *ctx, const GamepadState *gp)
{
    jsb_val *obj = jsb_new_object(ctx);
    jsb_set_prop(ctx, obj, "id",        jsb_new_str(ctx, gp->id));
    jsb_set_prop(ctx, obj, "index",     jsb_new_i32(ctx, gp->index));
    jsb_set_prop(ctx, obj, "connected", jsb_new_bool(ctx, true));
    jsb_set_prop(ctx, obj, "mapping",   jsb_new_str(ctx, "standard"));
    jsb_set_prop(ctx, obj, "timestamp", jsb_new_f64(ctx, 0.0));

    /* Buttons array: each element has { pressed, value } */
    jsb_val *buttons = jsb_new_array(ctx);
    for (int i = 0; i < INPUT_GAMEPAD_BUTTONS; i++) {
        jsb_val *btn = jsb_new_object(ctx);
        jsb_set_prop(ctx, btn, "pressed",
                     jsb_new_bool(ctx, gp->buttons[i].pressed));
        jsb_set_prop(ctx, btn, "value",
                     jsb_new_f64(ctx, gp->buttons[i].value));
        jsb_set_prop(ctx, btn, "touched",
                     jsb_new_bool(ctx, gp->buttons[i].pressed));
        jsb_set_index(ctx, buttons, (uint32_t)i, btn);
    }
    jsb_set_prop(ctx, obj, "buttons", buttons);

    /* Axes array */
    jsb_val *axes = jsb_new_array(ctx);
    for (int i = 0; i < INPUT_GAMEPAD_AXES; i++) {
        jsb_set_index(ctx, axes, (uint32_t)i,
                      jsb_new_f64(ctx, gp->axes[i]));
    }
    jsb_set_prop(ctx, obj, "axes", axes);

    return obj;
}

/* getGamepad(index) -> Gamepad-like object | null
 *
 * Why: backs the shim's navigator.getGamepads(). `index` is coerced with
 * ToInt32 (so "1" works) and a conversion that throws propagates. No
 * argument, a slot outside 0..INPUT_MAX_GAMEPADS-1 or a disconnected pad
 * gives null. */
static void js_input_get_gamepad(jsb_ctx *ctx)
{
    if (jsb_arg_count(ctx) < 1) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }

    int32_t index;
    if (!jsb_arg_to_i32(ctx, 0, &index))
        return;

    const GamepadState *gp = input_manager_get_gamepad(index);
    if (!gp || !gp->connected) {
        jsb_ret_val(ctx, jsb_new_null(ctx));
        return;
    }
    jsb_ret_val(ctx, gamepad_to_js(ctx, gp));
}

/* getGamepadCount() -> int (number of gamepad slots)
 *
 * Why: the shim sizes navigator.getGamepads() with it. Arguments ignored. */
static void js_input_get_gamepad_count(jsb_ctx *ctx)
{
    jsb_ret_i32(ctx, INPUT_MAX_GAMEPADS);
}

/* Registration */

/* Fills __native_input in upstream's function-list order, with its arities
 * as each function's `length`. Returns false with a JS exception pending. */
static bool input_module_init(jsb_ctx *ctx, jsb_val *module)
{
    return jsb_set_fn_n(ctx, module, "pollEvents",
                        js_input_poll_events, 0) &&
           jsb_set_fn_n(ctx, module, "getGamepad",
                        js_input_get_gamepad, 1) &&
           jsb_set_fn_n(ctx, module, "getGamepadCount",
                        js_input_get_gamepad_count, 0);
}

/* jsb_runtime_setup callback that defines the module; `user` is unused. */
static bool input_define_setup(jsb_ctx *ctx, void *user)
{
    (void)user;
    return jsb_define_module(ctx, "__native_input", input_module_init);
}

bool bind_input_define(jsb_runtime *rt)
{
    if (!rt) {
        fprintf(stderr, "bind_input_define: no jsb runtime\n");
        return false;
    }
    return jsb_runtime_setup(rt, input_define_setup, NULL);
}
