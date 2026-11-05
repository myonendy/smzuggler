# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
set(effekseer_jsb_sources
    ${CMAKE_CURRENT_LIST_DIR}/test_bind_effekseer_jsb.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_effekseer.c
    ${PROJECT_SOURCE_DIR}/src/effects/effekseer_backend.cpp
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c)
add_executable(test_bind_effekseer_jsb ${effekseer_jsb_sources})
target_link_libraries(test_bind_effekseer_jsb PRIVATE smz_js_engine_v8)
target_include_directories(test_bind_effekseer_jsb PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_bind_effekseer_jsb PRIVATE SDL3::SDL3 ${RMMZ_SYS_LIBS})
set_target_properties(test_bind_effekseer_jsb PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(MSVC)
    target_compile_options(test_bind_effekseer_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_bind_effekseer_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME bind_effekseer_jsb COMMAND test_bind_effekseer_jsb)
