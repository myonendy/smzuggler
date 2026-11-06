# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# §14 harness argument and fixture rules; see docs/perf.md.
# main.c is compiled without main() and without SDL on Linux, so the parser
# and save-map reader are tested exactly as the runtime links them.
add_executable(test_perf_suite_args
    ${CMAKE_CURRENT_LIST_DIR}/test_perf_suite_args.c
    ${PROJECT_SOURCE_DIR}/src/main.c)
target_compile_definitions(test_perf_suite_args PRIVATE RMMZ_TEST_DATA_DIR)
target_include_directories(test_perf_suite_args PRIVATE
    ${PROJECT_SOURCE_DIR}/src)
if(WIN32 OR APPLE)
    target_link_libraries(test_perf_suite_args PRIVATE SDL3::SDL3 ${RMMZ_SYS_LIBS})
endif()
if(MSVC)
    target_compile_options(test_perf_suite_args PRIVATE /UNDEBUG)
else()
    target_compile_options(test_perf_suite_args PRIVATE -UNDEBUG)
endif()
add_test(NAME perf_suite_args COMMAND test_perf_suite_args)
