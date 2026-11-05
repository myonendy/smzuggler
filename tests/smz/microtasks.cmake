# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
add_executable(test_microtasks
    ${CMAKE_CURRENT_LIST_DIR}/test_microtasks.c
    ${CMAKE_SOURCE_DIR}/src/engine/game_loop.c)
target_link_libraries(test_microtasks PRIVATE smz_js_engine_v8 SDL3::SDL3)
set_target_properties(test_microtasks PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(test_microtasks PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)
set_target_properties(test_microtasks PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_microtasks PRIVATE /UNDEBUG)
else()
    target_compile_options(test_microtasks PRIVATE -UNDEBUG)
endif()
add_test(NAME microtasks COMMAND test_microtasks WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
