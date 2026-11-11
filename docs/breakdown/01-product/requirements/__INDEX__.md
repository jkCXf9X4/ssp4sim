---
title: Requirements
summary: What the product promises, one leaf per requirement
---

# Requirements

## Contents

<!-- pb:index:start -->
<!-- GENERATED FILE — do not edit. Regenerate with `pb registers`. -->
- **REQ-001** [REQ-001: SSP 1.0 Archive Loading](REQ-001.md) — The engine must load and parse SSP 1.0 archives and unpacked SSP directories
- **REQ-002** [REQ-002: FMI 2.0 Co-Simulation Model Lifecycle](REQ-002.md) — The engine must instantiate, step, and terminate FMI 2.0 co-simulation FMUs through the complete lifecycle
- **REQ-003** [REQ-003: Configurable Execution Strategy](REQ-003.md) — The engine must support selecting the execution strategy at runtime through configuration
- **REQ-004** [REQ-004: CSV Output with Configurable Sample Interval](REQ-004.md) — The engine must support CSV export with a configurable sample interval
- **REQ-005** [REQ-005: Local Database Output with Per-Storage Tables](REQ-005.md) — The engine must support recording simulation results to a local database with per-storage tables
- **REQ-006** [REQ-006: User-Configurable Local Logging Levels and Sinks](REQ-006.md) — The engine must provide configurable local logging with multiple levels and sink types
- **REQ-007** [REQ-007: Distributed Observability via OpenTelemetry](REQ-007.md) — The engine must provide a distributed observability export path using OpenTelemetry for simulations spanning multiple processes or services
- **REQ-009** [REQ-009: Multiple Embedding API Surfaces](REQ-009.md) — The engine must expose simulation through CLI, C API, and Python bindings driving the same init-then-simulate lifecycle
- **REQ-010** [REQ-010: Execution Strategy Variants and Tuning](REQ-010.md) — The engine must support the documented execution strategy variants and tuning knobs
- **REQ-011** [REQ-011: Realtime Wall-Clock Pacing](REQ-011.md) — The engine must support pacing simulation execution to wall-clock time
- **REQ-012** [REQ-012: Configurable Recording Behavior](REQ-012.md) — The engine must support recording input signals, backpressure control, and initial start-value logging
- **REQ-013** [REQ-013: Automatic Analysis and Execution Graph Construction](REQ-013.md) — The engine must automatically construct the simulation graph from the SSP without manual ordering configuration
- **REQ-014** [REQ-014: Configurable Output Paths with Timestamped Artifacts](REQ-014.md) — The engine must anchor default output paths to a working directory and support timestamp substitution in output paths
- **REQ-015** [REQ-015: SSP Parameter and Start-Value Application](REQ-015.md) — The engine must apply the SSP's parameterization to the simulation before initialization
<!-- pb:index:end -->
