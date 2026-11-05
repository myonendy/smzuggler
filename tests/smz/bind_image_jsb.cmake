# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
add_executable(test_bind_image_jsb
    ${CMAKE_CURRENT_LIST_DIR}/test_bind_image_jsb.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_image.c
    ${PROJECT_SOURCE_DIR}/src/rendering/image_loader.c
    ${PROJECT_SOURCE_DIR}/src/rendering/renderer.c
    ${PROJECT_SOURCE_DIR}/src/rendering/sprite_batch.c
    ${PROJECT_SOURCE_DIR}/src/rendering/apng.c
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c)
target_link_libraries(test_bind_image_jsb PRIVATE smz_js_engine_v8)
set_target_properties(test_bind_image_jsb PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(test_bind_image_jsb PRIVATE
    ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/third_party/stb)
set_target_properties(test_bind_image_jsb PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
# Assertions are the test's checks, including in Release builds.
if(MSVC)
    target_compile_options(test_bind_image_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_bind_image_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME bind_image_jsb COMMAND test_bind_image_jsb
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
