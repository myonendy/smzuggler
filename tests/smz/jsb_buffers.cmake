# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Registers the jsb zero-copy buffer test (test_jsb_buffers.c) in both
# engine modes; smz_jsb is the V8 backend in v8 mode and the transitional
# QuickJS backend (DEC-SETUP-008) otherwise. QuickJS has no ICU, so the ICU
# data path is empty there and unused.
add_executable(test_jsb_buffers ${CMAKE_CURRENT_LIST_DIR}/test_jsb_buffers.c)
target_link_libraries(test_jsb_buffers PRIVATE smz_jsb)
set_target_properties(test_jsb_buffers PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(SMZ_JS_ENGINE STREQUAL "v8")
    set_target_properties(test_jsb_buffers PROPERTIES LINKER_LANGUAGE CXX)
    target_compile_definitions(test_jsb_buffers PRIVATE
        SMZ_V8_ICU_DATA_PATH="${SMZ_V8_ICU_DATA}")
else()
    target_compile_definitions(test_jsb_buffers PRIVATE SMZ_V8_ICU_DATA_PATH="")
endif()
if(MSVC)
    target_compile_options(test_jsb_buffers PRIVATE /UNDEBUG)
else()
    target_compile_options(test_jsb_buffers PRIVATE -UNDEBUG)
endif()
add_test(NAME jsb_buffers COMMAND test_jsb_buffers)
