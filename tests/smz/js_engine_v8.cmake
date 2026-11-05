# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Register the js_engine.h tests of card M1-T007 (spec section 4.1,
# docs/js-binding.md, "js_engine on V8").
# - js_engine_stack (test_js_engine_v8.c) runs in both engine modes: on
#   js_engine_v8.cc in v8 mode, on js_engine.c in the default mode.
# - js_engine_on_v8 builds Outsider's unchanged tests/test_js_engine.c, which
#   uses only js_engine.h, against js_engine_v8.cc (v8 mode only; the
#   mode already runs it as js_engine).
if(SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_js_engine_stack ${CMAKE_CURRENT_LIST_DIR}/test_js_engine_v8.c)
    target_link_libraries(test_js_engine_stack PRIVATE smz_js_engine_v8)
    add_executable(test_js_engine_on_v8 ${CMAKE_SOURCE_DIR}/tests/test_js_engine.c)
    target_link_libraries(test_js_engine_on_v8 PRIVATE smz_js_engine_v8)
    set_target_properties(test_js_engine_stack test_js_engine_on_v8 PROPERTIES
        LINKER_LANGUAGE CXX)
    target_include_directories(test_js_engine_on_v8 PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    add_test(NAME js_engine_on_v8 COMMAND test_js_engine_on_v8)
else()
    add_executable(test_js_engine_stack
        ${CMAKE_CURRENT_LIST_DIR}/test_js_engine_v8.c
        ${CMAKE_SOURCE_DIR}/src/engine/js_engine.c)
    target_include_directories(test_js_engine_stack PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_js_engine_stack PRIVATE qjs)
endif()
target_include_directories(test_js_engine_stack PRIVATE ${CMAKE_SOURCE_DIR}/tests)
set_target_properties(test_js_engine_stack PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_js_engine_stack PRIVATE /UNDEBUG)
else()
    target_compile_options(test_js_engine_stack PRIVATE -UNDEBUG)
endif()
add_test(NAME js_engine_stack COMMAND test_js_engine_stack)
