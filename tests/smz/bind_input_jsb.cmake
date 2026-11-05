# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
add_executable(test_bind_input_jsb
    ${CMAKE_CURRENT_LIST_DIR}/test_bind_input_jsb.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_input.c
    ${PROJECT_SOURCE_DIR}/src/input/input_manager.c)
target_link_libraries(test_bind_input_jsb PRIVATE smz_js_engine_v8 SDL3::SDL3)
set_target_properties(test_bind_input_jsb PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(test_bind_input_jsb PRIVATE ${PROJECT_SOURCE_DIR}/src)
set_target_properties(test_bind_input_jsb PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_bind_input_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_bind_input_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME bind_input_jsb COMMAND test_bind_input_jsb)
