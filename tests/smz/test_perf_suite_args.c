/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Check the §14 harness argument rules and fixture import refusals.
 * See docs/perf.md and docs/testing-fixtures.md. Temporary paths only.
 */
#define _POSIX_C_SOURCE 200809L
#include "smzuggler/perf_suite.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static char root[512];
/** Join BASE/LEAF into OUT (caller-owned). */
static void join(char *out, size_t size, const char *base, const char *leaf)
{
    int n = snprintf(out, size, "%s/%s", base, leaf);
    assert(n > 0 && (size_t)n < size);
}
/** Write TEXT to PATH. */
static void put(const char *path, const char *text)
{
    FILE *stream = fopen(path, "wb");
    assert(stream && fwrite(text, 1, strlen(text), stream) == strlen(text));
    assert(fclose(stream) == 0);
}
/** Write a zlib-wrapped deflate save, as StorageManager.jsonToZip does.
 * Vectors are zlib.compress UTF-8 JSON, compatible with pako.deflate.
 */
static void put_zip(const char *path, const char *data, size_t size)
{
    FILE *stream = fopen(path, "wb");
    assert(stream && fwrite(data, 1, size, stream) == size);
    assert(fclose(stream) == 0);
}
/** Parse ARGV and require STATUS; NEEDLE must appear in the error line. */
static void parse(int argc, char **argv, int status, const char *needle)
{
    SmzPerfSuiteOptions options;
    char err[512] = "";
    assert(smz_perf_suite_parse_args(argc, argv, &options, err, sizeof err)
           == status);
    if (needle) assert(strstr(err, needle) != NULL);
}
/** Accepted and refused argument combinations. */
static void test_parse(void)
{
    char out[1024], slash[1024], file[1024];
    join(out, sizeof out, root, "out");
    join(slash, sizeof slash, root, "slash/");
    join(file, sizeof file, root, "file.txt");
    assert(mkdir(out, 0700) == 0);
    put(file, "x");
    char *good[] = {"p", "--perf-suite", "--frame-stats", out};
    char *deck[] = {"p", "--perf-suite=deck", "--frame-stats", slash};
    char *bad[][7] = {
        {"p", "--perf-suite", "--frame-stats", NULL},
        {"p", "--perf-suite", "--frame-stats", file},
        {"p", "--perf-suite", "--frame-stats", out, "--control"},
        {"p", "--perf-suite=toaster", "--frame-stats", out},
    };
    const int lengths[] = {3, 4, 5, 4};
    const char *needles[] = {"needs a file", "directory", "--control",
                             "unknown profile"};
    int i;
    parse(4, good, 0, NULL);
    parse(4, deck, 0, NULL);
    for (i = 0; i < 4; i++)
        parse(lengths[i], bad[i], 2, needles[i]);
    parse(4, (char *[]){"p", "--import-fixture", "12", "fx-map92"}, 0, NULL);
    parse(3, (char *[]){"p", "--import-fixture", "12"}, 2, "expected");
    parse(4, (char *[]){"p", "--import-fixture", "zero", "fx-map92"}, 2,
          "slot must be");
    parse(4, (char *[]){"p", "--import-fixture", "12", "other"}, 2,
          "unknown fixture");
    parse(7, (char *[]){"p", "--perf-suite", "--frame-stats", out,
                        "--import-fixture", "12", "fx-map92"}, 2,
          "cannot be combined");
}
/** Missing, wrong-map and accepted imports. */
static void test_import(void)
{
    char xdg[1024], saves[1024], path[1024], fixture[1024];
    char err[512];
    SmzPerfSuiteOptions options = {0};
    join(xdg, sizeof xdg, root, "xdg");
    join(saves, sizeof saves, root, "saves");
    assert(mkdir(xdg, 0700) == 0 && mkdir(saves, 0700) == 0);
    assert(setenv("XDG_DATA_HOME", xdg, 1) == 0);
    assert(setenv("SMZUGGLER_SAVE_DIR", saves, 1) == 0);
    options.import_fixture = "fx-map92";
    options.import_slot = 1;
    assert(smz_perf_import_fixture(&options, err, sizeof err) == 1);
    assert(strstr(err, "not found") != NULL);
    join(path, sizeof path, saves, "file2.rmmzsave");
    put_zip(path, "\x78\x9c\xab\x56\xca\x4d\x2c\x50\xb2\xaa\x56\x8a\x07\xd2\x9e\x29\x4a\x56\xa6\xb5\xb5\x00\x45\x91\x06\xaa", 26);
    options.import_slot = 2;
    assert(smz_perf_import_fixture(&options, err, sizeof err) == 1);
    assert(strstr(err, "requires map 92") != NULL);
    join(path, sizeof path, saves, "file3.rmmzsave");
    put_zip(path, "\x78\x9c\xab\x56\xca\x4d\x2c\x50\xb2\xaa\x56\x8a\x07\xd2\x9e\x29\x4a\x56\x96\x46\xb5\xb5\x00\x4b\xe7\x06\xe0", 27);
    options.import_slot = 3;
    assert(smz_perf_import_fixture(&options, err, sizeof err) == 0);
    join(fixture, sizeof fixture, xdg, "smzuggler-rt/fixtures/fx-map92");
    assert(access(fixture, R_OK) == 0);
    assert(smz_perf_validate_fixture("fx-map92", err, sizeof err) == 0);
    assert(unlink(fixture) == 0);
    assert(smz_perf_validate_fixture("fx-map92", err, sizeof err) == 1);
    assert(strstr(err, "missing fixture") != NULL);
    put_zip(fixture, "\x78\x9c\xab\x56\xca\x4d\x2c\x50\xb2\xaa\x56\x8a\x07\xd2\x9e\x29\x4a\x56\xe6\xb5\xb5\x00\x45\x97\x06\xac", 26);
    assert(smz_perf_validate_fixture("fx-map92", err, sizeof err) == 1);
    assert(strstr(err, "map 7") && strstr(err, "map 92"));
    join(path, sizeof path, saves, "file4.rmmzsave");
    put_zip(path, "\x78\x9c\xab\x56\xca\x4d\x2c\x50\xb2\xaa\x56\x8a\x07\xd2\x9e\x29\x4a\x56\x96\xc6\xb5\xb5\x00\x4b\xea\x06\xe1", 27);
    options.import_slot = 4;
    options.import_fixture = "fx-trade";
    assert(smz_perf_import_fixture(&options, err, sizeof err) == 0);
}
/** Remove every path this test created. */
static void cleanup(void)
{
    char path[1024];
    const char *files[] = {"file.txt", "saves/file2.rmmzsave",
        "saves/file3.rmmzsave", "saves/file4.rmmzsave",
        "xdg/smzuggler-rt/fixtures/fx-map92",
        "xdg/smzuggler-rt/fixtures/fx-trade"};
    size_t i;
    for (i = 0; i < sizeof files / sizeof files[0]; i++) {
        join(path, sizeof path, root, files[i]);
        assert(unlink(path) == 0);
    }
    join(path, sizeof path, root, "out");
    assert(rmdir(path) == 0);
    join(path, sizeof path, root, "saves");
    assert(rmdir(path) == 0);
    join(path, sizeof path, root, "xdg/smzuggler-rt/fixtures");
    assert(rmdir(path) == 0);
    join(path, sizeof path, root, "xdg/smzuggler-rt");
    assert(rmdir(path) == 0);
    join(path, sizeof path, root, "xdg");
    assert(rmdir(path) == 0);
    assert(rmdir(root) == 0);
}
/** Exercise CLI and real encoded saves in an isolated temporary directory. */
int main(void)
{
    char template[] = "/tmp/smz-perf-args-XXXXXX";
    assert(mkdtemp(template) != NULL);
    snprintf(root, sizeof root, "%s", template);
    test_parse();
    test_import();
    cleanup();
    return 0;
}
