# Quality Attributes
<!-- Layer: 02-architecture -->
<!-- Stable ID: ARCH-QUALITY-001 -->

## Description

This artifact documents the non-functional characteristics that shape SSP4SIM architecture and design decisions.

## Performance

- **No formal performance targets.** The project is experimental; performance is "best effort" within the chosen architectural patterns.
- Parallel executor variants (Jacobi parallel with futures, spin-wait, TBB) exist for experimentation, not production benchmarks.
- Hot-path logging is compile-time optional to avoid runtime overhead.
- Recorder has bounded buffers (50 per storage, 4096 event queue) with optional backpressure.

## Scalability

- **Single-node only.** No distributed or multi-node simulation support.
- Thread count is configurable via `executor.thread_pool_workers`, not applicable for all pools, see code for current status.
- FMU count is bounded only by available memory and thread pool size.
- Recorder backpressure model limits data throughput when recording cannot keep up.

## Reliability

- **Experimental quality.** No formal reliability targets.
- FMU step failures handled gracefully (cleanup without fmi2Terminate).
- Recorder buffer overflow drops events with rate-limited warning.
- `ctest` integration is unreliable — test binary must be run directly.

## Security

- **No security model.** FMU vendor code runs with caller's privileges (dlopen, no sandboxing).
- Config files and SSP archives are treated as trusted input.
- No network exposure in the simulation engine.

## Maintainability

- Compact codebase with narrow runtime surface (project goal).
- Module areas with clear boundaries (analysis, execution, graph, handler, model, signal, utils).
- No enforced formatter — match surrounding style.
- Active experimental software: root-cause fixes preferred over compatibility shims.

## Traceability

- Backward: Traces to constraints in `product-breakdown/00-intent/constraints.md`.
- Sources: `readme.md` (project description), `docs/development.md` (coding style), `docs/logging_guidlines.md`, `docs/profiling.md`.
