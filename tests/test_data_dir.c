/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 */

/* Resolved log directory for opt-in --data-dir (main.c). */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_paths.h"

#ifndef RMMZ_DATA_APP
#define RMMZ_DATA_APP "outsider"
#endif

bool rmmz_resolve_log_dir(bool use_data_dir, const char *data_dir_path,
                          const char *game_dir, char *out, size_t out_size);

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("  TEST %s ... ", #name); \
    } while (0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while (0)

#define FAIL(msg) \
    do { \
        printf("FAIL: %s\n", msg); \
    } while (0)

static void test_setenv(const char *name, const char *value)
{
#ifdef _WIN32
    _putenv_s(name, value ? value : "");
#else
    if (value) setenv(name, value, 1);
    else unsetenv(name);
#endif
}

static char *test_getenv_copy(const char *name)
{
    const char *v = getenv(name);
    char *copy;
    if (!v) return NULL;
    copy = malloc(strlen(v) + 1);
    if (copy) memcpy(copy, v, strlen(v) + 1);
    return copy;
}

static void test_restore_env(const char *name, char *saved)
{
    test_setenv(name, saved);
    free(saved);
}

static void expect_path(const char *got, const char *want)
{
    char msg[TEST_PATH_MAX * 2];
    if (got && want && strcmp(got, want) == 0) {
        PASS();
        return;
    }
    snprintf(msg, sizeof(msg), "got \"%s\" want \"%s\"",
             got ? got : "(null)", want ? want : "(null)");
    FAIL(msg);
}

static void test_without_flag(void)
{
    char out[TEST_PATH_MAX];
    const char *game = test_tmp_path("rmmz_data_dir_game");

    TEST(without_flag_empty_without_game);
    if (rmmz_resolve_log_dir(false, NULL, NULL, out, sizeof(out)) &&
        out[0] == '\0') {
        PASS();
    } else {
        FAIL(out);
    }

    TEST(without_flag_uses_game_dir);
    if (!rmmz_resolve_log_dir(false, NULL, game, out, sizeof(out))) {
        FAIL("resolve failed");
    } else {
        expect_path(out, game);
    }
}

static void test_with_path(void)
{
    char out[TEST_PATH_MAX];
    const char *game = test_tmp_path("rmmz_data_dir_game");
    const char *path = test_tmp_path("rmmz_data_dir_explicit");

    TEST(with_path_uses_path);
    if (!rmmz_resolve_log_dir(true, path, game, out, sizeof(out))) {
        FAIL("resolve failed");
    } else {
        expect_path(out, path);
    }
}

#if defined(__linux__) && !defined(__ANDROID__)
static void test_with_flag_xdg(void)
{
    char out[TEST_PATH_MAX];
    char expect[TEST_PATH_MAX];
    char *saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    const char *state = test_tmp_path("rmmz_data_dir_state");
    const char *game = test_tmp_path("rmmz_data_dir_game");

    test_setenv("XDG_STATE_HOME", state);
    snprintf(expect, sizeof(expect), "%s/%s", state, RMMZ_DATA_APP);

    TEST(flag_uses_xdg_state_home);
    if (!rmmz_resolve_log_dir(true, NULL, game, out, sizeof(out))) {
        FAIL("resolve failed");
    } else {
        expect_path(out, expect);
    }

    test_restore_env("XDG_STATE_HOME", saved_xdg);
}

static void test_xdg_unset(void)
{
    char out[TEST_PATH_MAX];
    char expect[TEST_PATH_MAX];
    char home[TEST_PATH_MAX];
    char *saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    char *saved_home = test_getenv_copy("HOME");
    const char *game = test_tmp_path("rmmz_data_dir_game");

    snprintf(home, sizeof(home), "%s/rmmz_data_dir_home", test_tmp_root());
    test_setenv("XDG_STATE_HOME", NULL);
    test_setenv("HOME", home);
    snprintf(expect, sizeof(expect), "%s/.local/state/%s", home, RMMZ_DATA_APP);

    TEST(xdg_unset_uses_home_local_state);
    if (!rmmz_resolve_log_dir(true, NULL, game, out, sizeof(out))) {
        FAIL("resolve failed");
    } else {
        expect_path(out, expect);
    }

    test_restore_env("XDG_STATE_HOME", saved_xdg);
    test_restore_env("HOME", saved_home);
}
#endif

int main(void)
{
    printf("=== Data-dir log path tests ===\n");
    test_without_flag();
    test_with_path();
#if defined(__linux__) && !defined(__ANDROID__)
    test_with_flag_xdg();
    test_xdg_unset();
#endif
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
