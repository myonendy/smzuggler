# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: use the sole V8 engine for surface inventory.
# Game-free inventory test; see docs/shims.md. No device bindings are linked.
add_executable(test_dump_surface ${CMAKE_CURRENT_LIST_DIR}/test_dump_surface.c
    ${PROJECT_SOURCE_DIR}/src/smzuggler/dump_surface.c
    ${PROJECT_SOURCE_DIR}/src/engine/script_loader.c
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_io.c)
target_include_directories(test_dump_surface PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_dump_surface PRIVATE smz_js_engine_v8 SDL3::SDL3)
target_compile_definitions(test_dump_surface PRIVATE SHIM_DIR="${PROJECT_BINARY_DIR}/shims")
if(MSVC)
    target_compile_options(test_dump_surface PRIVATE /UNDEBUG)
else()
    target_compile_options(test_dump_surface PRIVATE -UNDEBUG)
endif()
add_test(NAME dump_surface COMMAND test_dump_surface)
