# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
set(bind_renderer_jsb_sources
    ${CMAKE_CURRENT_LIST_DIR}/test_bind_renderer_jsb.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_renderer.c
    ${PROJECT_SOURCE_DIR}/src/rendering/renderer.c
    ${PROJECT_SOURCE_DIR}/src/rendering/sprite_batch.c)
add_executable(test_bind_renderer_jsb ${bind_renderer_jsb_sources})
target_link_libraries(test_bind_renderer_jsb PRIVATE smz_js_engine_v8)
set_target_properties(test_bind_renderer_jsb PROPERTIES LINKER_LANGUAGE CXX)
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
