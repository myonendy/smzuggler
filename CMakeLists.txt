# Copyright (c) 2026 General Arcade (Pte. Ltd.)
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: build only V8 and remove the former engine selection.
# Modified 2026-10-30: configure runtime identity and diagnostic privacy.
# Modified 2026-10-31: build and stage the headless shim inventory.
# Modified 2026-10-31: restore Outsider's copyright and SPDX lines (spec section 12).
# Modified 2026-11-01: compile host data-directory resolution into the runtime.
# Modified 2026-11-01: build the jsb seam's V8 backend as smz_jsb (src/jsb/).
# Modified 2026-11-01: build js_engine.h on V8 as smz_js_engine_v8 in v8 mode.
# Modified 2026-11-01: add the script watchdog thread to smz_js_engine_v8.
# Modified 2026-11-04: add the V8 code cache store smz_js_codecache.
# Modified 2026-11-04: size V8 recursion for the process main thread.
# Modified 2026-11-05: add the optional V8 CPU profiler (--cpu-prof).
# Modified 2026-11-05: add the MZ_GLES provider switch (system or ANGLE).
# Build orchestration; see docs/DECISIONS.md (DEC-M0-T001).

cmake_minimum_required(VERSION 3.20)
project(outsider C CXX)

set(SMZUGGLER_NAME "smzuggler" CACHE STRING "Runtime display name")
set(SMZUGGLER_BIN "smzuggler-rt" CACHE STRING "Runtime executable name")
add_compile_definitions(
    SMZUGGLER_NAME="${SMZUGGLER_NAME}"
    SMZUGGLER_BIN="${SMZUGGLER_BIN}"
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type defaults to Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Options
include(CMakeDependentOption)
option(RMMZ_BUILD_TESTS "Build tests" ON)
# The tests are desktop executables; an Android build produces only the
# runtime library.
if(ANDROID)
    set(RMMZ_BUILD_TESTS OFF)
endif()
option(RMMZ_WIN32_CONSOLE "Windows: build outsider as a console application instead of a GUI application" OFF)

# Rendering backend. "gpu" targets SDL3's GPU API (Vulkan, D3D12 or Metal)
# with the shader bytecode in src/rendering/shaders; "gl" is the original
# OpenGL 4.5 path, kept as a fallback for machines without a supported GPU
# backend.
set(RMMZ_RENDER_BACKEND "gpu" CACHE STRING "Rendering backend: gpu or gl")
set_property(CACHE RMMZ_RENDER_BACKEND PROPERTY STRINGS gpu gl)
if(NOT RMMZ_RENDER_BACKEND MATCHES "^(gl|gpu)$")
    message(FATAL_ERROR "RMMZ_RENDER_BACKEND must be gpu or gl")
endif()

# Apple's OpenGL stops at 4.1 core and is deprecated; the GL renderer is built
# on 4.5 direct state access, so that backend cannot run here at all. Metal
# always exists instead, which is what the GPU backend uses.
if(APPLE AND RMMZ_RENDER_BACKEND STREQUAL "gl")
    message(FATAL_ERROR
        "RMMZ_RENDER_BACKEND=gl needs OpenGL 4.5, and macOS provides at most "
        "4.1. Build the GPU backend (the default) instead: "
        "-DRMMZ_RENDER_BACKEND=gpu")
endif()

# Compile shaders a game's plugins supply at run time. Only the GPU backend
# needs this: SDL3 takes bytecode rather than source, so without it a custom
# plugin filter draws unfiltered.
cmake_dependent_option(RMMZ_RUNTIME_SHADERS
    "Compile game-supplied shaders at run time (GPU backend)" ON
    "RMMZ_RENDER_BACKEND STREQUAL gpu" OFF)

# SDL3 takes shader bytecode, and only runs a backend whose format we ship:
# DXIL for D3D12, SPIR-V for Vulkan, MSL for Metal. Check the generated header
# carries the one this platform needs, so a missing format fails here instead
# of producing a runtime that builds and then cannot create a device.
if(RMMZ_RENDER_BACKEND STREQUAL "gpu")
    if(APPLE)
        set(RMMZ_NEEDED_SHADER "msl")
    elseif(WIN32)
        set(RMMZ_NEEDED_SHADER "dxil")
    else()
        set(RMMZ_NEEDED_SHADER "spirv")
    endif()
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/shaders/shaders_generated.h"
         RMMZ_SHADER_BLOBS_SRC)
    string(FIND "${RMMZ_SHADER_BLOBS_SRC}" "_${RMMZ_NEEDED_SHADER}[]" RMMZ_HAVE_SHADER)
    unset(RMMZ_SHADER_BLOBS_SRC)
    if(RMMZ_HAVE_SHADER EQUAL -1)
        message(FATAL_ERROR
            "RMMZ_RENDER_BACKEND=gpu needs ${RMMZ_NEEDED_SHADER} shaders on this "
            "platform, and src/rendering/shaders/shaders_generated.h has none.\n"
            "Regenerate them with SDL_shadercross:\n"
            "    python tools/compile_shaders.py --shadercross /path/to/shadercross\n"
            "or build the OpenGL backend with -DRMMZ_RENDER_BACKEND=gl.")
    endif()
endif()

# Import V8's CMake module first: it is the one place that names the pinned
# V8 version (SMZ_V8_VERSION_PIN), so the checks and defaults below never
# repeat the pin and drift from tools/v8/build_v8.sh.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/V8.cmake)

# V8 is the sole engine. A legacy SMZ_JS_ENGINE argument is only a migration
# diagnostic: v8 is accepted when SMZ_V8_DIR is also given, anything else is
# rejected. Normal builds omit the selector entirely.
if(DEFINED SMZ_JS_ENGINE)
    if(NOT SMZ_JS_ENGINE STREQUAL "v8")
        message(FATAL_ERROR
            "SMZ_JS_ENGINE must be omitted or v8, not '${SMZ_JS_ENGINE}'.")
    endif()
    if(NOT SMZ_V8_DIR)
        message(FATAL_ERROR
            "SMZ_JS_ENGINE=v8 needs SMZ_V8_DIR, the V8 install prefix. "
            "Build it with tools/v8/build_v8.sh and pass "
            "-DSMZ_V8_DIR=$SMZ_DEPS_DIR/v8/${SMZ_V8_VERSION_PIN} (docs/build-v8.md).")
    endif()
endif()

# Require the pinned offline install before dependencies. Without the legacy
# selector, use the standard dependency cache so the forge build stays offline.
set(_smz_v8_default "$ENV{SMZ_V8_DIR}")
if(NOT _smz_v8_default)
    if(DEFINED ENV{SMZ_DEPS_DIR})
        set(_smz_v8_default "$ENV{SMZ_DEPS_DIR}/v8/${SMZ_V8_VERSION_PIN}")
    else()
        set(_smz_v8_default "$ENV{HOME}/.cache/smzuggler-dev/deps/v8/${SMZ_V8_VERSION_PIN}")
    endif()
endif()
if(NOT SMZ_V8_DIR)
    set(SMZ_V8_DIR "${_smz_v8_default}" CACHE PATH "V8 install prefix from tools/v8/build_v8.sh" FORCE)
endif()
smz_import_v8()

# Platform-specific system libraries and compiler settings.
# On Linux/macOS libm and pthread are separate link libraries; on Windows
# they are part of the CRT, so RMMZ_SYS_LIBS is empty there.
if(WIN32)
    set(RMMZ_SYS_LIBS "")
    add_compile_definitions(
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_WARNINGS
    )
elseif(ANDROID)
    # Bionic has pthread built in; log and android are for logcat and assets.
    set(RMMZ_SYS_LIBS m log android)
else()
    set(RMMZ_SYS_LIBS m pthread)
endif()
if(MSVC)
    # Source files are UTF-8; silence noisy narrowing/deprecation warnings.
    add_compile_options(/wd4244 /wd4267 /wd4996 /utf-8)
endif()

# Game default resolution
set(RMMZ_DEFAULT_WIDTH 816 CACHE STRING "Default window width")
set(RMMZ_DEFAULT_HEIGHT 624 CACHE STRING "Default window height")

# Third-party dependencies
add_subdirectory(third_party)

# Link OpenGL (the GL backend, and the tests, which build the headless
# rendering sources). Android has no desktop OpenGL: only the GPU backend
# (Vulkan) is available there.
if(ANDROID)
    if(NOT RMMZ_RENDER_BACKEND STREQUAL "gpu")
        message(FATAL_ERROR "Android supports only -DRMMZ_RENDER_BACKEND=gpu")
    endif()
    set(RMMZ_GL_LIB "")
else()
    find_package(OpenGL REQUIRED)
    set(RMMZ_GL_LIB OpenGL::GL)
endif()

# OpenGL ES provider for the WebGL2 stack (spec §4.2). `system` is the Linux,
# Steam Deck and Android default; `angle` selects an installed ANGLE and is
# the Windows/macOS production path and the Linux CI reference oracle. Q5 is
# still open, so neither choice is forced here; see docs/build.md.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/GLES.cmake)

if(RMMZ_RENDER_BACKEND STREQUAL "gpu")
    set(RMMZ_RENDER_SOURCES
        src/rendering/gpu_backend.c
        src/rendering/sprite_batch_gpu.c
        src/rendering/renderer_gpu.c
        src/rendering/filters_gpu.c
        src/rendering/glsl_translate.c
        src/rendering/gpu_shader_runtime.c
    )
else()
    set(RMMZ_RENDER_SOURCES
        src/rendering/gl_loader.c
        src/rendering/sprite_batch.c
        src/rendering/renderer.c
        src/rendering/filters.c
    )
endif()

file(COPY src/shims/ DESTINATION "${PROJECT_BINARY_DIR}/shims"
    FILES_MATCHING PATTERN "*.js")
add_compile_definitions(SMZ_SURFACE_SHIM_DIR="${PROJECT_BINARY_DIR}/shims")

# Main runtime sources
set(RMMZ_RUNTIME_SOURCES
    src/main.c
    ${RMMZ_RENDER_SOURCES}
    src/platform/platform_sdl3.c
    src/platform/win_console.c
    src/platform/android_support.c
    src/engine/game_loop.c
    src/engine/error_handler.c
    src/smzuggler/dump_surface.c
    src/smzuggler/log_scrub.c
    src/smzuggler/data_dirs.c
    src/engine/script_loader.c
    src/io/file_io.c
    src/bindings/bind_io.c
    src/bindings/bind_image.c
    src/rendering/apng.c
    src/bindings/bind_canvas2d.c
    src/bindings/bind_renderer.c
    src/bindings/bind_font.c
    src/bindings/bind_tilemap.c
    src/bindings/bind_audio.c
    src/bindings/bind_input.c
    src/bindings/bind_effekseer.c
    src/bindings/bind_video.c
    src/bindings/bind_filters.c
    src/bindings/bind_platform.c
    src/audio/audio_engine.cpp
    src/effects/effekseer_backend.cpp
    src/video/video_player.c
    src/input/input_manager.c
    src/rendering/image_loader.c
    src/rendering/canvas2d.c
    src/rendering/woff.c
    src/rendering/font_manager.c
    src/rendering/tilemap.c
)

if(ANDROID)
    # SDLActivity loads libmain.so and calls its SDL_main, so on Android the
    # runtime is a shared library rather than an executable.
    add_library(outsider SHARED ${RMMZ_RUNTIME_SOURCES})
    set_target_properties(outsider PROPERTIES OUTPUT_NAME main)
else()
    add_executable(outsider ${RMMZ_RUNTIME_SOURCES})
    set_target_properties(outsider PROPERTIES OUTPUT_NAME "${SMZUGGLER_BIN}")
endif()

# Windows: embed an application manifest so the process uses the UTF-8 code
# page (game directories with non-ASCII names work with narrow CRT calls) and
# is declared per-monitor DPI aware.
if(WIN32)
    target_sources(outsider PRIVATE src/platform/outsider.manifest)
    # GUI subsystem: no console window on launch. SDL3 supplies WinMain.
    # Output still reaches an attached parent console or console.log
    # (see src/platform/win_console.c). RMMZ_WIN32_CONSOLE=ON restores the
    # classic console build for debugging.
    if(NOT RMMZ_WIN32_CONSOLE)
        set_target_properties(outsider PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

target_include_directories(outsider PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pl_mpeg
)

target_compile_definitions(outsider PRIVATE
    RMMZ_DEFAULT_WIDTH=${RMMZ_DEFAULT_WIDTH}
    RMMZ_DEFAULT_HEIGHT=${RMMZ_DEFAULT_HEIGHT}
    $<$<BOOL:${RMMZ_USE_EFFEKSEER}>:RMMZ_HAS_EFFEKSEER>
)

if(RMMZ_RENDER_BACKEND STREQUAL "gpu")
    target_compile_definitions(outsider PRIVATE RMMZ_RENDER_GPU)
else()
    target_compile_definitions(outsider PRIVATE RMMZ_HAS_GL)
endif()

target_link_libraries(outsider PRIVATE
    SDL3::SDL3
    ${RMMZ_GL_LIB}
    smz_js_engine_v8
    smz_js_profiler
    soloud
    ${RMMZ_SYS_LIBS}
)

# The WebGL2 stack reaches the GPU through the selected provider. On a host
# with only the other provider installed SMZ_GLES_AVAILABLE is FALSE and the
# runtime is left unlinked until that provider is selected (docs/build.md).
if(SMZ_GLES_AVAILABLE)
    target_link_libraries(outsider PRIVATE smz_gles)
endif()

if(RMMZ_RUNTIME_SHADERS)
    target_link_libraries(outsider PRIVATE
        sdl_shadercross glslang SPIRV glslang-default-resource-limits)
    target_compile_definitions(outsider PRIVATE RMMZ_RUNTIME_SHADERS)
endif()

if(RMMZ_USE_EFFEKSEER)
    target_link_libraries(outsider PRIVATE Effekseer EffekseerRendererGL)
endif()

# Install target for distribution packaging.
#
# Everything here belongs to the "runtime" component so a distribution can be
# staged with `cmake --install --component runtime`. Dependency install rules
# otherwise land in the same prefix and ship inside the archive.
install(TARGETS outsider
    RUNTIME DESTINATION .
    LIBRARY DESTINATION .
    COMPONENT runtime
)

# Install shim JS files alongside executable
install(DIRECTORY src/shims/
    DESTINATION shims
    COMPONENT runtime
    FILES_MATCHING PATTERN "*.js"
)

# Game directory is set at install time via -DRMMZ_GAME_DIR
set(RMMZ_GAME_DIR "" CACHE PATH "Path to RPG Maker MZ game directory for packaging")
if(RMMZ_GAME_DIR)
    install(DIRECTORY "${RMMZ_GAME_DIR}/"
        DESTINATION game
        COMPONENT runtime
        PATTERN "*.exe" EXCLUDE
        PATTERN "*.dll" EXCLUDE
        PATTERN "*.pak" EXCLUDE
        PATTERN "*.dat" EXCLUDE
        PATTERN "node.dll" EXCLUDE
        PATTERN "chromedriver.exe" EXCLUDE
        PATTERN "notification_helper.exe" EXCLUDE
        PATTERN "locales" EXCLUDE
        PATTERN "lib" EXCLUDE
        PATTERN "icon" EXCLUDE
        PATTERN "css" EXCLUDE
    )
endif()

# C bindings use the V8 seam; see docs/js-binding.md for handle ownership.
add_library(smz_jsb STATIC src/jsb/jsb_v8.cc)
target_link_libraries(smz_jsb PUBLIC smz::v8)
target_include_directories(smz_jsb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/jsb)

# The cache store keeps compiled scripts outside the game directory.
add_library(smz_js_codecache STATIC src/engine/js_codecache.cc)
target_include_directories(smz_js_codecache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(NOT MSVC)
    target_compile_options(smz_js_codecache PRIVATE -Wall -Wextra)
endif()
add_library(smz_js_engine_v8 STATIC src/engine/js_engine_v8.cc
    src/engine/js_watchdog.cc)
find_package(Threads REQUIRED)
target_link_libraries(smz_js_engine_v8 PUBLIC smz_jsb smz_js_codecache Threads::Threads)
target_include_directories(smz_js_engine_v8 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(smz_js_engine_v8
    PUBLIC SMZ_JS_ENGINE_V8
    PRIVATE SMZ_V8_ICU_DATA_PATH="${SMZ_V8_ICU_DATA}")

# --cpu-prof records V8's own samples and writes a DevTools .cpuprofile.
add_library(smz_js_profiler STATIC src/engine/js_profiler.cc)
target_link_libraries(smz_js_profiler PUBLIC smz::v8 smz_jsb smz_js_codecache)
target_include_directories(smz_js_profiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(NOT MSVC)
    target_compile_options(smz_js_profiler PRIVATE -Wall -Wextra)
endif()

# Tests
if(RMMZ_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
