# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Register the microtask-order test of card M1-T010 (spec section 4.1,
# docs/js-binding.md, "Microtask checkpoints", DEC-M1-T010). It runs the real
# game_loop.c, dom_shim.js and storage_shim.js on js_engine_v8.cc in v8 mode
# and on js_engine.c in the default engine mode; platform, renderer, audio,
# video and log calls are stubbed in the test itself.
add_executable(test_microtasks
    ${CMAKE_CURRENT_LIST_DIR}/test_microtasks.c
    ${CMAKE_SOURCE_DIR}/src/engine/game_loop.c)
if(SMZ_JS_ENGINE STREQUAL "v8")
    target_link_libraries(test_microtasks PRIVATE smz_js_engine_v8 SDL3::SDL3)
    set_target_properties(test_microtasks PROPERTIES LINKER_LANGUAGE CXX)
else()
    target_sources(test_microtasks PRIVATE ${CMAKE_SOURCE_DIR}/src/engine/js_engine.c)
    target_link_libraries(test_microtasks PRIVATE qjs SDL3::SDL3)
endif()
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
