# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Register the V8 code cache test of card M1-T012 (spec sections 4.1 item 4
# and 5.6; docs/js-binding.md, "Code cache"). The store checks run in both
# engine modes against smz_js_codecache; in v8 mode the test links
# smz_js_engine_v8 and also checks the engine's produce/consume/reject path.
add_executable(test_codecache ${CMAKE_CURRENT_LIST_DIR}/test_codecache.c)
if(SMZ_JS_ENGINE STREQUAL "v8")
    target_link_libraries(test_codecache PRIVATE smz_js_engine_v8)
else()
    target_link_libraries(test_codecache PRIVATE smz_js_codecache)
endif()
set_target_properties(test_codecache PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(test_codecache PRIVATE ${CMAKE_SOURCE_DIR}/tests)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_codecache PRIVATE /UNDEBUG)
else()
    target_compile_options(test_codecache PRIVATE -UNDEBUG)
endif()
add_test(NAME codecache COMMAND test_codecache)
