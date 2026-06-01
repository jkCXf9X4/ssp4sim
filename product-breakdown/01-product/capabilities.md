# Capabilities
<!-- Layer: 01-product -->
<!-- Stable ID: PRO-CAP-001 -->

## Description

This artifact enumerates the capabilities of SSP4SIM, each with a stable identifier. Capabilities describe what the product can do from a user's perspective.

## Capability List

| ID | Capability | Description |
|---|---|---|
| CAP-001 | SSP Archive Loading | Load and parse SSP 1.0 archives and unpacked SSP directories for simulation. |
| CAP-002 | FMI 2.0 Co-Simulation | Execute FMI 2.0 co-simulation models through the engine's FMU adapter layer. |
| CAP-003 | Multiple Execution Strategies | Support Gauss-Jacobi and Gauss-Seidel execution strategies with parallel and sequential variants. |
| CAP-004 | CSV Recording | Record simulation results to CSV with configurable sample interval. |
| CAP-005 | Local Database Recording | Record simulation results to a local database with per-storage tables. Current backend: DuckDB. |
| CAP-006 | Multiple API Surface | Expose simulation through CLI (`sim_app`), C API, and Python bindings (`pyssp4sim`). |
| CAP-007 | Local Logging Tiers | Provide configurable logging with terminal/file overview, full JSON archival logs, and cutelog live navigation, plus hot-path compile-time disable. |
| CAP-008 | Distributed Observability | Export simulation observability to OpenTelemetry for distributed simulation tracing and cross-service correlation. |

## Traceability

- Backward: Traces to intent outcomes in `product-breakdown/00-intent/outcomes.md`.
- Sources: `readme.md` (feature list), `docs/overview.md` (notable characteristics), `docs/logging_guidlines.md`, `docs/configuration.md`.
