# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: use the sole V8 engine for surface inventory.
# Pins which members the surface dump leaves out (DEC-M0-T009b); see
# docs/shims.md. Game-free: no device bindings are linked.
add_executable(test_dump_surface_members
    ${CMAKE_CURRENT_LIST_DIR}/test_dump_surface_members.c
    ${PROJECT_SOURCE_DIR}/src/smzuggler/dump_surface.c
    ${PROJECT_SOURCE_DIR}/src/engine/script_loader.c
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_io.c)
target_include_directories(test_dump_surface_members PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_dump_surface_members PRIVATE smz_js_engine_v8 SDL3::SDL3)
target_compile_definitions(test_dump_surface_members PRIVATE
    SHIM_DIR="${PROJECT_BINARY_DIR}/shims")
if(MSVC)
    target_compile_options(test_dump_surface_members PRIVATE /UNDEBUG)
else()
    target_compile_options(test_dump_surface_members PRIVATE -UNDEBUG)
endif()
add_test(NAME dump_surface_members COMMAND test_dump_surface_members)
