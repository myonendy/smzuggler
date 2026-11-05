# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Registers the checks particular to the transitional jsb backend
# (handle chunks, per-runtime function tables) in the default engine mode
# only; removed with that backend by M1-T027.
if(NOT SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_jsb_transitional ${CMAKE_CURRENT_LIST_DIR}/test_jsb_transitional.c)
    target_link_libraries(test_jsb_transitional PRIVATE smz_jsb)
    set_target_properties(test_jsb_transitional PROPERTIES
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    if(MSVC)
        target_compile_options(test_jsb_transitional PRIVATE /UNDEBUG)
    else()
        target_compile_options(test_jsb_transitional PRIVATE -UNDEBUG)
    endif()
    add_test(NAME jsb_transitional COMMAND test_jsb_transitional)
endif()
