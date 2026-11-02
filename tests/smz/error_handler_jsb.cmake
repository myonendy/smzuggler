# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Register the engine-neutral error handler test of card M1-T011 (spec
# section 4.1 item 5, docs/js-binding.md, "Error handler"). It runs in both
# engine modes: on js_engine_v8.cc (with jsb callbacks) in v8 mode, on
# js_engine.c in quickjs mode.
if(SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_error_handler_jsb
        ${CMAKE_CURRENT_LIST_DIR}/test_error_handler_jsb.c
        ${PROJECT_SOURCE_DIR}/src/engine/error_handler.c
        ${PROJECT_SOURCE_DIR}/src/smzuggler/log_scrub.c)
    target_link_libraries(test_error_handler_jsb PRIVATE smz_js_engine_v8 SDL3::SDL3)
    set_target_properties(test_error_handler_jsb PROPERTIES LINKER_LANGUAGE CXX)
else()
    add_executable(test_error_handler_jsb
        ${CMAKE_CURRENT_LIST_DIR}/test_error_handler_jsb.c
        ${PROJECT_SOURCE_DIR}/src/engine/error_handler.c
        ${PROJECT_SOURCE_DIR}/src/engine/js_engine.c
        ${PROJECT_SOURCE_DIR}/src/io/file_io.c
        ${PROJECT_SOURCE_DIR}/src/bindings/bind_io.c
        ${PROJECT_SOURCE_DIR}/src/smzuggler/log_scrub.c)
    target_link_libraries(test_error_handler_jsb PRIVATE qjs SDL3::SDL3)
endif()
target_include_directories(test_error_handler_jsb PRIVATE ${PROJECT_SOURCE_DIR}/src)
set_target_properties(test_error_handler_jsb PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
# The test checks with assert(); keep it live in Release builds.
if(MSVC)
    target_compile_options(test_error_handler_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_error_handler_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME error_handler_jsb COMMAND test_error_handler_jsb)
