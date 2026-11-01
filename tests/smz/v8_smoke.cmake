# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Register the V8 smoke test, built only when SMZ_JS_ENGINE=v8 (spec section
# 4.1). It proves smz::v8 compiles, links and runs, and that the linked V8 is
# the pin from cmake/V8.cmake. See docs/build-v8.md.
if(SMZ_JS_ENGINE STREQUAL "v8")
    add_executable(test_v8_smoke ${CMAKE_CURRENT_LIST_DIR}/test_v8_smoke.cc)
    target_link_libraries(test_v8_smoke PRIVATE smz::v8)
    target_compile_definitions(test_v8_smoke PRIVATE
        SMZ_V8_EXPECTED_VERSION="${SMZ_V8_VERSION_PIN}"
        SMZ_V8_ICU_DATA_PATH="${SMZ_V8_ICU_DATA}"
    )
    add_test(NAME v8_smoke COMMAND test_v8_smoke)
endif()
