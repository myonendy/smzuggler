/*
 * Copyright (c) 2026 General Arcade (Pte. Ltd.)
 * SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-GeneralArcade-Commercial
 */

/* Resolved log directory for opt-in --data-dir / --data-dir=PATH (main.c). */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test_paths.h"

#ifndef RMMZ_DATA_APP
#define RMMZ_DATA_APP "outsider"
#endif

bool rmmz_resolve_log_dir(int argc, char **argv, const char *game_dir,
                          char *out, size_t out_size);

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                       \
    do {                                 \
        tests_run++;                     \
        printf("  TEST %s ... ", #name); \
    } while (0)

#define PASS()            \
    do {                  \
        tests_passed++;   \
        printf("PASS\n"); \
    } while (0)

#define FAIL(msg)                  \
    do {                           \
        printf("FAIL: %s\n", msg); \
    } while (0)

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

static int resolve(char **argv, int argc, const char *game, char *out, size_t n)
{
    return rmmz_resolve_log_dir(argc, argv, game, out, n) ? 1 : 0;
}

static void test_without_flag(void)
{
    char out[TEST_PATH_MAX];
    char *argv_plain[] = {"rt"};
    char *argv_game[] = {"rt", "--game", NULL};
    const char *game = test_tmp_path("rmmz_data_dir_game");

    argv_game[2] = (char *)game;

    TEST(without_flag_empty_without_game);
    if (resolve(argv_plain, 1, NULL, out, sizeof(out)) && out[0] == '\0') {
        PASS();
    } else {
        FAIL(out);
    }

    TEST(without_flag_uses_game_dir);
    if (!resolve(argv_game, 3, game, out, sizeof(out))) {
        FAIL("resolve failed");
    } else {
        expect_path(out, game);
    }

    TEST(without_flag_ignores_other_options);
    {
        char *argv_other[] = {"rt", "--no-audio", "--game", NULL};
        argv_other[3] = (char *)game;
        if (!resolve(argv_other, 4, game, out, sizeof(out))) {
            FAIL("resolve failed");
        } else {
            expect_path(out, game);
        }
    }
}

static void test_with_path(void)
{
    char out[TEST_PATH_MAX];
    char flag[TEST_PATH_MAX];
    char *argv_eq[] = {"rt", flag};
    const char *game = test_tmp_path("rmmz_data_dir_game");
    const char *path = test_tmp_path("rmmz_data_dir_explicit");

    snprintf(flag, sizeof(flag), "--data-dir=%s", path);

    TEST(with_path_uses_path);
    if (!resolve(argv_eq, 2, game, out, sizeof(out))) {
        FAIL("resolve failed");
    } else {
        expect_path(out, path);
    }

    snprintf(flag, sizeof(flag), "--data-dir=%s/", path);
    TEST(with_path_trims_trailing_slash);
    if (!resolve(argv_eq, 2, game, out, sizeof(out))) {
        FAIL("resolve failed");
    } else {
        expect_path(out, path);
    }
}

static void test_with_flag_not_game_dir(void)
{
    char out[TEST_PATH_MAX];
    char *argv_flag[] = {"rt", "--data-dir"};
    const char *game = test_tmp_path("rmmz_data_dir_game");

    TEST(flag_without_path_not_game_dir);
    if (!resolve(argv_flag, 2, game, out, sizeof(out))) {
        FAIL("resolve failed");
    } else if (out[0] == '\0' || strcmp(out, game) == 0) {
        FAIL("flag used the game dir");
    } else {
        PASS();
    }
}

#if defined(__linux__) && !defined(__ANDROID__)
static void test_with_flag_xdg(void)
{
    char out[TEST_PATH_MAX];
    char expect[TEST_PATH_MAX];
    char flag[TEST_PATH_MAX];
    char *saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    char *argv_flag[] = {"rt", "--data-dir"};
    char *argv_eq[] = {"rt", flag};
    char *argv_plain[] = {"rt"};
    const char *state = test_tmp_path("rmmz_data_dir_state");
    const char *game = test_tmp_path("rmmz_data_dir_game");
    const char *path = test_tmp_path("rmmz_data_dir_explicit");
    int ok;

    test_setenv("XDG_STATE_HOME", state);
    snprintf(expect, sizeof(expect), "%s/%s", state, RMMZ_DATA_APP);
    ok = resolve(argv_flag, 2, game, out, sizeof(out));
    test_restore_env("XDG_STATE_HOME", saved_xdg);

    TEST(flag_uses_xdg_state_home);
    if (!ok) {
        FAIL("resolve failed");
    } else {
        expect_path(out, expect);
    }

    /* PATH wins over XDG_STATE_HOME when both are present. */
    snprintf(flag, sizeof(flag), "--data-dir=%s", path);
    saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    test_setenv("XDG_STATE_HOME", state);
    ok = resolve(argv_eq, 2, game, out, sizeof(out));
    test_restore_env("XDG_STATE_HOME", saved_xdg);

    TEST(path_wins_over_xdg);
    if (!ok) {
        FAIL("resolve failed");
    } else {
        expect_path(out, path);
    }

    /* Without the flag, XDG must not move logs out of the game dir. */
    saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    test_setenv("XDG_STATE_HOME", state);
    ok = resolve(argv_plain, 1, game, out, sizeof(out));
    test_restore_env("XDG_STATE_HOME", saved_xdg);

    TEST(without_flag_ignores_xdg);
    if (!ok) {
        FAIL("resolve failed");
    } else {
        expect_path(out, game);
    }
}

static void test_xdg_unset(void)
{
    char out[TEST_PATH_MAX];
    char expect[TEST_PATH_MAX];
    char home[TEST_PATH_MAX];
    char *saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    char *saved_home = test_getenv_copy("HOME");
    char *argv_flag[] = {"rt", "--data-dir"};
    const char *game = test_tmp_path("rmmz_data_dir_game");
    int ok;

    snprintf(home, sizeof(home), "%s/rmmz_data_dir_home", test_tmp_root());
    test_setenv("XDG_STATE_HOME", NULL);
    test_setenv("HOME", home);
    snprintf(expect, sizeof(expect), "%s/.local/state/%s", home, RMMZ_DATA_APP);
    ok = resolve(argv_flag, 2, game, out, sizeof(out));
    test_restore_env("XDG_STATE_HOME", saved_xdg);
    test_restore_env("HOME", saved_home);

    TEST(xdg_unset_uses_home_local_state);
    if (!ok) {
        FAIL("resolve failed");
    } else {
        expect_path(out, expect);
    }
}

static void test_last_flag_wins(void)
{
    char out[TEST_PATH_MAX];
    char first[TEST_PATH_MAX];
    char second[TEST_PATH_MAX];
    char *argv_path_then_flag[] = {"rt", first, "--data-dir"};
    char *argv_flag_then_path[] = {"rt", "--data-dir", second};
    const char *game = test_tmp_path("rmmz_data_dir_game");
    const char *path = test_tmp_path("rmmz_data_dir_explicit");
    char *saved_xdg = test_getenv_copy("XDG_STATE_HOME");
    const char *state = test_tmp_path("rmmz_data_dir_state");
    char expect_state[TEST_PATH_MAX];
    int ok;

    snprintf(first, sizeof(first), "--data-dir=%s", path);
    snprintf(second, sizeof(second), "--data-dir=%s", path);
    snprintf(expect_state, sizeof(expect_state), "%s/%s", state, RMMZ_DATA_APP);

    test_setenv("XDG_STATE_HOME", state);
    ok = resolve(argv_path_then_flag, 3, game, out, sizeof(out));
    test_restore_env("XDG_STATE_HOME", saved_xdg);

    TEST(bare_flag_after_path_uses_state_dir);
    if (!ok) {
        FAIL("resolve failed");
    } else {
        expect_path(out, expect_state);
    }

    TEST(path_after_bare_flag_uses_path);
    if (!resolve(argv_flag_then_path, 3, game, out, sizeof(out))) {
        FAIL("resolve failed");
    } else {
        expect_path(out, path);
    }
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
    test_last_flag_wins();
#endif
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
