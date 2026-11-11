---
id: INFO-005
type: info
title: Scope
summary: The promised deliverable and its explicit boundaries
date: 2026-11-11
status: current
---

# Scope

- **In scope:**
  - SSP 1.0 archives and unpacked SSP directories
  - FMI 2.0 co-simulation models
  - Gauss-Jacobi and Gauss-Seidel execution strategies
  - Parallel and sequential executor variants
  - Local database recording output with per-storage tables
  - CSV export output with configurable sample interval
  - C++23 library API, C API, and Python bindings
  - Local logging with terminal, file, JSON, and cutelog tiers
  - Linux x86_64 as primary platform
- **Out of scope** (boundaries canonical in `CON-001`):
  - FMI 2.0 model exchange
  - GUI, dashboard, or web interface
  - Windows or macOS platform support
  - SSP 2.0 support
  - Real-time simulation guarantees
  - Remote database ingestion and streaming output in current releases
  - Distributed observability via OpenTelemetry in current releases
  - Formal support or SLAs
