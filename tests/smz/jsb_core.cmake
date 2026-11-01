# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Register the jsb seam tests (spec section 4.1, docs/js-binding.md).
# The header check builds in both engine modes; test_jsb_core needs the V8
# backend (smz_jsb), so it exists only with SMZ_JS_ENGINE=v8 until the
# transitional QuickJS backend of DEC-SETUP-008 lands (M1-T004).
add_library(jsb_header_check OBJECT ${CMAKE_CURRENT_LIST_DIR}/test_jsb_header.c)
target_include_directories(jsb_header_check PRIVATE ${CMAKE_SOURCE_DIR}/src/jsb)
set_target_properties(jsb_header_check PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
if(NOT MSVC)
    target_compile_options(jsb_header_check PRIVATE -Wall -Wextra -pedantic-errors)
endif()

if(SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_jsb_core ${CMAKE_CURRENT_LIST_DIR}/test_jsb_core.c)
    target_link_libraries(test_jsb_core PRIVATE smz_jsb)
    set_target_properties(test_jsb_core PROPERTIES
        C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
    target_compile_definitions(test_jsb_core PRIVATE
        SMZ_V8_ICU_DATA_PATH="${SMZ_V8_ICU_DATA}")
    add_test(NAME jsb_core COMMAND test_jsb_core)
endif()
