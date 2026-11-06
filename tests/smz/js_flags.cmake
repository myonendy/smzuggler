# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# --js-flags validation and V8 pass-through checks (card M1-T029).
# src/main.c is compiled with RMMZ_TEST_DATA_DIR so this test can call the
# allow-list parser without pulling the window, renderer and bindings in.
add_executable(test_js_flags
    ${CMAKE_CURRENT_LIST_DIR}/test_js_flags.c
    ${CMAKE_SOURCE_DIR}/src/main.c)
target_compile_definitions(test_js_flags PRIVATE RMMZ_TEST_DATA_DIR)
target_include_directories(test_js_flags PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(test_js_flags PRIVATE smz_js_engine_v8)
# main.c includes SDL3 on non-Linux hosts even in this reduced test build.
if(WIN32 OR APPLE OR ANDROID)
    target_link_libraries(test_js_flags PRIVATE SDL3::SDL3 ${RMMZ_SYS_LIBS})
endif()
set_target_properties(test_js_flags PROPERTIES
    LINKER_LANGUAGE CXX
    C_STANDARD 11
    C_STANDARD_REQUIRED ON)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_js_flags PRIVATE /UNDEBUG)
else()
    target_compile_options(test_js_flags PRIVATE -UNDEBUG)
endif()
add_test(NAME js_flags COMMAND test_js_flags)
