# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# WebGL2 Fast API registration checks; see docs/webgl.md.
set(webgl_fast_api_sources
    ${CMAKE_CURRENT_LIST_DIR}/test_webgl_fast_api.cc
    ${PROJECT_SOURCE_DIR}/src/webgl/webgl.cc)
add_executable(test_webgl_fast_api ${webgl_fast_api_sources})
target_link_libraries(test_webgl_fast_api PRIVATE
    smz_js_engine_v8 SDL3::SDL3 OpenGL::GL ${RMMZ_SYS_LIBS})
set_target_properties(test_webgl_fast_api PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(test_webgl_fast_api PRIVATE ${PROJECT_SOURCE_DIR}/src)
add_test(NAME webgl_fast_api COMMAND test_webgl_fast_api)
