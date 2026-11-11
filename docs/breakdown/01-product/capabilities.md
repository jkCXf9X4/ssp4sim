---
id: CP-001
type: capability
title: Capabilities
summary: The promised capabilities of SSP4SIM, one labeled fact per capability
date: 2026-11-11
status: current
---

# Capabilities

Each capability is a promise from the user's perspective; requirements and use cases implement one or more.

- **SSP Archive Loading:** load and parse SSP 1.0 archives and unpacked SSP directories (`REQ-001`).
- **FMI 2.0 Co-Simulation:** execute FMI 2.0 co-simulation models through the FMU adapter layer (`REQ-002`).
- **Multiple Execution Strategies:** Gauss-Jacobi and Gauss-Seidel with parallel and sequential variants (`REQ-003`).
- **CSV Recording:** record simulation results to CSV with configurable sample interval (`REQ-004`).
- **Local Database Recording:** record results to a local database with per-storage tables; SQLite WAL backend (`REQ-005`).
- **Multiple API Surface:** CLI, C API, and Python bindings (`REQ-009`).
- **Local Logging Tiers:** terminal/file overview, JSON archival, cutelog live navigation, hot-path compile-time disable (`REQ-006`).
- **Distributed Observability:** export observability to OpenTelemetry for distributed simulation tracing (`REQ-007`).
