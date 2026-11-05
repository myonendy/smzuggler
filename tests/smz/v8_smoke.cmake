# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Modified 2026-11-05: register V8 coverage without the removed engine.
# Runtime binding checks; see docs/js-binding.md.
add_executable(test_v8_smoke ${CMAKE_CURRENT_LIST_DIR}/test_v8_smoke.cc)
target_link_libraries(test_v8_smoke PRIVATE smz::v8)
target_compile_definitions(test_v8_smoke PRIVATE
    SMZ_V8_EXPECTED_VERSION="${SMZ_V8_VERSION_PIN}"
    SMZ_V8_ICU_DATA_PATH="${SMZ_V8_ICU_DATA}"
    )
add_test(NAME v8_smoke COMMAND test_v8_smoke)
