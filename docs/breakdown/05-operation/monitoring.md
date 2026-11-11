---
id: INFO-034
type: info
title: Monitoring
summary: Runtime observability — log levels, sink tiers, key events to watch, and performance indicators
date: 2026-11-11
status: current
---

# Monitoring

Operator-facing guidance for observing and diagnosing SSP4SIM at runtime.

- **Log levels:**
  - INFO — normal operation (startup, shutdown, completed simulation).
  - WARNING — unexpected conditions (dropped recorder events, questionable config).
  - ERROR — operation failures (FMU step error, recorder write failure).
  - CRITICAL — startup failure, unrecoverable state.
- **Logging tiers:**
  - Overview — terminal and file sinks; fast human-readable run checks and operator summaries.
  - Deep analysis — JSON file; offline filtering, scriptable analysis, post-run correlation.
  - Live navigation — cutelog; interactive inspection of a live structured log stream.
  - Distributed observability — OpenTelemetry; cross-process simulation tracing and service correlation.
- **OpenTelemetry tier:** the distributed-observability contract for future multi-process simulations; separate from the current local log-sink configuration.
- **Key events to watch:**
  - "Selected scenario: {ssp}" — confirms the correct SSP loaded.
  - "Simulation completed" — normal completion.
  - "Rate-limited warning: recorder event dropped" — backpressure indicator.
  - "fmi2Error / fmi2Fatal" — FMU failure; check FMU configuration and models.
- **Performance indicators:**
  - Step timing roughly proportional to `(stop_time - start_time) / timestep * FMU count`.
  - Recording throughput bounded by buffer sizes (50 per storage, 4096 event queue).
  - Thread usage configurable via `executor.thread_pool_workers`.
