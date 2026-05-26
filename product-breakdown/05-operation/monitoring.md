# Monitoring
<!-- Layer: 05-operation -->
<!-- Stable ID: OPS-MONITORING-001 -->

## Description

This artifact provides operator-facing guidance for observing and diagnosing SSP4SIM behavior during runtime.

## Log Levels

| Level | When to Use |
|---|---|
| INFO | Normal operation — startup, shutdown, completed simulation |
| WARNING | Unexpected conditions — dropped recorder events, questionable config |
| ERROR | Operation failures — FMU step error, recorder write failure |
| CRITICAL | Startup failure, unrecoverable state |

## Sink Configuration

| Sink | Use Case |
|---|---|
| Terminal | Interactive debugging |
| File | Persistent logging for post-hoc analysis |
| JSON | Structured log consumption by tooling |
| cutelog | Real-time remote log viewing |

## Key Events to Watch

- "Selected scenario: {ssp}" — confirms correct SSP loaded
- "Simulation completed" — normal completion
- "Rate-limited warning: recorder event dropped" — backpressure indicator
- "fmi2Error / fmi2Fatal" — FMU failure, check FMU configuration and models

## Performance Indicators

- Step timing: roughly proportional to `(stop_time - start_time) / timestep * FMU count`
- Recording throughput: bounded by buffer sizes (50 per storage, 4096 event queue)
- Thread usage: configurable via `executor.thread_pool_workers`

## Traceability

- Backward: Implementation logging infrastructure (`product-breakdown/03-implementation/modules/signal.md`).
- Sources: `docs/logging_guidlines.md`, `docs/profiling.md`.
