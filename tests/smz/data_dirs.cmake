# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Game-free XDG data-directory resolver test; see docs/data-dirs.md.
# Linux cases need no SDL; other platforms link it for SDL_GetPrefPath.
add_executable(test_data_dirs ${CMAKE_CURRENT_LIST_DIR}/test_data_dirs.c
    ${PROJECT_SOURCE_DIR}/src/smzuggler/data_dirs.c)
target_include_directories(test_data_dirs PRIVATE
    ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/tests)
if(WIN32 OR APPLE OR ANDROID)
    target_link_libraries(test_data_dirs PRIVATE SDL3::SDL3)
endif()
if(MSVC)
    target_compile_options(test_data_dirs PRIVATE /UNDEBUG)
else()
    target_compile_options(test_data_dirs PRIVATE -UNDEBUG)
endif()
add_test(NAME data_dirs COMMAND test_data_dirs)
