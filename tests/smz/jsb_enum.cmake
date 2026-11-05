# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Registers the jsb enumeration/brand-check test (card M1-T026b) on every
# backend that is built: smz_jsb (V8 in v8 mode, the transitional backend
# otherwise) as jsb_enum, and in v8 mode also the transitional backend as
# jsb_enum_transitional.
set(_smz_enum_targets test_jsb_enum)
add_executable(test_jsb_enum ${CMAKE_CURRENT_LIST_DIR}/test_jsb_enum.c)
target_link_libraries(test_jsb_enum PRIVATE smz_jsb)
add_test(NAME jsb_enum COMMAND test_jsb_enum)
if(SMZ_JS_ENGINE STREQUAL "v8")
    set_target_properties(test_jsb_enum PROPERTIES LINKER_LANGUAGE CXX)
    target_compile_definitions(test_jsb_enum PRIVATE
        SMZ_V8_ICU_DATA_PATH="${SMZ_V8_ICU_DATA}")
    # src/jsb/ has exactly one C backend while the seam has two: select it by
    # extension so this fragment does not pin a filename that M1-T027 deletes.
    file(GLOB _smz_transitional_backend ${PROJECT_SOURCE_DIR}/src/jsb/*.c)
    add_executable(test_jsb_enum_transitional
        ${CMAKE_CURRENT_LIST_DIR}/test_jsb_enum.c
        ${_smz_transitional_backend})
    target_include_directories(test_jsb_enum_transitional PRIVATE
        ${PROJECT_SOURCE_DIR}/src/jsb)
    target_link_libraries(test_jsb_enum_transitional PRIVATE qjs)
    target_compile_definitions(test_jsb_enum_transitional PRIVATE SMZ_V8_ICU_DATA_PATH="")
    add_test(NAME jsb_enum_transitional COMMAND test_jsb_enum_transitional)
    list(APPEND _smz_enum_targets test_jsb_enum_transitional)
else()
    target_compile_definitions(test_jsb_enum PRIVATE SMZ_V8_ICU_DATA_PATH="")
endif()
foreach(_smz_t IN LISTS _smz_enum_targets)
    set_target_properties(${_smz_t} PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
    # The test checks with assert(); keep it live in Release builds.
    if(MSVC)
        target_compile_options(${_smz_t} PRIVATE /UNDEBUG)
    else()
        target_compile_options(${_smz_t} PRIVATE -UNDEBUG)
    endif()
endforeach()
