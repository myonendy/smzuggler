# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Imports the prebuilt, pinned V8 static monolith as the target `smz::v8`.
#
# Why: smzuggler replaces QuickJS with V8 (spec §4.1), and V8 is never built
# by CMake: tools/v8/build_v8.sh (or the CI tarball) installs it once into
# $SMZ_DEPS_DIR/v8/<version>/ (docs/build-v8.md). This file only checks that
# install and describes it to CMake, so every target that links `smz::v8`
# gets the same headers, defines and link flags.
#
# Rules: the defines below must match the GN args of build_v8.sh (headers
# compiled without them disagree with the library on object layout), and
# SMZ_V8_VERSION_PIN must equal V8_VERSION in build_v8.sh
# (tests/tools/test_v8_cmake.py checks both). Any problem is a configure-time
# FATAL_ERROR naming SMZ_V8_DIR; nothing here ever downloads.

# The V8 version smzuggler is built and tested against (DEC-M1-T001).
set(SMZ_V8_VERSION_PIN "13.6.233.8")

# Reads the version of the V8 headers under `dir` into `out_var`.
#
# Why: the version is taken from include/v8-version.h, not from the install's
# manifest, so a hand-assembled or stale directory cannot claim the pin.
# `dir` is the install prefix; `out_var` receives "major.minor.build.patch".
# Fails the configure if the header or one of its four numbers is missing.
function(_smz_v8_read_version dir out_var)
    set(header "${dir}/include/v8-version.h")
    if(NOT EXISTS "${header}")
        message(FATAL_ERROR
            "SMZ_V8_DIR=${dir} has no include/v8-version.h; it must point at a "
            "V8 install from tools/v8/build_v8.sh, i.e. "
            "$SMZ_DEPS_DIR/v8/${SMZ_V8_VERSION_PIN} (docs/build-v8.md).")
    endif()
    file(READ "${header}" text)
    set(parts "")
    foreach(field MAJOR_VERSION MINOR_VERSION BUILD_NUMBER PATCH_LEVEL)
        if(NOT text MATCHES "#define V8_${field} ([0-9]+)")
            message(FATAL_ERROR
                "SMZ_V8_DIR=${dir}: ${header} does not define V8_${field}.")
        endif()
        list(APPEND parts "${CMAKE_MATCH_1}")
    endforeach()
    list(JOIN parts "." version)
    set(${out_var} "${version}" PARENT_SCOPE)
endfunction()

# Fails the configure unless SMZ_V8_DIR holds the pinned V8 install.
#
# Why: a missing or wrong V8 otherwise surfaces as hundreds of compile or
# link errors far from the cause; checking here names the variable to fix.
# Reads the cache variable SMZ_V8_DIR; returns nothing.
function(_smz_v8_check_dir)
    if(NOT SMZ_V8_DIR)
        message(FATAL_ERROR
            "SMZ_JS_ENGINE=v8 needs SMZ_V8_DIR, the V8 install prefix. Build "
            "it with tools/v8/build_v8.sh and pass "
            "-DSMZ_V8_DIR=$SMZ_DEPS_DIR/v8/${SMZ_V8_VERSION_PIN} "
            "(docs/build-v8.md).")
    endif()
    if(NOT IS_DIRECTORY "${SMZ_V8_DIR}")
        message(FATAL_ERROR
            "SMZ_V8_DIR=${SMZ_V8_DIR} is not a directory (docs/build-v8.md).")
    endif()
    _smz_v8_read_version("${SMZ_V8_DIR}" found)
    if(NOT found STREQUAL SMZ_V8_VERSION_PIN)
        message(FATAL_ERROR
            "SMZ_V8_DIR=${SMZ_V8_DIR} holds V8 ${found}, but smzuggler is "
            "pinned to V8 ${SMZ_V8_VERSION_PIN}. Point SMZ_V8_DIR at "
            "$SMZ_DEPS_DIR/v8/${SMZ_V8_VERSION_PIN} (docs/build-v8.md).")
    endif()
    foreach(file lib/libv8_monolith.a share/icudtl.dat)
        if(NOT EXISTS "${SMZ_V8_DIR}/${file}")
            message(FATAL_ERROR
                "SMZ_V8_DIR=${SMZ_V8_DIR} has the V8 ${found} headers but no "
                "${file}; the install is incomplete. Re-run "
                "tools/v8/build_v8.sh (docs/build-v8.md).")
        endif()
    endforeach()
endfunction()

# Checks SMZ_V8_DIR and defines the imported static target `smz::v8`.
#
# Why: one place says how to compile and link against V8. Consumers need
# C++20 (V8 13.x headers require it), the pointer-compression defines that
# match `v8_enable_pointer_compression = true`, threads and libdl, and lld:
# V8's clang emits CREL relocations that GNU ld cannot read
# (DEC-M1-T001-SYSROOT), so a missing ld.lld fails the configure. Also sets
# SMZ_V8_ICU_DATA (path of icudtl.dat, which V8 needs at startup because
# `icu_use_data_file = true`) in the caller's scope. Safe to call twice.
function(smz_import_v8)
    _smz_v8_check_dir()
    set(SMZ_V8_ICU_DATA "${SMZ_V8_DIR}/share/icudtl.dat" PARENT_SCOPE)
    if(TARGET smz::v8)
        return()
    endif()
    find_program(SMZ_LD_LLD NAMES ld.lld)
    if(NOT SMZ_LD_LLD)
        message(FATAL_ERROR
            "SMZ_JS_ENGINE=v8 links with lld (-fuse-ld=lld) because the V8 "
            "monolith uses CREL relocations that GNU ld cannot read, but "
            "ld.lld was not found. Install lld (Arch, Debian, Ubuntu and "
            "Fedora package: lld) (docs/build-v8.md).")
    endif()
    find_package(Threads REQUIRED)
    add_library(smz::v8 STATIC IMPORTED GLOBAL)
    set_target_properties(smz::v8 PROPERTIES
        IMPORTED_LOCATION "${SMZ_V8_DIR}/lib/libv8_monolith.a"
        INTERFACE_INCLUDE_DIRECTORIES "${SMZ_V8_DIR}/include"
        INTERFACE_COMPILE_DEFINITIONS
            "V8_COMPRESS_POINTERS;V8_31BIT_SMIS_ON_64BIT_ARCH"
        INTERFACE_COMPILE_FEATURES cxx_std_20
        INTERFACE_LINK_LIBRARIES "Threads::Threads;${CMAKE_DL_LIBS}"
        INTERFACE_LINK_OPTIONS "-fuse-ld=lld")
    message(STATUS "smzuggler: V8 ${SMZ_V8_VERSION_PIN} from ${SMZ_V8_DIR}")
endfunction()
