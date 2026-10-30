# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 The smzuggler contributors
# Register the identity contract without creating platform resources.
# See docs/DECISIONS.md (DEC-M0-T001); inherit the configured build identity.
add_executable(test_names ${CMAKE_CURRENT_LIST_DIR}/test_names.c)
target_include_directories(test_names PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_compile_definitions(test_names PRIVATE
    EXPECTED_NAME="${SMZUGGLER_NAME}"
    EXPECTED_BIN="${SMZUGGLER_BIN}"
)
add_test(NAME names COMMAND test_names)
