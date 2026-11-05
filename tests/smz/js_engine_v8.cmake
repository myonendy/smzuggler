# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
add_executable(test_js_engine_stack ${CMAKE_CURRENT_LIST_DIR}/test_js_engine_v8.c)
target_link_libraries(test_js_engine_stack PRIVATE smz_js_engine_v8)
add_executable(test_js_engine_on_v8 ${CMAKE_SOURCE_DIR}/tests/test_js_engine.c)
target_link_libraries(test_js_engine_on_v8 PRIVATE smz_js_engine_v8)
set_target_properties(test_js_engine_stack test_js_engine_on_v8 PROPERTIES
    LINKER_LANGUAGE CXX)
target_include_directories(test_js_engine_on_v8 PRIVATE ${CMAKE_SOURCE_DIR}/tests)
add_test(NAME js_engine_on_v8 COMMAND test_js_engine_on_v8)
target_include_directories(test_js_engine_stack PRIVATE ${CMAKE_SOURCE_DIR}/tests)
set_target_properties(test_js_engine_stack PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_js_engine_stack PRIVATE /UNDEBUG)
else()
    target_compile_options(test_js_engine_stack PRIVATE -UNDEBUG)
endif()
add_test(NAME js_engine_stack COMMAND test_js_engine_stack)

# Both registrations now run the same V8 cache fixtures in the scratch root.
# Keep every assertion while preventing one run from deleting the other's files.
set_tests_properties(js_engine js_engine_on_v8 PROPERTIES
    RESOURCE_LOCK js_engine_fixtures)
