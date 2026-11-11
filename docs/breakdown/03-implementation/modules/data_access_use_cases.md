---
id: INFO-041
type: info
title: Data access and scheduling — read behaviors
summary: The fourteen design-side read behaviors the data-access design must express
date: 2026-11-11
status: current
---

# Data access and scheduling — read behaviors

Fourteen design-side read behaviors the data-access design must express, grouped by kind.

- **Baseline schedules:** sequential Jacobi (every node reads the prior step's output); parallel Jacobi (identical result regardless of thread finish order); sequential Seidel (a downstream node reads the just-written value within a sweep).
- **Mixed and adaptive reads:** mixed graph (per-connection fresh, stale, or delayed); SCC iterative convergence (iterate inside a step, advance the visible frontier each iteration); long/wide graph (freshness in one branch never gated by another branch's progress); delay/transport edges (delay as a number of steps back); close-by points / derivatives (area k, k−1, k−2).
- **Correctness and restart:** deterministic golden tests (reproducible output across runs); failed-step rollback / restart / restore from a checkpoint; initialization with t=0 (first-step reads with no prior commits); recorder and export accuracy (exported timeline matches what consumers read — `INFO-024`).
- **Sub-step and unlinked reads:** multi-rate / non-dividing sub-steps (break the one-area-per-step identity); unlinked data access (read of a producer with no explicit graph edge resolves stale-only against the committed frontier, never the live write head; fresh access without an edge is a graph-construction fault).
