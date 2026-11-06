# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Imports an installed ANGLE as the GLES provider for MZ_GLES=angle (spec §4.2).
#
# Why: ANGLE is not built by this CMake project. Its authoritative build is
# GN/Ninja over a gclient checkout, and the offline-build rule in AGENTS.md
# forbids a configure-time download, so this file only locates a staged ANGLE
# and describes it to CMake. docs/build.md holds the pinned revision and the
# CI job fragment that stages it under $SMZ_DEPS_DIR.
#
# ANGLE exposes the standard EGL and GLES entry points, so the runtime and the
# probe include the same Khronos headers whichever provider is selected; the
# switch changes which libraries those symbols resolve to.
#
# Any problem is a configure-time FATAL_ERROR naming SMZ_ANGLE_DIR.

# The ANGLE revision smzuggler builds against: Chrome 136 stable (136.0.7103.113),
# the Chromium release matching the V8 13.6 pin in cmake/V8.cmake. Pinning the
# release's own ANGLE commit makes the CI oracle reproduce the browser that
# WebGL2 has to match.
set(SMZ_ANGLE_PIN "fa40b7c586fd2da9fd7e5c4d893ecb1334553b9e")
set(SMZ_ANGLE_BRANCH "chromium/7103")
set(SMZ_ANGLE_VERSION "Chrome 136.0.7103.113")

# Finds one ANGLE library by name, preferring the staged install and never
# falling back to an unrelated system copy.
#
# Why: a system EGL/GLES (Mesa) on the search path would otherwise satisfy the
# request and the oracle would silently measure the wrong provider. `dir` is
# the staged prefix, `basename` is the library name without prefix/suffix, and
# `out_var` receives the absolute library path.
function(_smz_angle_find_library dir basename out_var)
    find_library(${out_var}
        NAMES "${basename}" "lib${basename}"
        PATHS "${dir}" "${dir}/lib" "${dir}/lib64" "${dir}/bin"
        NO_DEFAULT_PATH)
    if(NOT ${out_var})
        message(FATAL_ERROR
            "SMZ_ANGLE_DIR=${dir} has no ${basename} library. Build the pinned "
            "ANGLE ${SMZ_ANGLE_VERSION} (${SMZ_ANGLE_BRANCH}, commit "
            "${SMZ_ANGLE_PIN}) and stage include/ plus lib/ under one prefix, "
            "or point SMZ_ANGLE_DIR at that prefix (docs/build.md).")
    endif()
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

# Populates the `smz_gles` interface target from SMZ_ANGLE_DIR.
#
# Why: MZ_GLES=angle is an explicit request, so a missing or incomplete install
# must stop the configure with the variable to fix rather than link a binary
# that cannot start. `target` is the interface target; the result is returned
# through SMZ_GLES_AVAILABLE in the caller. Header lookup falls back to the
# system Khronos headers because ANGLE installs often ship libraries only, and
# its public EGL/GLES declarations are the standard ones.
function(smz_angle_provider target)
    if(NOT SMZ_ANGLE_DIR)
        message(FATAL_ERROR
            "MZ_GLES=angle needs SMZ_ANGLE_DIR, the prefix holding the pinned "
            "ANGLE ${SMZ_ANGLE_VERSION} (${SMZ_ANGLE_BRANCH}, commit "
            "${SMZ_ANGLE_PIN}) with include/ and lib/. See docs/build.md for "
            "the staged build used by the Linux CI oracle.")
    endif()
    if(NOT IS_DIRECTORY "${SMZ_ANGLE_DIR}")
        message(FATAL_ERROR
            "SMZ_ANGLE_DIR=${SMZ_ANGLE_DIR} is not a directory (docs/build.md).")
    endif()

    _smz_angle_find_library("${SMZ_ANGLE_DIR}" EGL SMZ_ANGLE_EGL_LIBRARY)
    _smz_angle_find_library("${SMZ_ANGLE_DIR}" GLESv2 SMZ_ANGLE_GLES_LIBRARY)

    find_path(SMZ_ANGLE_EGL_INCLUDE_DIR NAMES EGL/egl.h
        PATHS "${SMZ_ANGLE_DIR}/include" NO_DEFAULT_PATH)
    find_path(SMZ_ANGLE_GLES_INCLUDE_DIR NAMES GLES3/gl3.h
        PATHS "${SMZ_ANGLE_DIR}/include" NO_DEFAULT_PATH)
    if(NOT SMZ_ANGLE_EGL_INCLUDE_DIR)
        find_path(SMZ_ANGLE_EGL_INCLUDE_DIR NAMES EGL/egl.h)
    endif()
    if(NOT SMZ_ANGLE_GLES_INCLUDE_DIR)
        find_path(SMZ_ANGLE_GLES_INCLUDE_DIR NAMES GLES3/gl3.h)
    endif()
    if(NOT SMZ_ANGLE_EGL_INCLUDE_DIR OR NOT SMZ_ANGLE_GLES_INCLUDE_DIR)
        message(FATAL_ERROR
            "SMZ_ANGLE_DIR=${SMZ_ANGLE_DIR} has no EGL/GLES headers, and this "
            "host has no system Khronos headers to fall back to. Stage ANGLE's "
            "include/ tree in the prefix (docs/build.md).")
    endif()

    target_include_directories(${target} INTERFACE
        "${SMZ_ANGLE_EGL_INCLUDE_DIR}" "${SMZ_ANGLE_GLES_INCLUDE_DIR}")
    target_link_libraries(${target} INTERFACE
        "${SMZ_ANGLE_EGL_LIBRARY}" "${SMZ_ANGLE_GLES_LIBRARY}")
    set(SMZ_GLES_AVAILABLE TRUE PARENT_SCOPE)
endfunction()
