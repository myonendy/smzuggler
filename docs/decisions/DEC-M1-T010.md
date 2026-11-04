<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The smzuggler contributors -->

## DEC-M1-T010 — explicit checkpoints and nextTick priority

Spec §4.1 requires per-task checkpoints; V8 owns nextTick from context creation.
At a checkpoint it exhausts ticks, drains promises, then repeats if promises queued ticks.
Scheduler accessors cover startup registrations; input batches are polled once and
delivered through the original dispatcher one event at a time, then checkpointed.
This includes overlay bubbling and default actions; resize/gamepad dispatch uses shared depth.
Frame order: native events, timers by ID, rAF by request; each drains ticks and promises.
V8 regression log: `a,ap,b,bp,t1,n1,n1b,p1,p1b,t2,p2,iv,ivp,r1,rn1,rp1,r2`.
Startup ticks precede promises; promise-created ticks follow the current promise drain.
QuickJS retains phase-level jobs: `a,b,ap,bp,t1,t2,iv,n1,n1b,p1,p2,ivp,p1b,r1,r2,rn1,rp1`.
Its existing promise-backed nextTick does not prioritize ticks queued during script load.
Rejected: wrapping scheduler registration on the first frame, which misses startup work.
Rejected: checkpointing an entire input phase, which batches unrelated native events.

Overlay Backspace order (initial `abc`), V8: `key:abc,input:ab,tick:ab,promise:ab,key:ab,input:a,tick:a,promise:a`.
QuickJS runs `key:abc,input:ab,key:ab,input:a,tick:a,tick:a,promise:a,promise:a`.
