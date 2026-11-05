# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Runs the font binding on the selected engine's own context (DEC-SETUP-009).
# Real rendering and file helpers are linked; no graphics device is created.
set(font_jsb_sources
    ${CMAKE_CURRENT_LIST_DIR}/test_bind_font_jsb.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_font.c
    ${PROJECT_SOURCE_DIR}/src/rendering/font_manager.c
    ${PROJECT_SOURCE_DIR}/src/rendering/canvas2d.c
    ${PROJECT_SOURCE_DIR}/src/rendering/woff.c
    ${PROJECT_SOURCE_DIR}/src/rendering/image_loader.c
    ${PROJECT_SOURCE_DIR}/src/rendering/renderer.c
    ${PROJECT_SOURCE_DIR}/src/rendering/sprite_batch.c
    ${PROJECT_SOURCE_DIR}/src/rendering/apng.c
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c)
if(SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_bind_font_jsb ${font_jsb_sources})
    target_link_libraries(test_bind_font_jsb PRIVATE smz_js_engine_v8)
    set_target_properties(test_bind_font_jsb PROPERTIES LINKER_LANGUAGE CXX)
else()
    add_executable(test_bind_font_jsb ${font_jsb_sources}
        ${PROJECT_SOURCE_DIR}/src/engine/js_engine.c)
    target_link_libraries(test_bind_font_jsb PRIVATE smz_jsb_quickjs qjs)
endif()
target_include_directories(test_bind_font_jsb PRIVATE
    ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/third_party/stb)
target_link_libraries(test_bind_font_jsb PRIVATE ${RMMZ_SYS_LIBS})
set_target_properties(test_bind_font_jsb PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(MSVC)
    target_compile_options(test_bind_font_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_bind_font_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME bind_font_jsb COMMAND test_bind_font_jsb)
