# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Register the game-loop JS test of card M1-T009 (spec section 4.1,
# docs/js-binding.md, "Game loop"). It runs in both engine modes: the real
# game_loop.c on js_engine_v8.cc in v8 mode and on js_engine.c in the
# default mode, with the platform, renderer, audio, video and log calls stubbed in
# the test itself.
add_executable(test_game_loop_js
    ${CMAKE_CURRENT_LIST_DIR}/test_game_loop_js.c
    ${CMAKE_SOURCE_DIR}/src/engine/game_loop.c)
if(SMZ_JS_ENGINE STREQUAL "v8")
    target_link_libraries(test_game_loop_js PRIVATE smz_js_engine_v8 SDL3::SDL3)
    set_target_properties(test_game_loop_js PROPERTIES LINKER_LANGUAGE CXX)
else()
    target_sources(test_game_loop_js PRIVATE ${CMAKE_SOURCE_DIR}/src/engine/js_engine.c)
    target_link_libraries(test_game_loop_js PRIVATE qjs SDL3::SDL3)
endif()
target_include_directories(test_game_loop_js PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/tests)
set_target_properties(test_game_loop_js PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_game_loop_js PRIVATE /UNDEBUG)
else()
    target_compile_options(test_game_loop_js PRIVATE -UNDEBUG)
endif()
add_test(NAME game_loop_js COMMAND test_game_loop_js WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
