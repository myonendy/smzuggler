<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
<!-- Copyright (C) 2026 The smzuggler contributors -->

## DEC-M1-T010 — explicit checkpoints and nextTick priority

Spec §4.1 requires checkpoints after each macrotask, including startup callbacks.
V8 owns a dedicated nextTick queue from context creation, before loading shims.
At a checkpoint it exhausts ticks, drains promises, then repeats if promises queued ticks.
Scheduler accessors cover startup registrations; native phases temporarily wrap
document/window dispatch with a shared depth so nested listeners remain synchronous.
The exact frame order is queued native events, timers in ID order, then rAF in request order.
Each callback completes its ticks and promises before the next callback runs.
For the regression scenario the V8 log is:
`a,ap,b,bp,t1,n1,n1b,p1,p1b,t2,p2,iv,ivp,r1,rn1,rp1,r2`.
A promise queued before a startup tick still follows that tick; promise-created ticks
run after the current promise queue drains and before the checkpoint returns.
QuickJS retains phase-level jobs: `a,b,ap,bp,t1,t2,iv,n1,n1b,p1,p2,ivp,p1b,r1,r2,rn1,rp1`.
Its existing promise-backed nextTick does not prioritize ticks queued during script load.
Rejected: wrapping scheduler registration on the first frame, which misses startup work.
Rejected: checkpointing an entire input phase, which batches unrelated native events.
