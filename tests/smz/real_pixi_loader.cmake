# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Planned real-PIXI versus --renderer=outsider script list (spec §4.2).
# Why: the two modes must be assertable without a window, GL device or game
# copy, so this links the loader's query path only. No SDL is needed.
add_executable(test_real_pixi_loader
    ${CMAKE_CURRENT_LIST_DIR}/test_real_pixi_loader.c
    ${PROJECT_SOURCE_DIR}/src/engine/script_loader.c
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c)
target_include_directories(test_real_pixi_loader PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/tests)
target_link_libraries(test_real_pixi_loader PRIVATE smz_js_engine_v8)
set_target_properties(test_real_pixi_loader PROPERTIES LINKER_LANGUAGE CXX)
if(MSVC)
    target_compile_options(test_real_pixi_loader PRIVATE /UNDEBUG)
else()
    target_compile_options(test_real_pixi_loader PRIVATE -UNDEBUG)
endif()
add_test(NAME real_pixi_loader COMMAND test_real_pixi_loader)
