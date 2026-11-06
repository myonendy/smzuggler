# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Exercise repeated software canvas lookups without game assets or a GPU.
add_executable(test_canvas2d_lookup
    ${CMAKE_CURRENT_LIST_DIR}/test_canvas2d_lookup.c
    ${PROJECT_SOURCE_DIR}/src/rendering/canvas2d.c
    ${PROJECT_SOURCE_DIR}/src/rendering/woff.c
    ${PROJECT_SOURCE_DIR}/src/rendering/image_loader.c
    ${PROJECT_SOURCE_DIR}/src/rendering/apng.c
    ${PROJECT_SOURCE_DIR}/src/rendering/renderer.c
    ${PROJECT_SOURCE_DIR}/src/rendering/sprite_batch.c
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c)
target_include_directories(test_canvas2d_lookup PRIVATE
    ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/third_party/stb)
target_link_libraries(test_canvas2d_lookup PRIVATE ${RMMZ_SYS_LIBS})
if(MSVC)
    target_compile_options(test_canvas2d_lookup PRIVATE /UNDEBUG)
else()
    target_compile_options(test_canvas2d_lookup PRIVATE -UNDEBUG)
endif()
add_test(NAME canvas2d_lookup COMMAND test_canvas2d_lookup)
