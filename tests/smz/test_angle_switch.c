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
#include <string.h>

/** Report whether NEEDLE occurs in TEXT; NULL TEXT has no matches. */
static int text_has(const char *text, const char *needle)
{
    return text != NULL && strstr(text, needle) != NULL;
}

/** Return the EGL client extension string, or NULL when the client has none. */
static const char *client_extensions(void)
{
    const char *extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    return extensions;
}

/**
 * Link-time provider contract; runs without a window or GPU device.
 *
 * Returns 0 when the library linked into this binary is the provider the
 * build selected, printing the client vendor and extensions for the record.
 * Returns nonzero with a diagnostic when the EGL client cannot answer, when an
 * ANGLE build did not reach ANGLE, or when a system build unexpectedly did.
 */
int main(void)
{
    const char *extensions;
    const char *expected;

#if defined(SMZ_GLES_ANGLE)
    expected = "angle";
#elif defined(SMZ_GLES_SYSTEM)
    expected = "system";
#else
    fputs("MZ_GLES provider is not defined; the build is inconsistent\n", stderr);
    return 1;
#endif

    extensions = client_extensions();
    printf("MZ_GLES provider: %s\n", expected);
    printf("EGL client extensions: %s\n",
           extensions != NULL ? extensions : "(none)");
    if (extensions == NULL) {
        fputs("EGL client reports no extensions; cannot identify the provider\n",
              stderr);
        return 1;
    }

    if (strcmp(expected, "angle") == 0) {
        if (!text_has(extensions, "EGL_ANGLE_")) {
            fputs("MZ_GLES=angle did not link an ANGLE EGL client\n", stderr);
            return 1;
        }
    } else if (text_has(extensions, "EGL_ANGLE_")) {
        fputs("MZ_GLES=system linked an ANGLE EGL client\n", stderr);
        return 1;
    }
    return 0;
}
