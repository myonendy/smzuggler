# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Game-free process/require shim test; see docs/shims.md. No device bindings.
add_executable(test_process_shim ${CMAKE_CURRENT_LIST_DIR}/test_process_shim.c
    ${PROJECT_SOURCE_DIR}/src/engine/js_engine.c
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_io.c)
target_include_directories(test_process_shim PRIVATE
    ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(test_process_shim PRIVATE qjs)
if(MSVC)
    target_compile_options(test_process_shim PRIVATE /UNDEBUG)
else()
    target_compile_options(test_process_shim PRIVATE -UNDEBUG)
endif()
add_test(NAME process_shim COMMAND test_process_shim
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
