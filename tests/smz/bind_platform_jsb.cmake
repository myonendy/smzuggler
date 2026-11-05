# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Registers the bind_platform jsb port test (card M1-T014, DEC-SETUP-009).
# In v8 mode it runs on smz_js_engine_v8, so the port really runs on V8; in
# quickjs mode on js_engine.c, whose adopted runtime smz_jsb_quickjs supplies
# (linked by the root CMakeLists.txt for every target building js_engine.c).
# The test replaces the Platform functions, so it needs SDL3 only for quit().
if(SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_bind_platform_jsb
        ${CMAKE_CURRENT_LIST_DIR}/test_bind_platform_jsb.c
        ${PROJECT_SOURCE_DIR}/src/bindings/bind_platform.c)
    target_link_libraries(test_bind_platform_jsb PRIVATE smz_js_engine_v8)
    set_target_properties(test_bind_platform_jsb PROPERTIES LINKER_LANGUAGE CXX)
else()
    add_executable(test_bind_platform_jsb
        ${CMAKE_CURRENT_LIST_DIR}/test_bind_platform_jsb.c
        ${PROJECT_SOURCE_DIR}/src/bindings/bind_platform.c
        ${PROJECT_SOURCE_DIR}/src/engine/js_engine.c)
    target_link_libraries(test_bind_platform_jsb PRIVATE qjs)
endif()
target_include_directories(test_bind_platform_jsb PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_bind_platform_jsb PRIVATE SDL3::SDL3)
set_target_properties(test_bind_platform_jsb PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_bind_platform_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_bind_platform_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME bind_platform_jsb COMMAND test_bind_platform_jsb)
