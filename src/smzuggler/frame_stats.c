/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 The smzuggler contributors
 * Always-compiled frame-timing writer for --frame-stats (spec §14.3).
 * See docs/perf.md. The main thread owns the writer; the path variant owns
 * its FILE, the stream variant borrows it. GPU queries need a current GL
 * context and are skipped when the entry points are unavailable.
 */
#include "smzuggler/frame_stats.h"

#include <stdlib.h>
#include <string.h>

#ifdef RMMZ_HAS_GL
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#endif

/* GL query constants are the same for core GL and EXT_disjoint_timer_query. */
#define FRAME_STATS_GL_TIME_ELAPSED 0x88BF
#define FRAME_STATS_GL_QUERY_RESULT 0x8866
#define FRAME_STATS_GL_QUERY_RESULT_AVAILABLE 0x8867

#ifdef RMMZ_HAS_GL
typedef void (*FrameStatsGenQueries)(int, unsigned *);
typedef void (*FrameStatsDeleteQueries)(int, const unsigned *);
typedef void (*FrameStatsBeginQuery)(unsigned, unsigned);
typedef void (*FrameStatsEndQuery)(unsigned);
typedef void (*FrameStatsGetQueryUiv)(unsigned, unsigned, unsigned *);
typedef void (*FrameStatsGetQueryUi64v)(unsigned, unsigned, uint64_t *);
#endif

struct FrameStats {
    FILE  *out;
    bool   owns_stream;
    double refresh_hz;
    double budget_ms;

    uint64_t frame_index;    /* next frame number */
    uint64_t current_frame;  /* frame whose markers are being recorded */
    uint64_t pending_frame;  /* frame held until its GPU result is read */
    bool     has_pending;
    FrameStatsSample pending;

    FrameStatsSample *samples;
    size_t count;
    size_t capacity;

#ifdef RMMZ_HAS_GL
    bool gpu_ready;
    bool query_active;
    int  query_slot;
    int  query_pending;
    unsigned queries[2];
    FrameStatsGenQueries gen_queries;
    FrameStatsDeleteQueries delete_queries;
    FrameStatsBeginQuery begin_query;
    FrameStatsEndQuery end_query;
    FrameStatsGetQueryUiv get_query_uiv;
    FrameStatsGetQueryUi64v get_query_ui64v;
#endif
};

/* Resolve one GL entry point, trying the core name first and the GLES EXT
 * name second. Returns NULL when neither exists. */
#ifdef RMMZ_HAS_GL
static void *gl_proc(const char *core, const char *ext)
{
    void *fn = SDL_GL_GetProcAddress(core);
    return fn ? fn : SDL_GL_GetProcAddress(ext);
}

/* Load the timer-query entry points and create two query objects. A missing
 * entry point leaves GPU timing unavailable rather than inventing numbers. */
static bool gpu_load(FrameStats *stats)
{
    stats->gen_queries = (FrameStatsGenQueries)gl_proc("glGenQueries", "glGenQueriesEXT");
    stats->delete_queries = (FrameStatsDeleteQueries)gl_proc("glDeleteQueries", "glDeleteQueriesEXT");
    stats->begin_query = (FrameStatsBeginQuery)gl_proc("glBeginQuery", "glBeginQueryEXT");
    stats->end_query = (FrameStatsEndQuery)gl_proc("glEndQuery", "glEndQueryEXT");
    stats->get_query_uiv = (FrameStatsGetQueryUiv)gl_proc("glGetQueryObjectuiv", "glGetQueryObjectuivEXT");
    stats->get_query_ui64v = (FrameStatsGetQueryUi64v)gl_proc("glGetQueryObjectui64v", "glGetQueryObjectui64vEXT");
    if (!stats->gen_queries || !stats->delete_queries || !stats->begin_query ||
        !stats->end_query || !stats->get_query_uiv)
        return false;
    stats->gen_queries(2, stats->queries);
    stats->query_slot = -1;
    stats->query_pending = -1;
    return true;
}

/* Begin this frame's elapsed-time query. The result is read one frame later
 * so the CPU never waits for the GPU. */
static void gpu_begin(FrameStats *stats)
{
    if (!stats->gpu_ready) return;
    stats->query_slot = (stats->query_slot + 1) % 2;
    stats->begin_query(FRAME_STATS_GL_TIME_ELAPSED, stats->queries[stats->query_slot]);
    stats->query_active = true;
}

/* End this frame's query and remember it for the next begin. */
static void gpu_end(FrameStats *stats)
{
    if (!stats->gpu_ready || !stats->query_active) return;
    stats->end_query(FRAME_STATS_GL_TIME_ELAPSED);
    stats->query_active = false;
    stats->query_pending = stats->query_slot;
}

/* Read the previous frame's query without blocking. Returns false when the
 * result is not ready yet; the frame is then written with gpu_ms null. */
static bool gpu_read_pending(FrameStats *stats, double *out_ms)
{
    if (!stats->gpu_ready || stats->query_pending < 0) return false;
    unsigned available = 0;
    stats->get_query_uiv(stats->queries[stats->query_pending],
                         FRAME_STATS_GL_QUERY_RESULT_AVAILABLE, &available);
    if (!available) {
        stats->query_pending = -1;
        return false;
    }
    uint64_t ns = 0;
    if (stats->get_query_ui64v) {
        stats->get_query_ui64v(stats->queries[stats->query_pending],
                               FRAME_STATS_GL_QUERY_RESULT, &ns);
    } else {
        unsigned value = 0;
        stats->get_query_uiv(stats->queries[stats->query_pending],
                             FRAME_STATS_GL_QUERY_RESULT, &value);
        ns = value;
    }
    stats->query_pending = -1;
    *out_ms = (double)ns / 1000000.0;
    return true;
}
#else
/* Non-GL builds have no timer queries; keep the call sites unconditional. */
static void gpu_begin(FrameStats *stats) { (void)stats; }

/* Non-GL builds have no timer queries; nothing to end. */
static void gpu_end(FrameStats *stats) { (void)stats; }

/* Non-GL builds never have a GPU result, so the record says unavailable. */
static bool gpu_read_pending(FrameStats *stats, double *out_ms)
{
    (void)stats;
    (void)out_ms;
    return false;
}
#endif

/* Append one sample to the summary array, growing it geometrically. A failed
 * growth keeps the JSON line but drops the sample from the summary. */
static void append_sample(FrameStats *stats, const FrameStatsSample *sample)
{
    if (stats->count == stats->capacity) {
        size_t capacity = stats->capacity ? stats->capacity * 2 : 256;
        FrameStatsSample *grown = realloc(stats->samples, capacity * sizeof *grown);
        if (!grown) return;
        stats->samples = grown;
        stats->capacity = capacity;
    }
    stats->samples[stats->count++] = *sample;
}

/* Write one JSON Lines frame record. Missed vblanks use the spec's 1.5x
 * refresh rule; over-budget uses the configured budget. */
static void write_sample(FrameStats *stats, uint64_t frame,
                         const FrameStatsSample *sample)
{
    double period = 1000.0 / stats->refresh_hz;
    bool missed = sample->present_interval_ms > 1.5 * period;
    bool over = sample->work_ms > stats->budget_ms;
    fprintf(stats->out,
            "{\"type\":\"frame\",\"frame\":%llu,\"work_ms\":%.6f,",
            (unsigned long long)frame, sample->work_ms);
    if (sample->gpu_available)
        fprintf(stats->out, "\"gpu_ms\":%.6f,", sample->gpu_ms);
    else
        fprintf(stats->out, "\"gpu_ms\":null,");
    fprintf(stats->out,
            "\"present_interval_ms\":%.6f,\"missed_vblank\":%s,"
            "\"over_budget\":%s}\n",
            sample->present_interval_ms, missed ? "true" : "false",
            over ? "true" : "false");
}

/* Write the held frame once its GPU query has had a frame to finish. */
static void flush_pending(FrameStats *stats)
{
    if (!stats->has_pending) return;
    FrameStatsSample sample = stats->pending;
    sample.gpu_available = gpu_read_pending(stats, &sample.gpu_ms);
    write_sample(stats, stats->pending_frame, &sample);
    append_sample(stats, &sample);
    stats->has_pending = false;
}

/* Compare two doubles for qsort without subtracting, which can overflow. */
static int compare_double(const void *left, const void *right)
{
    double a = *(const double *)left;
    double b = *(const double *)right;
    return (a > b) - (a < b);
}

/* Nearest-rank percentile: rank = ceil(p/100 * count), clamped to 1..count.
 * The caller passes a sorted array with at least one element. */
static double percentile(const double *sorted, size_t count, double p)
{
    double scaled = p * (double)count / 100.0;
    size_t rank = (size_t)scaled;
    if ((double)rank < scaled) rank++;
    if (rank < 1) rank = 1;
    if (rank > count) rank = count;
    return sorted[rank - 1];
}

/* Write the summary record. mean_gpu_ms is null when no query result was
 * ready, so a CPU-only run never reports a fabricated GPU number. */
static void write_summary(FrameStats *stats)
{
    FrameStatsSummary summary;
    frame_stats_summarize(stats->samples, stats->count, stats->refresh_hz,
                          stats->budget_ms, &summary);
    fprintf(stats->out,
            "{\"type\":\"summary\",\"frames\":%zu,\"refresh_hz\":%.6f,"
            "\"budget_ms\":%.6f,\"mean_work_ms\":%.6f,\"p50_work_ms\":%.6f,"
            "\"p95_work_ms\":%.6f,\"p99_work_ms\":%.6f,"
            "\"p99_9_work_ms\":%.6f,\"max_work_ms\":%.6f,"
            "\"frames_over_budget\":%zu,\"missed_vblanks\":%zu,"
            "\"gpu_frames\":%zu,",
            summary.frames, stats->refresh_hz, stats->budget_ms,
            summary.mean_work_ms, summary.p50_work_ms, summary.p95_work_ms,
            summary.p99_work_ms, summary.p99_9_work_ms, summary.max_work_ms,
            summary.frames_over_budget, summary.missed_vblanks,
            summary.gpu_frames);
    if (summary.gpu_frames)
        fprintf(stats->out, "\"mean_gpu_ms\":%.6f}\n", summary.mean_gpu_ms);
    else
        fprintf(stats->out, "\"mean_gpu_ms\":null}\n");
}

/* Allocate a writer around STREAM. OWNED says whether close must fclose it. */
static FrameStats *frame_stats_create(FILE *stream, bool owned, double refresh_hz,
                                      double budget_ms)
{
    if (!stream) return NULL;
    FrameStats *stats = calloc(1, sizeof *stats);
    if (!stats) return NULL;
    stats->out = stream;
    stats->owns_stream = owned;
    stats->refresh_hz = refresh_hz > 0 ? refresh_hz : 60.0;
    stats->budget_ms = budget_ms > 0 ? budget_ms : 1000.0 / stats->refresh_hz;
#ifdef RMMZ_HAS_GL
    stats->gpu_ready = gpu_load(stats);
#endif
    return stats;
}

/* See the header for ownership and the refresh/budget defaults. */
FrameStats *frame_stats_open(const char *path, double refresh_hz, double budget_ms)
{
    if (!path || !*path) return NULL;
    FILE *stream = fopen(path, "w");
    if (!stream) return NULL;
    FrameStats *stats = frame_stats_create(stream, true, refresh_hz, budget_ms);
    if (!stats) fclose(stream);
    return stats;
}

/* See the header: the caller keeps ownership of STREAM. */
FrameStats *frame_stats_open_stream(FILE *stream, double refresh_hz, double budget_ms)
{
    return frame_stats_create(stream, false, refresh_hz, budget_ms);
}

/* See the header. The last frame is flushed before the summary. */
void frame_stats_close(FrameStats *stats)
{
    if (!stats) return;
    flush_pending(stats);
    write_summary(stats);
    if (stats->out) fflush(stats->out);
#ifdef RMMZ_HAS_GL
    if (stats->gpu_ready) {
        if (stats->query_active) stats->end_query(FRAME_STATS_GL_TIME_ELAPSED);
        stats->delete_queries(2, stats->queries);
    }
#endif
    if (stats->owns_stream && stats->out) fclose(stats->out);
    free(stats->samples);
    free(stats);
}

/* See the header. The previous frame is written before the new query starts. */
void frame_stats_begin_frame(FrameStats *stats)
{
    if (!stats) return;
    flush_pending(stats);
    stats->current_frame = stats->frame_index++;
    gpu_begin(stats);
}

/* See the header. The sample waits for the next begin to read its query. */
void frame_stats_end_frame(FrameStats *stats, double work_ms,
                           double present_interval_ms)
{
    if (!stats) return;
    gpu_end(stats);
    stats->pending.work_ms = work_ms;
    stats->pending.present_interval_ms = present_interval_ms;
    stats->pending.gpu_ms = 0.0;
    stats->pending.gpu_available = false;
    stats->pending_frame = stats->current_frame;
    stats->has_pending = true;
}

/* Marker names are fixed, so no JSON escaping is needed. */
static const char *marker_kind_name(FrameStatsMarker kind)
{
    switch (kind) {
    case FRAME_STATS_MARKER_SCENE: return "scene";
    case FRAME_STATS_MARKER_TRANSFER: return "transfer";
    case FRAME_STATS_MARKER_SAVE: return "save";
    case FRAME_STATS_MARKER_LOAD: return "load";
    }
    return "unknown";
}

/* Marker phases are fixed, so no JSON escaping is needed. */
static const char *marker_phase_name(FrameStatsMarkerPhase phase)
{
    switch (phase) {
    case FRAME_STATS_MARKER_BEGIN: return "begin";
    case FRAME_STATS_MARKER_END: return "end";
    case FRAME_STATS_MARKER_POINT: return "point";
    }
    return "point";
}

/* See the header. Markers are written immediately so a crash still leaves
 * the transition boundaries in the file. */
void frame_stats_marker(FrameStats *stats, FrameStatsMarker kind,
                        FrameStatsMarkerPhase phase)
{
    if (!stats) return;
    fprintf(stats->out,
            "{\"type\":\"marker\",\"frame\":%llu,\"kind\":\"%s\","
            "\"phase\":\"%s\"}\n",
            (unsigned long long)stats->current_frame, marker_kind_name(kind),
            marker_phase_name(phase));
}

/* See the header for the nearest-rank definition and the zero-count case. */
void frame_stats_summarize(const FrameStatsSample *samples, size_t count,
                           double refresh_hz, double budget_ms,
                           FrameStatsSummary *out)
{
    if (!out) return;
    memset(out, 0, sizeof *out);
    if (!samples || count == 0) return;
    if (refresh_hz <= 0) refresh_hz = 60.0;
    if (budget_ms <= 0) budget_ms = 1000.0 / refresh_hz;
    double *work = malloc(count * sizeof *work);
    if (!work) return;
    double period = 1000.0 / refresh_hz;
    double work_sum = 0.0;
    double gpu_sum = 0.0;
    for (size_t i = 0; i < count; i++) {
        work[i] = samples[i].work_ms;
        work_sum += samples[i].work_ms;
        if (samples[i].gpu_available) {
            gpu_sum += samples[i].gpu_ms;
            out->gpu_frames++;
        }
        if (samples[i].work_ms > budget_ms) out->frames_over_budget++;
        if (samples[i].present_interval_ms > 1.5 * period) out->missed_vblanks++;
    }
    qsort(work, count, sizeof *work, compare_double);
    out->frames = count;
    out->mean_work_ms = work_sum / (double)count;
    out->p50_work_ms = percentile(work, count, 50.0);
    out->p95_work_ms = percentile(work, count, 95.0);
    out->p99_work_ms = percentile(work, count, 99.0);
    out->p99_9_work_ms = percentile(work, count, 99.9);
    out->max_work_ms = work[count - 1];
    if (out->gpu_frames) out->mean_gpu_ms = gpu_sum / (double)out->gpu_frames;
    free(work);
}
