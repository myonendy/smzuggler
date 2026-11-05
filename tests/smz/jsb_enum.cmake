# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
set(_smz_enum_targets test_jsb_enum)
add_executable(test_jsb_enum ${CMAKE_CURRENT_LIST_DIR}/test_jsb_enum.c)
target_link_libraries(test_jsb_enum PRIVATE smz_jsb)
add_test(NAME jsb_enum COMMAND test_jsb_enum)
set_target_properties(test_jsb_enum PROPERTIES LINKER_LANGUAGE CXX)
target_compile_definitions(test_jsb_enum PRIVATE
    SMZ_V8_ICU_DATA_PATH="${SMZ_V8_ICU_DATA}")
foreach(_smz_t IN LISTS _smz_enum_targets)
    set_target_properties(${_smz_t} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
    # The test checks with assert(); keep it live in Release builds.
    if(MSVC)
        target_compile_options(${_smz_t} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${_smz_t} PRIVATE -UNDEBUG)
    endif()
endforeach()
