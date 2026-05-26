# Interfaces
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-INTERFACES-001 -->

## Description

This artifact documents the internal and public API contracts of SSP4SIM.

## Public API Boundary

Defined in `lib/public_include/`:

| Header | Description |
|---|---|
| `simulator.hpp` | C++ API — Simulator class with init/simulate lifecycle |
| `simulator_c_api.h` | C API — C-compatible wrapper for embedding |
| `shared_config.hpp` | Config struct — shared between public API and internal runtime |

The public API is intentionally minimal: create a Simulator with a config path, call init(), call simulate().

## Key Internal Interfaces

| Interface | File | Purpose |
|---|---|---|
| `Invocable` | `lib/include/execution/invocable.hpp` | Abstract interface with init() and invoke(data). Implemented by FmuModel and other invocable objects. |
| `InvocableNode` | `lib/include/graph/graph.hpp` | Interface marker for graph nodes that can be scheduled. |
| `ExecutionBase` | `lib/include/execution/executor.hpp` | Abstract executor — manages model vector, defines executor interface for Jacobi/Seidel variants. |

## Config Interface

Config flows through:
1. JSON file → `Config::loadFromFile()` → internal Config object
2. Runtime code reads values via dotted keys: `simulation.ssp`, `simulation.executor.method`, etc.
3. `get*()` for required keys (throws if missing), `getOr()` for optional keys (returns default)
4. Missing key handling: rejected for required, default for optional

## Recording Interface

- `DataRecorder` exposes `add_storage()` and `add_sink()` for runtime setup
- `RecorderSink` abstract interface: `start()`, `on_event()`, `stop()`
- Two implementations: `CsvRecorderSink`, `DuckDBRecorderSink`

## Traceability

- Backward: Architecture container view (container boundaries).
- Sources: `lib/public_include/`, `lib/include/execution/invocable.hpp`, `lib/include/graph/graph.hpp`.