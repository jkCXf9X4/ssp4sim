---
id: INFO-035
type: info
title: Profiling
summary: Two profiling categories — build-time tracing and runtime perf — with generated traces stored under build/
date: 2026-11-11
status: current
---

# Profiling

Repeatable profiling for build-time and runtime investigation; generated traces stay under build/.

- **Build-time tracing:** Ninja trace, stats, and graph inspection diagnose slow compilation and link steps.
- **Runtime profiling:** Linux perf investigates CPU-bound performance during simulation runs.
- **Trace storage:** generated traces are stored under `build/` to keep them out of source control.
- **Commands:** exact commands and step-by-step instructions live in the profiling docs.
