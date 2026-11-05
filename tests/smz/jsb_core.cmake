# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
add_library(jsb_header_check OBJECT ${CMAKE_CURRENT_LIST_DIR}/test_jsb_header.c)
target_include_directories(jsb_header_check PRIVATE ${CMAKE_SOURCE_DIR}/src/jsb)
set_target_properties(jsb_header_check PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
if(NOT MSVC)
    target_compile_options(jsb_header_check PRIVATE -Wall -Wextra -pedantic-errors)
endif()

add_executable(test_jsb_core ${CMAKE_CURRENT_LIST_DIR}/test_jsb_core.c)
target_link_libraries(test_jsb_core PRIVATE smz_jsb)
set_target_properties(test_jsb_core PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
target_compile_definitions(test_jsb_core PRIVATE
    SMZ_V8_ICU_DATA_PATH="${SMZ_V8_ICU_DATA}")
if(MSVC)
    target_compile_options(test_jsb_core PRIVATE /UNDEBUG)
else()
    target_compile_options(test_jsb_core PRIVATE -UNDEBUG)
endif()
add_test(NAME jsb_core COMMAND test_jsb_core)
