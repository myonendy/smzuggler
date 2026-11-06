# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Frame-timing summary, JSON Lines and overhead test; see docs/perf.md.
# No SDL or GL context is linked, so GPU timing stays unavailable here.
add_executable(test_frame_stats ${CMAKE_CURRENT_LIST_DIR}/test_frame_stats.c
    ${PROJECT_SOURCE_DIR}/src/smzuggler/frame_stats.c)
target_include_directories(test_frame_stats PRIVATE ${PROJECT_SOURCE_DIR}/src)
if(MSVC)
    target_compile_options(test_frame_stats PRIVATE /UNDEBUG)
else()
    target_compile_options(test_frame_stats PRIVATE -UNDEBUG)
endif()
add_test(NAME frame_stats COMMAND test_frame_stats)

# Existing targets compile game_loop.c directly. Supply the writer they now
# call without editing their source lists. Defer the scan until every
# tests/smz fragment has created its targets, since this file sorts first.
function(smz_add_frame_stats_sources)
    get_property(smz_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(smz_target IN LISTS smz_targets)
        get_target_property(smz_sources ${smz_target} SOURCES)
        if("${smz_sources}" MATCHES "src/engine/game_loop\\.c")
            target_sources(${smz_target} PRIVATE
                ${PROJECT_SOURCE_DIR}/src/smzuggler/frame_stats.c)
        endif()
    endforeach()
endfunction()
cmake_language(DEFER CALL smz_add_frame_stats_sources)
