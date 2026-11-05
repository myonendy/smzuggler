# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
add_executable(test_codecache ${CMAKE_CURRENT_LIST_DIR}/test_codecache.c)
target_link_libraries(test_codecache PRIVATE smz_js_engine_v8)
set_target_properties(test_codecache PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(test_codecache PRIVATE ${CMAKE_SOURCE_DIR}/tests)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_codecache PRIVATE /UNDEBUG)
else()
    target_compile_options(test_codecache PRIVATE -UNDEBUG)
endif()
add_test(NAME codecache COMMAND test_codecache)
