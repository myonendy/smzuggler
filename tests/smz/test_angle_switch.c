/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 *
 * Provider probe for the MZ_GLES switch (spec §4.2).
 *
 * Why: the build selects either the platform EGL/GLES or ANGLE, but both
 * expose the same symbols, so a successful link alone cannot tell them apart.
 * The EGL client extension string can: ANGLE advertises EGL_ANGLE_* platform
 * extensions, while Mesa's client extensions do not. The probe therefore
 * records which provider it was built for and fails when the linked library
 * disagrees. It never creates a window or a GPU device, so it also runs on a
 * CI runner with no display.
 */
#include <EGL/egl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Report whether NEEDLE occurs in TEXT; NULL TEXT has no matches. */
static int text_has(const char *text, const char *needle)
{
    return text != NULL && strstr(text, needle) != NULL;
}

/** Return the EGL client extension string, or NULL when the client has none. */
static const char *client_extensions(void)
{
    return eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
}

/**
 * Fail loudly with a diagnostic and a nonzero exit status in every build.
 *
 * The message names the provider contract that broke. An explicit exit keeps
 * the failure visible to ctest even when Release builds define NDEBUG.
 */
static void check(int ok, const char *what)
{
    if (!ok) {
        fprintf(stderr, "angle_switch: %s\n", what);
        exit(EXIT_FAILURE);
    }
}

/**
 * Link-time provider contract; runs without a window or GPU device.
 *
 * The build defines exactly one provider macro, and the EGL client extension
 * string proves which library the link actually reached. The test owns no
 * resources, so it is safe to call on any thread and returns 0 on success.
 */
int main(void)
{
    const char *extensions;
    const char *expected;
    int want_angle;

#if defined(SMZ_GLES_ANGLE) && defined(SMZ_GLES_SYSTEM)
#error "MZ_GLES must select exactly one provider"
#elif defined(SMZ_GLES_ANGLE)
    expected = "angle";
    want_angle = 1;
#elif defined(SMZ_GLES_SYSTEM)
    expected = "system";
    want_angle = 0;
#else
    fputs("MZ_GLES provider is not defined; the build is inconsistent\n", stderr);
    return 1;
#endif

    extensions = client_extensions();
    printf("MZ_GLES provider: %s\n", expected);
    printf("EGL client extensions: %s\n",
           extensions != NULL ? extensions : "(none)");

    check(extensions != NULL,
          "EGL client reports no extensions; cannot identify the provider");
    check(want_angle == text_has(extensions, "EGL_ANGLE_"),
          want_angle ? "MZ_GLES=angle did not link an ANGLE EGL client"
                     : "MZ_GLES=system linked an ANGLE EGL client");
    return 0;
}
