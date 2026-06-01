# Scope
<!-- Layer: 01-product -->
<!-- Stable ID: PRO-SCOPE-001 -->

## Description

This artifact defines what is in scope and explicitly out of scope for SSP4SIM. It guides feature decisions by making boundaries clear.

## In Scope

- SSP 1.0 archives and unpacked SSP directories
- FMI 2.0 co-simulation models
- Gauss-Jacobi and Gauss-Seidel execution strategies
- Parallel and sequential executor variants
- Local database recording output with per-storage tables
- CSV export output with configurable sample interval
- C++23 library API, C API, and Python bindings
- Local logging with terminal, file, JSON, and cutelog tiers
- Linux x86_64 as primary platform

## Out of Scope

- FMI 2.0 model exchange (co-simulation only)
- GUI, dashboard, or web interface
- Windows or macOS platform support
- SSP 2.0 support
- Real-time simulation guarantees
- Remote database ingestion and streaming output in current releases
- Distributed observability via OpenTelemetry in current releases
- Formal support or SLAs

## Traceability

- Backward: Traces to constraints documented in `product-breakdown/00-intent/constraints.md`.
- Sources: `readme.md` (features list), `docs/overview.md` (notable characteristics), `docs/usage.md` (result artifacts).
