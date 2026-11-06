/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Verify frame-timing summary maths, JSON Lines output and writer overhead.
 * See docs/perf.md. This process owns its temporary streams and files.
 */
#define _POSIX_C_SOURCE 200809L
#include "smzuggler/frame_stats.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <unistd.h>
#endif

/* Compare two doubles without a math library dependency. */
static int close_enough(double a, double b)
{
    double diff = a - b;
    if (diff < 0) diff = -diff;
    return diff < 1e-9;
}

/* Read a flushed stream into an owned NUL-terminated string. */
static char *read_stream(FILE *stream)
{
    assert(fflush(stream) == 0);
    assert(fseek(stream, 0, SEEK_END) == 0);
    long size = ftell(stream);
    assert(size >= 0);
    assert(fseek(stream, 0, SEEK_SET) == 0);
    char *text = malloc((size_t)size + 1);
    assert(text);
    assert(fread(text, 1, (size_t)size, stream) == (size_t)size);
    text[size] = '\0';
    return text;
}

/* Hand-computed fixture: work 1..9 and 20 ms, one 26 ms present interval.
 * Nearest-rank percentiles for ten values are p50=5, p95=p99=p99.9=20. */
static void test_summary(void)
{
    FrameStatsSample samples[10];
    for (int i = 0; i < 9; i++) {
        samples[i].work_ms = (double)(i + 1);
        samples[i].present_interval_ms = 16.0;
        samples[i].gpu_ms = 0.0;
        samples[i].gpu_available = false;
    }
    samples[9].work_ms = 20.0;
    samples[9].present_interval_ms = 26.0;
    samples[9].gpu_ms = 0.0;
    samples[9].gpu_available = false;
    samples[0].gpu_ms = 1.0;
    samples[0].gpu_available = true;
    samples[1].gpu_ms = 3.0;
    samples[1].gpu_available = true;

    FrameStatsSummary summary;
    frame_stats_summarize(samples, 10, 60.0, 1000.0 / 60.0, &summary);
    assert(summary.frames == 10);
    assert(close_enough(summary.mean_work_ms, 6.5));
    assert(close_enough(summary.p50_work_ms, 5.0));
    assert(close_enough(summary.p95_work_ms, 20.0));
    assert(close_enough(summary.p99_work_ms, 20.0));
    assert(close_enough(summary.p99_9_work_ms, 20.0));
    assert(close_enough(summary.max_work_ms, 20.0));
    assert(summary.frames_over_budget == 1);
    assert(summary.missed_vblanks == 1);
    assert(summary.gpu_frames == 2);
    assert(close_enough(summary.mean_gpu_ms, 2.0));

    frame_stats_summarize(NULL, 0, 60.0, 0.0, &summary);
    assert(summary.frames == 0);
    assert(close_enough(summary.mean_work_ms, 0.0));
}

/* Prove the writer emits frame, marker and summary records with the expected
 * booleans, and that a missing GPU result is null rather than invented. */
static void test_json_lines(void)
{
    FILE *stream = tmpfile();
    assert(stream);
    FrameStats *stats = frame_stats_open_stream(stream, 60.0, 0.0);
    assert(stats);

    frame_stats_begin_frame(stats);
    frame_stats_marker(stats, FRAME_STATS_MARKER_SCENE, FRAME_STATS_MARKER_POINT);
    frame_stats_end_frame(stats, 5.0, 16.0);

    frame_stats_begin_frame(stats);
    frame_stats_marker(stats, FRAME_STATS_MARKER_SAVE, FRAME_STATS_MARKER_BEGIN);
    frame_stats_marker(stats, FRAME_STATS_MARKER_SAVE, FRAME_STATS_MARKER_END);
    frame_stats_end_frame(stats, 20.0, 30.0);
    frame_stats_close(stats);

    char *text = read_stream(stream);
    assert(strstr(text, "\"type\":\"frame\""));
    assert(strstr(text, "\"type\":\"marker\""));
    assert(strstr(text, "\"type\":\"summary\""));
    assert(strstr(text, "\"kind\":\"scene\""));
    assert(strstr(text, "\"kind\":\"save\""));
    assert(strstr(text, "\"missed_vblank\":true"));
    assert(strstr(text, "\"over_budget\":true"));
    assert(strstr(text, "\"gpu_ms\":null"));
    assert(strstr(text, "\"frames\":2"));
    assert(strstr(text, "\"frames_over_budget\":1"));
    assert(strstr(text, "\"missed_vblanks\":1"));
    free(text);
    assert(fclose(stream) == 0);
}

/* The disabled path is a set of NULL-safe no-ops and opens no file. */
static void test_disabled(void)
{
    frame_stats_begin_frame(NULL);
    frame_stats_end_frame(NULL, 1.0, 1.0);
    frame_stats_marker(NULL, FRAME_STATS_MARKER_SCENE, FRAME_STATS_MARKER_POINT);
    frame_stats_close(NULL);
    assert(frame_stats_open(NULL, 60.0, 0.0) == NULL);
    assert(frame_stats_open("", 60.0, 0.0) == NULL);
}

/* The path variant owns and closes its file; the summary is readable after. */
static void test_path(void)
{
#ifndef _WIN32
    char path[] = "/tmp/smz-frame-stats-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    close(fd);
    FrameStats *stats = frame_stats_open(path, 60.0, 0.0);
    assert(stats);
    frame_stats_begin_frame(stats);
    frame_stats_end_frame(stats, 1.0, 16.0);
    frame_stats_close(stats);
    FILE *stream = fopen(path, "r");
    assert(stream);
    char *text = read_stream(stream);
    assert(strstr(text, "\"type\":\"summary\""));
    free(text);
    assert(fclose(stream) == 0);
    assert(remove(path) == 0);
#endif
}

/* Measure the enabled writer's per-frame cost. The card's target is under
 * 0.2 ms/frame; the test fails rather than reporting a number that misses it. */
static void test_overhead(void)
{
    FILE *stream = tmpfile();
    assert(stream);
    FrameStats *stats = frame_stats_open_stream(stream, 60.0, 0.0);
    assert(stats);
    const int frames = 5000;
    clock_t start = clock();
    for (int i = 0; i < frames; i++) {
        frame_stats_begin_frame(stats);
        frame_stats_end_frame(stats, 1.0, 16.0);
    }
    double elapsed_ms = (double)(clock() - start) * 1000.0 / (double)CLOCKS_PER_SEC;
    double per_frame = elapsed_ms / (double)frames;
    printf("frame_stats overhead: %.4f ms/frame (%d frames)\n", per_frame, frames);
    assert(per_frame < 0.2);
    frame_stats_close(stats);
    assert(fclose(stream) == 0);
}

/* Run every fixture on the test thread; no SDL or GL context is needed. */
int main(void)
{
    test_summary();
    test_json_lines();
    test_disabled();
    test_path();
    test_overhead();
    return 0;
}
