/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Frame-timing instrumentation for --frame-stats (spec §14.3).
 * See docs/perf.md. The main thread owns the writer and its samples; the
 * stream variant borrows the caller's FILE, the path variant owns its file.
 */
#ifndef SMZUGGLER_FRAME_STATS_H
#define SMZUGGLER_FRAME_STATS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct FrameStats FrameStats;

/* One completed frame. Times are milliseconds; present_interval_ms is 0 for
 * the first frame, when no previous present exists. gpu_available is false
 * when no GL timer query result was ready, so gpu_ms is then meaningless. */
typedef struct {
    double work_ms;
    double present_interval_ms;
    double gpu_ms;
    bool   gpu_available;
} FrameStatsSample;

/* Summary of a sample array. Percentiles use the nearest-rank definition:
 * pN is the value at rank ceil(N/100 * count), 1-based. A zero count yields
 * a zeroed summary. mean_gpu_ms covers only samples with gpu_available. */
typedef struct {
    size_t frames;
    double mean_work_ms;
    double p50_work_ms;
    double p95_work_ms;
    double p99_work_ms;
    double p99_9_work_ms;
    double max_work_ms;
    size_t frames_over_budget;
    size_t missed_vblanks;
    size_t gpu_frames;
    double mean_gpu_ms;
} FrameStatsSummary;

/* Marker kinds the analyser can exclude (spec §14.5). Scene and transfer are
 * point events; save and load are begin/end windows around the call. */
typedef enum {
    FRAME_STATS_MARKER_SCENE,
    FRAME_STATS_MARKER_TRANSFER,
    FRAME_STATS_MARKER_SAVE,
    FRAME_STATS_MARKER_LOAD
} FrameStatsMarker;

typedef enum {
    FRAME_STATS_MARKER_BEGIN,
    FRAME_STATS_MARKER_END,
    FRAME_STATS_MARKER_POINT
} FrameStatsMarkerPhase;

/* Open PATH for writing and own the file. refresh_hz <= 0 means 60 Hz;
 * budget_ms <= 0 means one refresh period. Returns NULL on failure. */
FrameStats *frame_stats_open(const char *path, double refresh_hz, double budget_ms);

/* Write to a borrowed STREAM; the caller keeps it open until after
 * frame_stats_close and then closes it. Returns NULL on allocation failure. */
FrameStats *frame_stats_open_stream(FILE *stream, double refresh_hz, double budget_ms);

/* Flush the last frame and the summary, release GL queries and samples, and
 * close the file only when frame_stats_open created it. NULL is ignored. */
void frame_stats_close(FrameStats *stats);

/* Start a frame: write the previous frame now that its GPU query can be read,
 * then begin this frame's GPU query. Call before taking the frame start time
 * so the writer's cost is outside the measured work. NULL is a no-op. */
void frame_stats_begin_frame(FrameStats *stats);

/* End a frame: stop the GPU query and hold the sample until the next begin
 * (or close) can attach the query result. work_ms is frame start through
 * swap; present_interval_ms is the time since the previous present. */
void frame_stats_end_frame(FrameStats *stats, double work_ms,
                           double present_interval_ms);

/* Record a marker event for the current frame. NULL is a no-op. */
void frame_stats_marker(FrameStats *stats, FrameStatsMarker kind,
                        FrameStatsMarkerPhase phase);

/* Compute the summary of COUNT samples. SAMPLES is borrowed and may be NULL
 * when COUNT is 0; OUT is caller-owned and always written. */
void frame_stats_summarize(const FrameStatsSample *samples, size_t count,
                           double refresh_hz, double budget_ms,
                           FrameStatsSummary *out);

#endif /* SMZUGGLER_FRAME_STATS_H */
