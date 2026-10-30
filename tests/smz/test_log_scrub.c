/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Verify the privacy boundary before diagnostics leave the runtime.
 * See docs/privacy.md; this standalone process owns its temporary environment.
 */
#define _POSIX_C_SOURCE 200809L
#include "engine/error_handler.h"
#include "smzuggler/log_scrub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Fail without printing private diagnostic contents; callable on the test thread. */
static void check(int ok)
{
    if (!ok) { fputs("privacy assertion failed\n", stderr); exit(1); }
}

/* A callback borrows the filtered message on the logger thread. */
static void capture(LogLevel level, const char *message, void *userdata)
{
    (void)level;
    check(strstr(message, userdata) == NULL);
    check(strstr(message, "~/a ~/b ~") != NULL);
}

/* Exercise bounded output and all logger sinks using process-owned temporary paths. */
int main(void)
{
    char home[] = "/tmp/smz-log-XXXXXX";
    check(mkdtemp(home) != NULL);
    check(setenv("HOME", home, 1) == 0);
#ifdef _WIN32
    check(_putenv_s("USERPROFILE", home) == 0);
#endif
    char file[256], msg[1024], out[1024], other[256];
    snprintf(file, sizeof(file), "%s/log", home);
    snprintf(other, sizeof(other), "%sx/file", home);
    snprintf(msg, sizeof(msg), "%s/a %s/b %s", home, home, home);
    log_scrub(msg, out, sizeof(out));
    check(strcmp(out, "~/a ~/b ~") == 0);
    log_scrub(other, out, sizeof(out));
    check(strcmp(out, other) == 0);
    for (size_t i = 0; msg[i]; ++i) if (msg[i] == '/') msg[i] = '\\';
    log_scrub(msg, out, sizeof(out));
    check(strcmp(out, "~\\a ~\\b ~") == 0);
    log_scrub(home, out, 1);
    check(out[0] == '\0');
    log_scrub(home, NULL, 0);
    FILE *console = tmpfile();
    check(console != NULL);
    fflush(stdout);
    int saved = dup(STDOUT_FILENO);
    check(saved >= 0 && dup2(fileno(console), STDOUT_FILENO) >= 0);
    error_handler_init(file, LOG_DEBUG);
    error_handler_set_callback(capture, home);
    error_handler_log(LOG_INFO, "%s/a %s/b %s", home, home, home);
    error_handler_shutdown();
    fflush(stdout);
    check(dup2(saved, STDOUT_FILENO) >= 0);
    close(saved);
    rewind(console);
    size_t console_count = fread(out, 1, sizeof(out) - 1, console);
    out[console_count] = '\0';
    check(strstr(out, "~/a ~/b ~") != NULL);
    check(strstr(out, home) == NULL);
    fclose(console);
    FILE *f = fopen(file, "r");
    check(f != NULL);
    size_t count = fread(out, 1, sizeof(out) - 1, f);
    out[count] = '\0';
    fclose(f);
    check(strstr(out, "~/a ~/b ~") != NULL);
    check(strstr(out, home) == NULL);
    unlink(file);
    rmdir(home);
    return 0;
}
