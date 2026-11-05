# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
add_executable(test_stack_size ${CMAKE_CURRENT_LIST_DIR}/test_stack_size.c)
target_link_libraries(test_stack_size PRIVATE smz_js_engine_v8)
set_target_properties(test_stack_size PROPERTIES LINKER_LANGUAGE CXX C_STANDARD 11)
if(MSVC)
    target_compile_options(test_stack_size PRIVATE /UNDEBUG)
else()
    target_compile_options(test_stack_size PRIVATE -UNDEBUG)
endif()
add_test(NAME stack_size COMMAND test_stack_size)
