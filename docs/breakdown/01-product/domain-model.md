---
id: INFO-006
type: info
title: Domain Model
summary: Core domain concepts and their relationships
date: 2026-11-11
status: current
---

# Domain Model

The core concepts of the SSP4SIM problem domain and how they relate.

- **SSP Archive:** structured archive or unpacked directory conforming to SSP 1.0; holds the system structure description (SSD), parameter values (SSV/SSM), and referenced FMU binaries.
- **FMU:** packaged co-simulation model conforming to FMI 2.0; holds the model implementation, model description XML, and optional resources.
- **Execution Strategy:** algorithm that advances simulation time and coordinates FMU stepping.
  - **Gauss-Jacobi:** models step concurrently using values from the previous step.
  - **Gauss-Seidel:** models step sequentially, feeding updated values forward within the same step.
- **Recording:** pipeline that captures simulation results into CSV and SQLite WAL sinks (`INFO-009`).
- **Signal Storage:** in-memory buffers holding FMU input and output values during simulation; the recorder copies completed updates into recorder-owned buffers for output.
- **Simulation Graph:** directed acyclic graph (DAG) of invocable nodes built from analysis results and recorder hooks.
- **Concurrent-writer constraint:** only per-simulation-file SQLite WAL supports parallel execution; CSV and shared-file SQLite are unsafe for concurrent writers (`REQ-005`).
- **Relationships:** SSP Archive contains FMUs; simulation uses an execution strategy; simulation creates a recording; simulation builds an analysis graph then an execution graph; the execution graph schedules FMU models over signal storage.
