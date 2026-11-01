/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Resolve host data directories against scratch XDG/HOME values.
 * See docs/data-dirs.md. This process owns its temporary directories.
 */
#include "smzuggler/data_dirs.h"
#include "smzuggler/names.h"
#include "test_paths.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/** Fail without printing private environment contents; test thread only. */
static void check(int ok, const char *what)
{
    if (!ok) fprintf(stderr, "data_dirs: %s\n", what);
    assert(ok);
}

/** Heap copy of getenv, or NULL when unset. Caller frees. */
static char *save_env(const char *name)
{
    const char *v = getenv(name);
    char *copy;
    if (!v) return NULL;
    copy = malloc(strlen(v) + 1);
    check(copy != NULL, "save_env alloc");
    memcpy(copy, v, strlen(v) + 1);
    return copy;
}

/** Restore name from save_env; frees saved. */
static void restore_env(const char *name, char *saved)
{
    if (saved) check(setenv(name, saved, 1) == 0, "restore setenv");
    else check(unsetenv(name) == 0, "restore unsetenv");
    free(saved);
}

/** True when dir contains only . and .. (or does not exist). */
static int dir_is_empty(const char *dir)
{
    DIR *d;
    struct dirent *ent;
    int empty = 1;
    d = opendir(dir);
    if (!d) return 1;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        empty = 0;
        break;
    }
    closedir(d);
    return empty;
}

/** True when path exists on disk. */
static int exists_path(const char *path)
{
    struct stat st;
    return path && path[0] && stat(path, &st) == 0;
}

#if defined(__linux__) && !defined(__ANDROID__)

static char *saved_state, *saved_cache, *saved_config, *saved_home;

/** Snapshot XDG_* and HOME so each case can mutate them. */
static void stash_env(void)
{
    saved_state = save_env("XDG_STATE_HOME");
    saved_cache = save_env("XDG_CACHE_HOME");
    saved_config = save_env("XDG_CONFIG_HOME");
    saved_home = save_env("HOME");
}

/** Put the process environment back; must pair with stash_env. */
static void unstash_env(void)
{
    restore_env("XDG_STATE_HOME", saved_state);
    restore_env("XDG_CACHE_HOME", saved_cache);
    restore_env("XDG_CONFIG_HOME", saved_config);
    restore_env("HOME", saved_home);
    saved_state = saved_cache = saved_config = saved_home = NULL;
}

/** Expect kind to resolve to want; print want on mismatch only (scratch). */
static void expect_dir(SmzDataDirKind kind, const char *game, const char *want)
{
    char out[TEST_PATH_MAX];
    check(smz_data_dir(kind, game, out, sizeof(out)), "resolve");
    if (strcmp(out, want) != 0) {
        fprintf(stderr, "data_dirs: got %s want %s\n", out, want);
        exit(1);
    }
    check(smz_data_dir_usable(out, game), "usable");
    check(!exists_path(out), "resolver created a directory");
}

/** Expect got equals want; print both on mismatch (scratch paths only). */
static void expect_eq(const char *got, const char *want, const char *what)
{
    if (got && want && strcmp(got, want) == 0) return;
    fprintf(stderr, "data_dirs: %s mismatch\n", what);
    if (got) fprintf(stderr, "data_dirs: got %s\n", got);
    if (want) fprintf(stderr, "data_dirs: want %s\n", want);
    exit(1);
}

/** Point XDG_* at scratch bases named after tag. Caller has stash_env. */
static void set_scratch_xdg(const char *tag, char *state, char *cache, char *config)
{
    snprintf(state, TEST_PATH_MAX, "%s/%s-state", test_tmp_root(), tag);
    snprintf(cache, TEST_PATH_MAX, "%s/%s-cache", test_tmp_root(), tag);
    snprintf(config, TEST_PATH_MAX, "%s/%s-config", test_tmp_root(), tag);
    check(setenv("XDG_STATE_HOME", state, 1) == 0, "set state");
    check(setenv("XDG_CACHE_HOME", cache, 1) == 0, "set cache");
    check(setenv("XDG_CONFIG_HOME", config, 1) == 0, "set config");
}

/** Scratch XDG_* bases produce $base/smzuggler-rt for each kind. */
static void test_xdg_set(void)
{
    char state[TEST_PATH_MAX], cache[TEST_PATH_MAX], config[TEST_PATH_MAX];
    char want_state[TEST_PATH_MAX], want_cache[TEST_PATH_MAX], want_config[TEST_PATH_MAX];
    const char *game = test_tmp_path("smz-dd-game");
    snprintf(state, sizeof(state), "%s/smz-dd-state", test_tmp_root());
    snprintf(cache, sizeof(cache), "%s/smz-dd-cache", test_tmp_root());
    snprintf(config, sizeof(config), "%s/smz-dd-config", test_tmp_root());
    snprintf(want_state, sizeof(want_state), "%s/%s", state, SMZUGGLER_BIN);
    snprintf(want_cache, sizeof(want_cache), "%s/%s", cache, SMZUGGLER_BIN);
    snprintf(want_config, sizeof(want_config), "%s/%s", config, SMZUGGLER_BIN);
    stash_env();
    check(setenv("XDG_STATE_HOME", state, 1) == 0, "set state");
    check(setenv("XDG_CACHE_HOME", cache, 1) == 0, "set cache");
    check(setenv("XDG_CONFIG_HOME", config, 1) == 0, "set config");
    expect_dir(SMZ_DATA_DIR_STATE, game, want_state);
    expect_dir(SMZ_DATA_DIR_CACHE, game, want_cache);
    expect_dir(SMZ_DATA_DIR_CONFIG, game, want_config);
    unstash_env();
}

/** Unset XDG_* uses $HOME/.local/state, .cache and .config plus the bin name. */
static void test_xdg_unset(void)
{
    char home[TEST_PATH_MAX];
    char want_state[TEST_PATH_MAX], want_cache[TEST_PATH_MAX], want_config[TEST_PATH_MAX];
    const char *game = test_tmp_path("smz-dd-game");
    snprintf(home, sizeof(home), "%s/smz-dd-home", test_tmp_root());
    snprintf(want_state, sizeof(want_state), "%s/.local/state/%s", home, SMZUGGLER_BIN);
    snprintf(want_cache, sizeof(want_cache), "%s/.cache/%s", home, SMZUGGLER_BIN);
    snprintf(want_config, sizeof(want_config), "%s/.config/%s", home, SMZUGGLER_BIN);
    stash_env();
    check(unsetenv("XDG_STATE_HOME") == 0, "unset state");
    check(unsetenv("XDG_CACHE_HOME") == 0, "unset cache");
    check(unsetenv("XDG_CONFIG_HOME") == 0, "unset config");
    check(setenv("HOME", home, 1) == 0, "set home");
    expect_dir(SMZ_DATA_DIR_STATE, game, want_state);
    expect_dir(SMZ_DATA_DIR_CACHE, game, want_cache);
    expect_dir(SMZ_DATA_DIR_CONFIG, game, want_config);
    unstash_env();
}

/** Empty XDG_* strings are treated as unset (HOME fallback, no cwd writes). */
static void test_xdg_empty(void)
{
    char home[TEST_PATH_MAX], cwd[TEST_PATH_MAX], want[TEST_PATH_MAX];
    char old_cwd[TEST_PATH_MAX];
    const char *game = test_tmp_path("smz-dd-game");
    snprintf(home, sizeof(home), "%s/smz-dd-empty-home", test_tmp_root());
    snprintf(cwd, sizeof(cwd), "%s/smz-dd-empty-cwd", test_tmp_root());
    snprintf(want, sizeof(want), "%s/.local/state/%s", home, SMZUGGLER_BIN);
    check(test_mkdir_p(cwd), "empty cwd");
    check(getcwd(old_cwd, sizeof(old_cwd)) != NULL, "getcwd");
    check(chdir(cwd) == 0, "chdir empty");
    stash_env();
    check(setenv("XDG_STATE_HOME", "", 1) == 0, "empty state");
    check(setenv("XDG_CACHE_HOME", "", 1) == 0, "empty cache");
    check(setenv("XDG_CONFIG_HOME", "", 1) == 0, "empty config");
    check(setenv("HOME", home, 1) == 0, "set home");
    expect_dir(SMZ_DATA_DIR_STATE, game, want);
    check(dir_is_empty(cwd), "empty XDG wrote into cwd");
    unstash_env();
    check(chdir(old_cwd) == 0, "chdir back");
}

/** Relative XDG values fall back to HOME and do not create cwd children. */
static void test_relative_xdg(void)
{
    char home[TEST_PATH_MAX], cwd[TEST_PATH_MAX], want[TEST_PATH_MAX];
    char old_cwd[TEST_PATH_MAX];
    const char *game = test_tmp_path("smz-dd-game");
    snprintf(home, sizeof(home), "%s/smz-dd-rel-home", test_tmp_root());
    snprintf(cwd, sizeof(cwd), "%s/smz-dd-rel-cwd", test_tmp_root());
    snprintf(want, sizeof(want), "%s/.cache/%s", home, SMZUGGLER_BIN);
    check(test_mkdir_p(cwd), "rel cwd");
    check(getcwd(old_cwd, sizeof(old_cwd)) != NULL, "getcwd");
    check(chdir(cwd) == 0, "chdir rel");
    stash_env();
    check(setenv("XDG_CACHE_HOME", "relative-cache", 1) == 0, "rel cache");
    check(setenv("HOME", home, 1) == 0, "set home");
    expect_dir(SMZ_DATA_DIR_CACHE, game, want);
    check(dir_is_empty(cwd), "relative XDG wrote into cwd");
    check(!exists_path("relative-cache"), "relative XDG dir created");
    unstash_env();
    check(chdir(old_cwd) == 0, "chdir back");
}

/** With XDG and HOME both gone, resolve fails and cwd stays empty. */
static void test_no_home(void)
{
    char cwd[TEST_PATH_MAX], out[TEST_PATH_MAX];
    char old_cwd[TEST_PATH_MAX];
    const char *game = test_tmp_path("smz-dd-game");
    snprintf(cwd, sizeof(cwd), "%s/smz-dd-nohome-cwd", test_tmp_root());
    check(test_mkdir_p(cwd), "nohome cwd");
    check(getcwd(old_cwd, sizeof(old_cwd)) != NULL, "getcwd");
    check(chdir(cwd) == 0, "chdir nohome");
    stash_env();
    check(unsetenv("XDG_STATE_HOME") == 0, "unset state");
    check(unsetenv("XDG_CACHE_HOME") == 0, "unset cache");
    check(unsetenv("XDG_CONFIG_HOME") == 0, "unset config");
    check(unsetenv("HOME") == 0, "unset home");
    check(!smz_data_dir(SMZ_DATA_DIR_STATE, game, out, sizeof(out)), "no env");
    check(out[0] == '\0', "failed resolve left a path");
    check(dir_is_empty(cwd), "missing HOME wrote into cwd");
    unstash_env();
    check(chdir(old_cwd) == 0, "chdir back");
}

/** A result under the game root is refused, including XDG pointing at it. */
static void test_game_root(void)
{
    char xdg[TEST_PATH_MAX], sibling[TEST_PATH_MAX], out[TEST_PATH_MAX];
    char want[TEST_PATH_MAX];
    snprintf(xdg, sizeof(xdg), "%s/smz-dd-in-game", test_tmp_root());
    snprintf(sibling, sizeof(sibling), "%s/smz-dd-sibling", test_tmp_root());
    snprintf(want, sizeof(want), "%s/%s", sibling, SMZUGGLER_BIN);
    stash_env();
    check(setenv("XDG_STATE_HOME", xdg, 1) == 0, "set colliding xdg");
    check(!smz_data_dir(SMZ_DATA_DIR_STATE, xdg, out, sizeof(out)),
          "accepted game-root path");
    check(out[0] == '\0', "collision left a path");
    check(setenv("XDG_STATE_HOME", sibling, 1) == 0, "set sibling");
    expect_dir(SMZ_DATA_DIR_STATE, xdg, want);
    check(!smz_data_dir_usable("relative/logs", xdg), "relative usable");
    check(!smz_data_dir_usable("", xdg), "empty usable");
    unstash_env();
}

/** No --data-dir still uses the state dir (default-on), never the game dir. */
static void test_default_on(void)
{
    char state[TEST_PATH_MAX], cache[TEST_PATH_MAX], config[TEST_PATH_MAX];
    char want_state[TEST_PATH_MAX], out[TEST_PATH_MAX];
    char *argv[] = {"rt", "--game", NULL};
    const char *game = test_tmp_path("smz-dd-def-game");
    argv[2] = (char *)game;
    stash_env();
    set_scratch_xdg("smz-dd-def", state, cache, config);
    snprintf(want_state, sizeof(want_state), "%s/%s", state, SMZUGGLER_BIN);
    check(smz_choose_log_dir(3, argv, game, out, sizeof(out)), "default-on");
    expect_eq(out, want_state, "default-on log dir");
    check(strcmp(out, game) != 0, "default-on used game dir");
    unstash_env();
}

/** --data-dir=PATH moves logs; cache and config stay on the XDG bases. */
static void test_path_override(void)
{
    char state[TEST_PATH_MAX], cache[TEST_PATH_MAX], config[TEST_PATH_MAX];
    char path[TEST_PATH_MAX], flag[TEST_PATH_MAX], want_debug[TEST_PATH_MAX];
    char want_cache[TEST_PATH_MAX], want_config[TEST_PATH_MAX];
    char want_jsc[TEST_PATH_MAX];
    SmzHostDirs dirs;
    char *argv[] = {"rt", flag};
    const char *game = test_tmp_path("smz-dd-ov-game");
    snprintf(path, sizeof(path), "%s/smz-dd-ov-path", test_tmp_root());
    snprintf(flag, sizeof(flag), "--data-dir=%s/", path);
    snprintf(want_debug, sizeof(want_debug), "%s/debug.log", path);
    stash_env();
    set_scratch_xdg("smz-dd-ov", state, cache, config);
    snprintf(want_cache, sizeof(want_cache), "%s/%s", cache, SMZUGGLER_BIN);
    snprintf(want_config, sizeof(want_config), "%s/%s", config, SMZUGGLER_BIN);
    snprintf(want_jsc, sizeof(want_jsc), "%s/jsc", want_cache);
    check(smz_host_dirs(2, argv, game, &dirs), "path override");
    expect_eq(dirs.log_dir, path, "override log dir");
    expect_eq(dirs.debug_log, want_debug, "override debug.log");
    expect_eq(dirs.cache_dir, want_cache, "override cache");
    expect_eq(dirs.jsc_dir, want_jsc, "override jsc");
    expect_eq(dirs.config_dir, want_config, "override config");
    unstash_env();
}

/** Bare --data-dir and --data-dir= use the host state dir. */
static void test_empty_flag(void)
{
    char state[TEST_PATH_MAX], cache[TEST_PATH_MAX], config[TEST_PATH_MAX];
    char want_state[TEST_PATH_MAX], out[TEST_PATH_MAX];
    char *argv_flag[] = {"rt", "--data-dir"};
    char *argv_eq[] = {"rt", "--data-dir="};
    const char *game = test_tmp_path("smz-dd-flag-game");
    stash_env();
    set_scratch_xdg("smz-dd-flag", state, cache, config);
    snprintf(want_state, sizeof(want_state), "%s/%s", state, SMZUGGLER_BIN);
    check(smz_choose_log_dir(2, argv_flag, game, out, sizeof(out)), "bare flag");
    expect_eq(out, want_state, "bare --data-dir");
    check(smz_choose_log_dir(2, argv_eq, game, out, sizeof(out)), "empty eq");
    expect_eq(out, want_state, "empty --data-dir=");
    unstash_env();
}

/** --data-dir inside the game root is refused; logs fall back to state. */
static void test_path_in_game(void)
{
    char state[TEST_PATH_MAX], cache[TEST_PATH_MAX], config[TEST_PATH_MAX];
    char flag[TEST_PATH_MAX], want_state[TEST_PATH_MAX], out[TEST_PATH_MAX];
    const char *game = test_tmp_path("smz-dd-bad-game");
    char *argv[] = {"rt", flag};
    snprintf(flag, sizeof(flag), "--data-dir=%s", game);
    stash_env();
    set_scratch_xdg("smz-dd-bad", state, cache, config);
    snprintf(want_state, sizeof(want_state), "%s/%s", state, SMZUGGLER_BIN);
    check(smz_choose_log_dir(2, argv, game, out, sizeof(out)), "refused path");
    expect_eq(out, want_state, "game-root --data-dir fallback");
    check(!smz_data_dir_usable(game, game), "game root usable");
    unstash_env();
}

/** smz_host_dirs names debug.log, the jsc child and config as main() does. */
static void test_host_paths(void)
{
    char state[TEST_PATH_MAX], cache[TEST_PATH_MAX], config[TEST_PATH_MAX];
    char want_state[TEST_PATH_MAX], want_debug[TEST_PATH_MAX];
    char want_jsc[TEST_PATH_MAX], want_config[TEST_PATH_MAX];
    char *argv[] = {"rt"};
    const char *game = test_tmp_path("smz-dd-host-game");
    SmzHostDirs dirs;
    stash_env();
    set_scratch_xdg("smz-dd-host", state, cache, config);
    snprintf(want_state, sizeof(want_state), "%s/%s", state, SMZUGGLER_BIN);
    snprintf(want_debug, sizeof(want_debug), "%s/debug.log", want_state);
    snprintf(want_jsc, sizeof(want_jsc), "%s/%s/jsc", cache, SMZUGGLER_BIN);
    snprintf(want_config, sizeof(want_config), "%s/%s", config, SMZUGGLER_BIN);
    check(smz_host_dirs(1, argv, game, &dirs), "host dirs");
    expect_eq(dirs.log_dir, want_state, "host log dir");
    expect_eq(dirs.debug_log, want_debug, "host debug.log");
    expect_eq(dirs.jsc_dir, want_jsc, "host jsc dir");
    expect_eq(dirs.config_dir, want_config, "host config dir");
    check(smz_data_dir_usable(dirs.debug_log, game), "debug.log usable");
    check(smz_data_dir_usable(dirs.jsc_dir, game), "jsc usable");
    check(smz_data_dir_usable(dirs.config_dir, game), "config usable");
    check(!exists_path(dirs.log_dir), "host dirs mkdir log");
    check(!exists_path(dirs.cache_dir), "host dirs mkdir cache");
    check(!exists_path(dirs.config_dir), "host dirs mkdir config");
    unstash_env();
}

/** A write through the helper's debug.log path lands in state, not game or cwd. */
static void test_logs_land(void)
{
    char state[TEST_PATH_MAX], cache[TEST_PATH_MAX], config[TEST_PATH_MAX];
    char cwd[TEST_PATH_MAX], old_cwd[TEST_PATH_MAX], game_log[TEST_PATH_MAX];
    char *argv[] = {"rt"};
    const char *game = test_tmp_path("smz-dd-land-game");
    SmzHostDirs dirs;
    FILE *f;
    snprintf(cwd, sizeof(cwd), "%s/smz-dd-land-cwd", test_tmp_root());
    check(test_mkdir_p(cwd), "land cwd");
    check(test_mkdir_p(game), "land game");
    check(getcwd(old_cwd, sizeof(old_cwd)) != NULL, "getcwd");
    check(chdir(cwd) == 0, "chdir land");
    stash_env();
    set_scratch_xdg("smz-dd-land", state, cache, config);
    check(smz_host_dirs(1, argv, game, &dirs), "land host dirs");
    check(test_mkdir_p(dirs.log_dir), "mkdir log dir");
    f = fopen(dirs.debug_log, "w");
    check(f != NULL, "open debug.log");
    check(fputs("ok\n", f) >= 0, "write debug.log");
    fclose(f);
    check(exists_path(dirs.debug_log), "debug.log missing in state");
    snprintf(game_log, sizeof(game_log), "%s/debug.log", game);
    check(!exists_path(game_log), "debug.log in game dir");
    check(!exists_path("debug.log"), "debug.log in cwd");
    check(dir_is_empty(cwd), "log write created cwd files");
    unstash_env();
    check(chdir(old_cwd) == 0, "chdir back");
}

#endif /* linux */

/** Reject bad arguments on every platform; XDG cases run on Linux. */
int main(void)
{
    char out[8];
    check(!smz_data_dir(SMZ_DATA_DIR_STATE, NULL, NULL, 10), "NULL out");
    check(!smz_data_dir(SMZ_DATA_DIR_STATE, NULL, out, 0), "zero size");
    check(!smz_data_dir((SmzDataDirKind)99, NULL, out, sizeof(out)), "bad kind");
    check(!smz_data_dir_usable(NULL, NULL), "NULL usable");
    check(!smz_choose_log_dir(1, NULL, NULL, NULL, 10), "choose NULL out");
    check(!smz_host_dirs(1, NULL, NULL, NULL), "host NULL dirs");
#if defined(__linux__) && !defined(__ANDROID__)
    test_xdg_set();
    test_xdg_unset();
    test_xdg_empty();
    test_relative_xdg();
    test_no_home();
    test_game_root();
    test_default_on();
    test_path_override();
    test_empty_flag();
    test_path_in_game();
    test_host_paths();
    test_logs_land();
#endif
    return 0;
}
