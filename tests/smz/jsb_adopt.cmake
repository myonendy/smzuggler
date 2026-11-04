# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Registers the jsb adoption test (card M1-T007b). It runs on the QuickJS
# engine (js_engine.c) in both engine modes, because the runtime does until
# M1-T027; smz_jsb_quickjs supplies the adopted jsb runtime.
add_executable(test_jsb_adopt
    ${CMAKE_CURRENT_LIST_DIR}/test_jsb_adopt.c
    ${PROJECT_SOURCE_DIR}/src/engine/js_engine.c)
target_include_directories(test_jsb_adopt PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_jsb_adopt PRIVATE smz_jsb_quickjs qjs)
set_target_properties(test_jsb_adopt PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_jsb_adopt PRIVATE /UNDEBUG)
else()
    target_compile_options(test_jsb_adopt PRIVATE -UNDEBUG)
endif()
add_test(NAME jsb_adopt COMMAND test_jsb_adopt)
