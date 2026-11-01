# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Registers the unchanged jsb seam test (test_jsb_core.c) against the
# transitional QuickJS backend in quickjs mode (DEC-SETUP-008; removed with
# that backend by M1-T027). jsb_core.cmake registers it for v8 mode. QuickJS
# has no ICU, so the ICU data path the test passes is empty and unused.
if(NOT SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_jsb_core ${CMAKE_CURRENT_LIST_DIR}/test_jsb_core.c)
    target_link_libraries(test_jsb_core PRIVATE smz_jsb)
    set_target_properties(test_jsb_core PROPERTIES
        C_STANDARD 11 C_STANDARD_REQUIRED ON)
    target_compile_definitions(test_jsb_core PRIVATE SMZ_V8_ICU_DATA_PATH="")
    if(MSVC)
        target_compile_options(test_jsb_core PRIVATE /UNDEBUG)
    else()
        target_compile_options(test_jsb_core PRIVATE -UNDEBUG)
    endif()
    add_test(NAME jsb_core COMMAND test_jsb_core)
endif()
