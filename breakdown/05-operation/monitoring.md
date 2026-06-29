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

## Logging Tiers

| Tier | Sink(s) | Use Case |
|---|---|---|
| Overview | terminal, file | Fast human-readable run checks and operator summaries |
| Deep analysis | JSON file | Offline filtering, scriptable analysis, and post-run correlation |
| Live navigation | cutelog | Interactive inspection of a live structured log stream |
| Distributed observability | OpenTelemetry | Cross-process simulation tracing and service correlation |

The OpenTelemetry tier is the distributed-observability contract for future
multi-process simulations. It is separate from the current local log sink
configuration.

## Sink Configuration

| Sink | Use Case |
|---|---|
| Terminal | Overview tier, interactive debugging |
| File | Overview tier, persistent logging for post-hoc analysis |
| JSON | Deep-analysis tier, structured log consumption by tooling |
| cutelog | Live navigation tier, real-time remote log viewing |

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
- Sources: `docs/logging_guidlines.md`, `docs/profiling.md`, `product-breakdown/05-operation/decisions/OD-005.md`.
