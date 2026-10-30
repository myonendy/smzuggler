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
#ifdef _WIN32
    result = _putenv_s("USERPROFILE", home);
    assert(result == 0);
#endif
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
    result = rmdir(home);
    assert(result == 0);
    return 0;
}
