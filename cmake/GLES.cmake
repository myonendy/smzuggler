# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Selects the OpenGL ES provider behind the WebGL2 stack (spec §4.2).
#
# Why: WebGL2 is GLES 3.0 plus validation, so the runtime can reach the GPU
# through the platform's own EGL/GLES (Linux, Steam Deck, Android) or through
# ANGLE, which maps the same API to D3D11 on Windows and Metal on macOS.
# Chrome itself uses ANGLE, so the Linux CI oracle builds with ANGLE too while
# the measured Linux targets keep system Mesa. Q5 (ANGLE everywhere versus
# system GL on Linux) is still open; this file only provides the switch.
#
# Providing `smz_gles` here, rather than editing every consumer, means the
# WebGL2 stack links one target whatever the selected provider is. When the
# chosen provider is not installed on this host, the target is left unusable
# and `SMZ_GLES_AVAILABLE` is FALSE, so optional probes can be skipped instead
# of failing a platform that will use the other provider.

# The provider is a cache choice, validated before anything links it so a typo
# cannot silently fall back to a different graphics stack.
set(MZ_GLES "system" CACHE STRING "OpenGL ES provider: system (EGL/GLES) or angle")
set_property(CACHE MZ_GLES PROPERTY STRINGS system angle)
if(NOT MZ_GLES MATCHES "^(system|angle)$")
    message(FATAL_ERROR
        "MZ_GLES must be system or angle, not '${MZ_GLES}'. "
        "system uses the platform EGL/GLES; angle uses an installed ANGLE "
        "(docs/build.md).")
endif()

set(SMZ_GLES_AVAILABLE FALSE)
set(SMZ_GLES_PROVIDER_DESC "not resolved")

add_library(smz_gles INTERFACE)

# Resolves the platform EGL/GLES libraries for the `smz_gles` target.
#
# Why: Linux, Steam Deck and Android expose GLES 3.x through EGL, and the
# measured §14 targets use exactly that path. A host without the headers (a
# macOS or Windows machine, where GLES only exists through ANGLE) is not an
# error: it reports the condition and leaves SMZ_GLES_AVAILABLE FALSE so the
# ANGLE provider can be selected instead. `target` is the interface target to
# populate; the result is returned through SMZ_GLES_AVAILABLE in the caller.
function(_smz_gles_system_provider target)
    find_path(SMZ_EGL_INCLUDE_DIR NAMES EGL/egl.h)
    find_path(SMZ_GLES_INCLUDE_DIR NAMES GLES3/gl3.h)
    find_library(SMZ_EGL_LIBRARY NAMES EGL libEGL)
    find_library(SMZ_GLES_LIBRARY NAMES GLESv2 libGLESv2)
    if(NOT SMZ_EGL_INCLUDE_DIR OR NOT SMZ_GLES_INCLUDE_DIR
            OR NOT SMZ_EGL_LIBRARY OR NOT SMZ_GLES_LIBRARY)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            message(FATAL_ERROR
                "MZ_GLES=system needs the EGL and GLESv2 development files, "
                "which are missing on this Linux host. Install them (Arch "
                "libglvnd + mesa, Debian/Ubuntu libegl1-mesa-dev and "
                "libgles2-mesa-dev, Fedora mesa-libEGL-devel and "
                "mesa-libGLES-devel) or select -DMZ_GLES=angle (docs/build.md).")
        endif()
        message(STATUS
            "MZ_GLES=system: no platform EGL/GLES on ${CMAKE_SYSTEM_NAME}; "
            "select -DMZ_GLES=angle for the ANGLE provider (docs/build.md).")
        set(SMZ_GLES_AVAILABLE FALSE PARENT_SCOPE)
        return()
    endif()

    target_include_directories(${target} INTERFACE
        "${SMZ_EGL_INCLUDE_DIR}" "${SMZ_GLES_INCLUDE_DIR}")
    target_link_libraries(${target} INTERFACE
        "${SMZ_EGL_LIBRARY}" "${SMZ_GLES_LIBRARY}")
    set(SMZ_GLES_AVAILABLE TRUE PARENT_SCOPE)
endfunction()

# Populates the interface target and the availability flag for MZ_GLES.
#
# Why: consumers and the provider probe must see the same answer, and the
# compile definition records the choice for diagnostics (the probe prints it
# and asserts the matching EGL client vendor). The `target` argument is the
# `smz_gles` interface target; it is populated only when the provider exists.
function(_smz_gles_select target)
    if(MZ_GLES STREQUAL "angle")
        include("${CMAKE_CURRENT_LIST_DIR}/ANGLE.cmake")
        smz_angle_provider("${target}")
        if(SMZ_GLES_AVAILABLE)
            target_compile_definitions(${target} INTERFACE SMZ_GLES_ANGLE=1)
            set(SMZ_GLES_PROVIDER_DESC "ANGLE ${SMZ_ANGLE_PIN}" PARENT_SCOPE)
        endif()
    else()
        _smz_gles_system_provider("${target}")
        if(SMZ_GLES_AVAILABLE)
            target_compile_definitions(${target} INTERFACE SMZ_GLES_SYSTEM=1)
            set(SMZ_GLES_PROVIDER_DESC "system EGL/GLES" PARENT_SCOPE)
        endif()
    endif()
    set(SMZ_GLES_AVAILABLE "${SMZ_GLES_AVAILABLE}" PARENT_SCOPE)
endfunction()

_smz_gles_select(smz_gles)
if(SMZ_GLES_AVAILABLE)
    message(STATUS "MZ_GLES=${MZ_GLES} (${SMZ_GLES_PROVIDER_DESC})")
endif()
