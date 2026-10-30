/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Guard privacy filtering at buffer and path boundaries; see docs/privacy.md.
 * This process owns its temporary environment and never prints its contents.
 */
#define _POSIX_C_SOURCE 200809L
#include "smzuggler/log_scrub.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Check fail-closed delimiters and ignore roots using borrowed home text.
 * Test-thread only; owns local buffers and environment changes, asserts on error.
 */
static void check_delimiters_and_roots(const char *home)
{
    char input[512], output[512];
    int result;
    snprintf(input, sizeof(input), "%s. %s> %s| %s= %s! %s? %s`",
             home, home, home, home, home, home, home);
    log_scrub(input, output, sizeof(output));
    assert(strcmp(output, "~. ~> ~| ~= ~! ~? ~`") == 0);
    snprintf(input, sizeof(input), "%s.", home);
    log_scrub(input, output, sizeof(output));
    assert(strcmp(output, "~.") == 0);
    snprintf(input, sizeof(input), "%s.log %s-child %s_child", home, home, home);
    log_scrub(input, output, sizeof(output));
    assert(strcmp(output, input) == 0);
    result = setenv("HOME", "/", 1);
    assert(result == 0);
    log_scrub("/var/log /", output, sizeof(output));
    assert(strcmp(output, "/var/log /") == 0);
    result = setenv("HOME", "C:/", 1);
    assert(result == 0);
    log_scrub("C:/logs C:/", output, sizeof(output));
    assert(strcmp(output, "C:/logs C:/") == 0);
}

/* Run on the test thread; all buffers are local and temporary paths are removed.
 * Fail by assertion without printing private inputs; no resources escape.
 */
int main(void)
{
    char home[] = "/tmp/smz-boundary-XXXXXX";
    char input[512], output[512];
    char bounded[] = {'x', 'x', 'x', 'x'};
    char *created = mkdtemp(home);
    assert(created != NULL);
    int result = setenv("HOME", home, 1);
    assert(result == 0);
    snprintf(input, sizeof(input), "(%s), %s/child %sx/child", home, home, home);
    log_scrub(input, output, sizeof(output));
    assert(strncmp(output, "(~), ~/child ", 13) == 0);
    char *sibling = strstr(output, home);
    assert(sibling != NULL);
    assert(strcmp(sibling + strlen(home), "x/child") == 0);
    log_scrub(home, bounded, 2);
    assert(bounded[0] == '~');
    assert(bounded[1] == '\0');
    assert(bounded[2] == 'x' && bounded[3] == 'x');
    log_scrub("abcdef", bounded, 3);
    assert(strcmp(bounded, "ab") == 0);
    assert(bounded[3] == 'x');
    check_delimiters_and_roots(home);
    result = rmdir(home);
    assert(result == 0);
    return 0;
}
