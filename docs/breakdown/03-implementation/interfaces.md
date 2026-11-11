---
id: INFO-019
type: info
title: Interfaces
summary: Public and internal API contracts of SSP4SIM
date: 2026-11-11
status: current
---

# Interfaces

A deliberately minimal public API plus a small set of internal interface contracts.

- **Public API:** `lib/public_include/` holds `simulator.hpp` (C++ Simulator with init/simulate lifecycle), `simulator_c_api.h` (C wrapper for embedding), `shared_config.hpp` (config struct shared between public API and runtime); the surface is minimal — create a Simulator from a config path, call init, call simulate.
- **Invocable:** `lib/include/execution/invocable.hpp` — abstract init()/invoke(data); implemented by FmuModel and other invocable objects.
- **ExecutionBase:** `lib/include/execution/executor.hpp` — abstract executor managing the model vector; the interface for Jacobi/Seidel variants (`INFO-023`).
- **Graph nodes:** `InvocableNode` marker interface for schedulable graph nodes, `lib/include/graph/graph.hpp`.
- **Recorder interface:** `DataRecorder::add_storage()` and `add_sink()`; `RecorderSink` abstract `start()`/`on_event()`/`stop()`; implementations `CsvRecorderSink` and `SqliteWALRecorderSink` — signal module (`INFO-024`).
- **Boundary:** these contracts satisfy the container boundaries (`INFO-011`); the config access pattern is the configuration leaf (`INFO-017`).
