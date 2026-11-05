# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Registers the bind_renderer jsb port test (card M1-T023, DEC-SETUP-009).
# In v8 mode it runs on smz_js_engine_v8, so the port really runs on V8; in
# quickjs mode on js_engine.c, whose adopted runtime smz_jsb_quickjs supplies
# (linked by the root CMakeLists.txt for every target building js_engine.c).
# The renderer and sprite batch are built without RMMZ_HAS_GL, so no graphics
# device is created and the test stays headless.
set(bind_renderer_jsb_sources
    ${CMAKE_CURRENT_LIST_DIR}/test_bind_renderer_jsb.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_renderer.c
    ${PROJECT_SOURCE_DIR}/src/rendering/renderer.c
    ${PROJECT_SOURCE_DIR}/src/rendering/sprite_batch.c)
if(SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_bind_renderer_jsb ${bind_renderer_jsb_sources})
    target_link_libraries(test_bind_renderer_jsb PRIVATE smz_js_engine_v8)
    set_target_properties(test_bind_renderer_jsb PROPERTIES LINKER_LANGUAGE CXX)
else()
    add_executable(test_bind_renderer_jsb ${bind_renderer_jsb_sources}
        ${PROJECT_SOURCE_DIR}/src/engine/js_engine.c)
    target_link_libraries(test_bind_renderer_jsb PRIVATE smz_jsb_quickjs qjs)
endif()
target_include_directories(test_bind_renderer_jsb PRIVATE
    ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_bind_renderer_jsb PRIVATE ${RMMZ_SYS_LIBS})
set_target_properties(test_bind_renderer_jsb PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON)
# Assertions are the test's checks, including in Release builds.
if(MSVC)
    target_compile_options(test_bind_renderer_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_bind_renderer_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME bind_renderer_jsb COMMAND test_bind_renderer_jsb)
