# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
set(canvas2d_jsb_sources
    ${CMAKE_CURRENT_LIST_DIR}/test_bind_canvas2d_jsb.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_canvas2d.c
    ${PROJECT_SOURCE_DIR}/src/rendering/font_manager.c
    ${PROJECT_SOURCE_DIR}/src/rendering/canvas2d.c
    ${PROJECT_SOURCE_DIR}/src/rendering/woff.c
    ${PROJECT_SOURCE_DIR}/src/rendering/image_loader.c
    ${PROJECT_SOURCE_DIR}/src/rendering/renderer.c
    ${PROJECT_SOURCE_DIR}/src/rendering/sprite_batch.c
    ${PROJECT_SOURCE_DIR}/src/rendering/apng.c
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c)
add_executable(test_bind_canvas2d_jsb ${canvas2d_jsb_sources})
target_link_libraries(test_bind_canvas2d_jsb PRIVATE smz_js_engine_v8)
set_target_properties(test_bind_canvas2d_jsb PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(test_bind_canvas2d_jsb PRIVATE
    ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/third_party/stb)
target_link_libraries(test_bind_canvas2d_jsb PRIVATE ${RMMZ_SYS_LIBS})
set_target_properties(test_bind_canvas2d_jsb PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(MSVC)
    target_compile_options(test_bind_canvas2d_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_bind_canvas2d_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME bind_canvas2d_jsb COMMAND test_bind_canvas2d_jsb)
