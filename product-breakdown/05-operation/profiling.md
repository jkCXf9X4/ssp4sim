# Profiling
<!-- Layer: 05-operation -->
<!-- Stable ID: OPS-PROFILING-001 -->

## Description

Repeatable profiling commands for build-time and runtime investigation. Store generated traces under build/ to keep them out of source control.

Two profiling categories are available: build-time tracing and runtime profiling. Build-time profiling (Ninja trace, stats, and graph inspection) is used to diagnose slow compilation and link steps. Runtime profiling (via Linux perf) is used to investigate CPU-bound performance characteristics during simulation runs. Refer to `docs/profiling.md` for exact commands and step-by-step instructions.

## Traceability

- Backward: Operation monitoring guidance.
- Sources: `docs/profiling.md`.
