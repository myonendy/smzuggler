# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
#
# Provider probe for the MZ_GLES build switch (spec §4.2).
#
# Why: both providers export the same EGL/GLES symbols, so only running a
# binary built against the selected provider proves the switch reached it.
# The probe is a test, not part of the runtime: a host that lacks the selected
# provider reports that at configure time and this fragment registers nothing,
# so the other platform's build is not failed by a graphics stack it does not
# use (docs/build.md).
if(SMZ_GLES_AVAILABLE)
    add_executable(test_angle_switch ${CMAKE_CURRENT_LIST_DIR}/test_angle_switch.c)
    if(MSVC)
        target_compile_options(test_angle_switch PRIVATE /UNDEBUG)
    else()
        target_compile_options(test_angle_switch PRIVATE -UNDEBUG)
    endif()
    target_link_libraries(test_angle_switch PRIVATE smz_gles ${RMMZ_SYS_LIBS})
    add_test(NAME angle_switch COMMAND test_angle_switch)
else()
    message(STATUS
        "angle_switch: not built; MZ_GLES=${MZ_GLES} is unavailable on this "
        "host (docs/build.md)")
endif()
