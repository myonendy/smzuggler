// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The smzuggler contributors
// Modified 2026-11-05: remove former engine migration contracts.
//
// The script watchdog's timer thread; the contract is in js_watchdog.h.
//
// Why: V8 can only stop a runaway script from another thread (spec section
// 4.1, item 3), so the engine needs a thread that sleeps until the current
// deadline and calls back once. It replaces the former polled interrupt
// handler. See docs/js-binding.md, "Watchdog".
//
// Rules:
// - All state is guarded by `mutex`; the fire callback runs with the mutex
//   released, so it may take as long as it likes without blocking arm().
// - The thread is joined in js_watchdog_stop; it never outlives the struct.
// - No V8 handles here (spec section 15.8).

#include "engine/js_watchdog.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <new>
#include <system_error>
#include <thread>

namespace {

using Clock = std::chrono::steady_clock;

// Watchdog threads alive now; see js_watchdog_live_threads.
std::atomic<int> g_live_threads{0};

}  // namespace

// One watchdog. `generation` and `armed_at` are written by arm(); `fired`
// is the last generation the thread called back for.
struct js_watchdog {
    std::chrono::milliseconds limit{0};
    js_watchdog_fire_fn fire = nullptr;
    void *user = nullptr;
    std::mutex mutex;
    std::condition_variable wake;
    std::atomic<uint64_t> generation{0};
    uint64_t fired = 0;
    Clock::time_point armed_at;
    bool stopping = false;
    std::thread thread;
};

namespace {

// Waits, with `lock` held on wd->mutex, until a generation that has not
// fired yet passes its deadline, or until stop. Returns that generation, or
// 0 when stopping. Watchdog thread only.
uint64_t wait_for_overrun(js_watchdog *wd, std::unique_lock<std::mutex> &lock) {
    for (;;) {
        wd->wake.wait(lock, [wd] { return wd->stopping || wd->generation != wd->fired; });
        if (wd->stopping) return 0;
        uint64_t gen = wd->generation;
        Clock::time_point deadline = wd->armed_at + wd->limit;
        bool interrupted = wd->wake.wait_until(
            lock, deadline, [wd, gen] { return wd->stopping || wd->generation != gen; });
        if (wd->stopping) return 0;
        if (!interrupted) return gen;  // deadline passed with no re-arm
    }
}

// The watchdog thread: calls back once for every generation that overruns,
// until js_watchdog_stop. js_watchdog_start counted it in g_live_threads;
// it uncounts itself on exit, just before js_watchdog_stop's join returns.
void watchdog_main(js_watchdog *wd) {
    std::unique_lock<std::mutex> lock(wd->mutex);
    for (;;) {
        uint64_t gen = wait_for_overrun(wd, lock);
        if (gen == 0) break;
        wd->fired = gen;
        lock.unlock();
        wd->fire(gen, wd->user);
        lock.lock();
    }
    lock.unlock();
    g_live_threads.fetch_sub(1);
}

}  // namespace

extern "C" {

// See js_watchdog.h.
js_watchdog *js_watchdog_start(unsigned limit_ms, js_watchdog_fire_fn fire, void *user) {
    if (limit_ms == 0 || !fire) {
        std::fprintf(stderr, "js_watchdog: need a non-zero limit and a callback\n");
        return nullptr;
    }
    js_watchdog *wd = new (std::nothrow) js_watchdog();
    if (!wd) {
        std::fprintf(stderr, "js_watchdog: out of memory\n");
        return nullptr;
    }
    wd->limit = std::chrono::milliseconds(limit_ms);
    wd->fire = fire;
    wd->user = user;
    // Counted before the thread exists, so the count is right as soon as
    // this returns, not only once the new thread is scheduled.
    g_live_threads.fetch_add(1);
    try {
        wd->thread = std::thread(watchdog_main, wd);
    } catch (const std::system_error &e) {
        g_live_threads.fetch_sub(1);
        std::fprintf(stderr, "js_watchdog: cannot start the watchdog thread: %s\n", e.what());
        delete wd;
        return nullptr;
    }
    return wd;
}

// See js_watchdog.h.
uint64_t js_watchdog_arm(js_watchdog *wd) {
    if (!wd) return 0;
    uint64_t gen;
    {
        std::lock_guard<std::mutex> guard(wd->mutex);
        wd->armed_at = Clock::now();
        gen = wd->generation.load() + 1;
        wd->generation.store(gen);
    }
    wd->wake.notify_one();
    return gen;
}

// See js_watchdog.h.
uint64_t js_watchdog_generation(js_watchdog *wd) {
    return wd ? wd->generation.load() : 0;
}

// See js_watchdog.h.
void js_watchdog_stop(js_watchdog *wd) {
    if (!wd) return;
    {
        std::lock_guard<std::mutex> guard(wd->mutex);
        wd->stopping = true;
    }
    wd->wake.notify_one();
    wd->thread.join();
    delete wd;
}

// See js_watchdog.h.
int js_watchdog_live_threads(void) { return g_live_threads.load(); }

}  // extern "C"
