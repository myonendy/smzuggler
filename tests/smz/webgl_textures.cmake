# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# WebGL2 texture upload checks; see docs/webgl.md.
set(webgl_textures_sources
    ${CMAKE_CURRENT_LIST_DIR}/test_webgl_textures.c
    ${PROJECT_SOURCE_DIR}/src/webgl/webgl.cc
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_canvas2d.c
    ${PROJECT_SOURCE_DIR}/src/rendering/canvas2d.c
    ${PROJECT_SOURCE_DIR}/src/rendering/font_manager.c
    ${PROJECT_SOURCE_DIR}/src/rendering/woff.c
    ${PROJECT_SOURCE_DIR}/src/rendering/image_loader.c
    ${PROJECT_SOURCE_DIR}/src/rendering/renderer.c
    ${PROJECT_SOURCE_DIR}/src/rendering/sprite_batch.c
    ${PROJECT_SOURCE_DIR}/src/rendering/apng.c
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c)
add_executable(test_webgl_textures ${webgl_textures_sources})
target_link_libraries(test_webgl_textures PRIVATE
    smz_js_engine_v8 SDL3::SDL3 OpenGL::GL ${RMMZ_SYS_LIBS})
target_include_directories(test_webgl_textures PRIVATE
    ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/third_party/stb)
set_target_properties(test_webgl_textures PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(test_webgl_textures PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(MSVC)
    target_compile_options(test_webgl_textures PRIVATE /UNDEBUG)
else()
    target_compile_options(test_webgl_textures PRIVATE -UNDEBUG)
endif()
add_test(NAME webgl_textures COMMAND test_webgl_textures
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
