# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# V8 engine-owned jsb runtime checks; see docs/js-binding.md.
add_executable(test_js_engine_v8_jsb
    ${CMAKE_CURRENT_LIST_DIR}/test_js_engine_v8_jsb.c)
target_link_libraries(test_js_engine_v8_jsb PRIVATE smz_js_engine_v8)
set_target_properties(test_js_engine_v8_jsb PROPERTIES
    LINKER_LANGUAGE CXX C_STANDARD 11 C_STANDARD_REQUIRED ON)
target_compile_definitions(test_js_engine_v8_jsb PRIVATE
    SMZ_V8_ICU_DATA_PATH="${SMZ_V8_ICU_DATA}")
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_js_engine_v8_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_js_engine_v8_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME js_engine_v8_jsb COMMAND test_js_engine_v8_jsb)
