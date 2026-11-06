# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# --cpu-prof writes a Chrome DevTools .cpuprofile (card M1-T028).
add_executable(test_cpu_prof ${CMAKE_CURRENT_LIST_DIR}/test_cpu_prof.c)
target_link_libraries(test_cpu_prof PRIVATE smz_js_engine_v8 smz_js_profiler)
set_target_properties(test_cpu_prof PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(test_cpu_prof PRIVATE ${CMAKE_SOURCE_DIR}/tests)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_cpu_prof PRIVATE /UNDEBUG)
else()
    target_compile_options(test_cpu_prof PRIVATE -UNDEBUG)
endif()
add_test(NAME cpu_prof COMMAND test_cpu_prof)
