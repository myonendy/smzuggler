/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 */

/* Resolved log directory for opt-in --data-dir (main.c). */

#include <assert.h>
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

#define ASSERT(cond) \
    do { \
        if (!(cond)) { \
            printf("FAIL\n    assertion failed: %s\n    at %s:%d\n", \
                   #cond, __FILE__, __LINE__); \
            return; \
        } \
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

static void test_without_flag(void)
{
    char out[TEST_PATH_MAX];
    const char *game = test_tmp_path("rmmz_data_dir_game");

    TEST(without_flag_empty_without_game);
    ASSERT(rmmz_resolve_log_dir(false, NULL, NULL, out, sizeof(out)));
    ASSERT(out[0] == '\0');
    assert(out[0] == '\0');
    PASS();

    TEST(without_flag_uses_game_dir);
    ASSERT(rmmz_resolve_log_dir(false, NULL, game, out, sizeof(out)));
    ASSERT(strcmp(out, game) == 0);
    assert(strcmp(out, game) == 0);
    PASS();
}

static void test_with_path(void)
{
    char out[TEST_PATH_MAX];
    const char *game = test_tmp_path("rmmz_data_dir_game");
    const char *path = test_tmp_path("rmmz_data_dir_explicit");

    TEST(with_path_uses_path);
    ASSERT(rmmz_resolve_log_dir(true, path, game, out, sizeof(out)));
    ASSERT(strcmp(out, path) == 0);
    assert(strcmp(out, path) == 0);
    PASS();
}

static void test_with_flag_not_game_dir(void)
{
    char out[TEST_PATH_MAX];
    const char *game = test_tmp_path("rmmz_data_dir_game");

    TEST(flag_without_path_not_game_dir);
    ASSERT(rmmz_resolve_log_dir(true, NULL, game, out, sizeof(out)));
    ASSERT(out[0] != '\0');
    ASSERT(strcmp(out, game) != 0);
    assert(strcmp(out, game) != 0);
    PASS();
}

#if defined(__linux__) && !defined(__ANDROID__)
static void test_with_flag_xdg(void)
{
    char out[TEST_PATH_MAX];
    char expect[TEST_PATH_MAX];
    char *saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    const char *state = test_tmp_path("rmmz_data_dir_state");
    const char *game = test_tmp_path("rmmz_data_dir_game");
    const char *path = test_tmp_path("rmmz_data_dir_explicit");
    int ok;

    test_setenv("XDG_STATE_HOME", state);
    snprintf(expect, sizeof(expect), "%s/%s", state, RMMZ_DATA_APP);
    ok = rmmz_resolve_log_dir(true, NULL, game, out, sizeof(out));
    test_restore_env("XDG_STATE_HOME", saved_xdg);

    TEST(flag_uses_xdg_state_home);
    ASSERT(ok);
    ASSERT(strcmp(out, expect) == 0);
    assert(strcmp(out, expect) == 0);
    PASS();

    /* PATH wins over XDG_STATE_HOME when both are present. */
    saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    test_setenv("XDG_STATE_HOME", state);
    ok = rmmz_resolve_log_dir(true, path, game, out, sizeof(out));
    test_restore_env("XDG_STATE_HOME", saved_xdg);

    TEST(path_wins_over_xdg);
    ASSERT(ok);
    ASSERT(strcmp(out, path) == 0);
    assert(strcmp(out, path) == 0);
    PASS();

    /* Without the flag, XDG must not move logs out of the game dir. */
    saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    test_setenv("XDG_STATE_HOME", state);
    ok = rmmz_resolve_log_dir(false, NULL, game, out, sizeof(out));
    test_restore_env("XDG_STATE_HOME", saved_xdg);

    TEST(without_flag_ignores_xdg);
    ASSERT(ok);
    ASSERT(strcmp(out, game) == 0);
    assert(strcmp(out, game) == 0);
    PASS();
}

static void test_xdg_unset(void)
{
    char out[TEST_PATH_MAX];
    char expect[TEST_PATH_MAX];
    char home[TEST_PATH_MAX];
    char *saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    char *saved_home = test_getenv_copy("HOME");
    const char *game = test_tmp_path("rmmz_data_dir_game");
    int ok;

    snprintf(home, sizeof(home), "%s/rmmz_data_dir_home", test_tmp_root());
    test_setenv("XDG_STATE_HOME", NULL);
    test_setenv("HOME", home);
    snprintf(expect, sizeof(expect), "%s/.local/state/%s", home, RMMZ_DATA_APP);
    ok = rmmz_resolve_log_dir(true, NULL, game, out, sizeof(out));
    test_restore_env("XDG_STATE_HOME", saved_xdg);
    test_restore_env("HOME", saved_home);

    TEST(xdg_unset_uses_home_local_state);
    ASSERT(ok);
    ASSERT(strcmp(out, expect) == 0);
    assert(strcmp(out, expect) == 0);
    PASS();
}
#endif

int main(void)
{
    printf("=== Data-dir log path tests ===\n");
    test_without_flag();
    test_with_path();
    test_with_flag_not_game_dir();
#if defined(__linux__) && !defined(__ANDROID__)
    test_with_flag_xdg();
    test_xdg_unset();
#endif
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
