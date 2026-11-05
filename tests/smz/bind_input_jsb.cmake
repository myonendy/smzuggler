# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Registers the __native_input API test of card M1-T015 (bind_input.c on
# jsb, docs/js-binding.md). It runs in both engine modes: on
# smz_js_engine_v8 (the V8 jsb backend) in v8 mode, on js_engine.c with the
# adopted jsb runtime in the default engine mode.
if(SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_bind_input_jsb
        ${CMAKE_CURRENT_LIST_DIR}/test_bind_input_jsb.c
        ${PROJECT_SOURCE_DIR}/src/bindings/bind_input.c
        ${PROJECT_SOURCE_DIR}/src/input/input_manager.c)
    target_link_libraries(test_bind_input_jsb PRIVATE smz_js_engine_v8 SDL3::SDL3)
    set_target_properties(test_bind_input_jsb PROPERTIES LINKER_LANGUAGE CXX)
else()
    add_executable(test_bind_input_jsb
        ${CMAKE_CURRENT_LIST_DIR}/test_bind_input_jsb.c
        ${PROJECT_SOURCE_DIR}/src/bindings/bind_input.c
        ${PROJECT_SOURCE_DIR}/src/input/input_manager.c
        ${PROJECT_SOURCE_DIR}/src/engine/js_engine.c)
    target_link_libraries(test_bind_input_jsb PRIVATE smz_jsb SDL3::SDL3)
endif()
target_include_directories(test_bind_input_jsb PRIVATE ${PROJECT_SOURCE_DIR}/src)
set_target_properties(test_bind_input_jsb PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_bind_input_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_bind_input_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME bind_input_jsb COMMAND test_bind_input_jsb)
