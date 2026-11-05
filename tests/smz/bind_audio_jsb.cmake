# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Exercise the audio bridge on the selected engine (DEC-SETUP-009).
# Links the real audio engine, but opens no device and retains no JS handles.
set(audio_jsb_sources
    ${CMAKE_CURRENT_LIST_DIR}/test_bind_audio_jsb.c
    ${PROJECT_SOURCE_DIR}/src/bindings/bind_audio.c
    ${PROJECT_SOURCE_DIR}/src/audio/audio_engine.cpp
    ${PROJECT_SOURCE_DIR}/src/io/file_io.c)
if(SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_bind_audio_jsb ${audio_jsb_sources})
    target_link_libraries(test_bind_audio_jsb PRIVATE smz_js_engine_v8)
else()
    add_executable(test_bind_audio_jsb ${audio_jsb_sources}
        ${PROJECT_SOURCE_DIR}/src/engine/js_engine.c)
    target_link_libraries(test_bind_audio_jsb PRIVATE smz_jsb)
endif()
target_include_directories(test_bind_audio_jsb PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_bind_audio_jsb PRIVATE soloud SDL3::SDL3 ${RMMZ_SYS_LIBS})
set_target_properties(test_bind_audio_jsb PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(MSVC)
    target_compile_options(test_bind_audio_jsb PRIVATE /UNDEBUG)
else()
    target_compile_options(test_bind_audio_jsb PRIVATE -UNDEBUG)
endif()
add_test(NAME bind_audio_jsb COMMAND test_bind_audio_jsb)
