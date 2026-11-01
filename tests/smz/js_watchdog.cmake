# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Register the V8 script watchdog test of card M1-T008 (spec section 4.1,
# docs/js-binding.md, "Watchdog"). v8 mode only: the watchdog thread and
# TerminateExecution exist only in js_engine_v8.cc.
if(SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_js_watchdog ${CMAKE_CURRENT_LIST_DIR}/test_js_watchdog.c)
    target_link_libraries(test_js_watchdog PRIVATE smz_js_engine_v8)
    set_target_properties(test_js_watchdog PROPERTIES
        LINKER_LANGUAGE CXX C_STANDARD 11 C_STANDARD_REQUIRED ON)
    # The test checks with assert(); keep it live in Release builds.
    if(MSVC)
        target_compile_options(test_js_watchdog PRIVATE /UNDEBUG)
    else()
        target_compile_options(test_js_watchdog PRIVATE -UNDEBUG)
    endif()
    add_test(NAME js_watchdog COMMAND test_js_watchdog)
endif()
