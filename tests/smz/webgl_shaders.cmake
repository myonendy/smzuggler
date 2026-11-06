# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# WebGL2 GLSL ES 1.00/3.00 compile, link and draw checks; see docs/webgl.md.
set(webgl_shaders_sources
    ${CMAKE_CURRENT_LIST_DIR}/test_webgl_shaders.c
    ${PROJECT_SOURCE_DIR}/src/webgl/webgl.cc)
add_executable(test_webgl_shaders ${webgl_shaders_sources})
target_link_libraries(test_webgl_shaders PRIVATE
    smz_js_engine_v8 SDL3::SDL3 OpenGL::GL ${RMMZ_SYS_LIBS})
set_target_properties(test_webgl_shaders PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(test_webgl_shaders PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
target_include_directories(test_webgl_shaders PRIVATE ${PROJECT_SOURCE_DIR}/src)
if(MSVC)
    target_compile_options(test_webgl_shaders PRIVATE /UNDEBUG)
else()
    target_compile_options(test_webgl_shaders PRIVATE -UNDEBUG)
endif()
add_test(NAME webgl_shaders COMMAND test_webgl_shaders
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
