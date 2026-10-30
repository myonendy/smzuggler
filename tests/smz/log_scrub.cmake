# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Exercise the logger boundary with the same dependencies as error_handler.
# See docs/DECISIONS.md (DEC-M0-T004) and docs/privacy.md; fixtures are temporary and contain no game material.
# These fixtures use POSIX temporary directories and descriptor redirection.
# Windows builds omit both tests; the production filter remains portable.
if(NOT WIN32)
add_executable(test_log_scrub ${CMAKE_CURRENT_LIST_DIR}/test_log_scrub.c
    ${PROJECT_SOURCE_DIR}/src/engine/error_handler.c
    ${PROJECT_SOURCE_DIR}/src/engine/js_engine.c
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_io.c)
target_include_directories(test_log_scrub PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_log_scrub PRIVATE qjs SDL3::SDL3)
add_test(NAME log_scrub COMMAND test_log_scrub)
add_executable(test_log_scrub_boundaries
    ${CMAKE_CURRENT_LIST_DIR}/test_log_scrub_boundaries.c
    ${PROJECT_SOURCE_DIR}/src/smzuggler/log_scrub.c)
target_include_directories(test_log_scrub_boundaries PRIVATE ${PROJECT_SOURCE_DIR}/src)
# Assertions must remain active in the Release build used by the gate.
target_compile_options(test_log_scrub_boundaries PRIVATE -UNDEBUG)
add_test(NAME log_scrub_boundaries COMMAND test_log_scrub_boundaries)
endif()
# Existing upstream test targets compile error_handler directly. Supply the
# privacy boundary to every such target without editing upstream test lists.
get_property(smz_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(smz_target IN LISTS smz_targets)
    get_target_property(smz_sources ${smz_target} SOURCES)
    if("${smz_sources}" MATCHES "src/engine/error_handler\\.c")
        target_sources(${smz_target} PRIVATE ${PROJECT_SOURCE_DIR}/src/smzuggler/log_scrub.c)
    endif()
endforeach()
