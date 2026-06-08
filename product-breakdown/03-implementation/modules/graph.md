# Module: graph
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-GRAPH-002 -->

## Purpose

Organizes invocable nodes into a directed acyclic graph and dispatches simulation steps to the active executor. Bridges analysis results to executable runtime nodes by **flattening** the hierarchical analysis system into a flat model-to-model dependency graph.

## Key Components

- `Graph`: Manages the DAG of invocable nodes and delegates step execution to the selected executor.
- `GraphBuilder`: Constructs executable graph from an `AnalysisSystem` and recorder hooks.
    - Accepts `const analysis::AnalysisSystem&` (no longer `AnalysisGraph*`).
    - Only creates `FmuModel` instances for models that have a non-null `fmu` field (skips system containers).
    - `create_fmu_models()`: iterates `analysis_system.get_all_models()` to find FMU components.
    - `create_data_storage_areas()`: iterates model connectors (now `std::vector`, not `std::map`) and creates `ConnectorInfo` entries.
    - `wire_connections()`: iterates `analysis_system.get_all_connections()`, resolves source/target by string names, skips boundary-crossing connections, and reads `is_feedthrough` from analysis connectors.
    - `derive_model_edges()`: iterates connections to build unique model-to-model dependency pairs.
- `AsyncNode`: Wraps an invocable object in a dedicated worker thread.
- `InvocableNode`: Interface marker for graph nodes that can be scheduled.
- `Invocable`: Interface for objects with init/invoke entry points.

## Include Boundary

- Path: `lib/include/graph/` (6 files)
- No longer includes `lib/include/graph/analysis/` (deprecated).

## Dependencies

- `analysis/` — for `AnalysisSystem` input (consumes hierarchical model/connector/connection vectors). The include boundary is `analysis/analysis_system.hpp`, `analysis/analysis_model.hpp`, `analysis/analysis_connector.hpp`, `analysis/analysis_connection.hpp`.
- `execution/` — for ExecutionBase, Jacobi, Seidel
- `model/` — for FmuModel as invocable nodes
- `signal/` — for DataRecorder hooks during graph building
- `utils/` — for node/edge iteration utilities

## Notable Patterns

- Graph is a DAG of sub-graphs → DAGs of models.
- The execution algorithm could be injected in the constructor (currently set during build).
- AsyncNode enables parallel FMU execution on dedicated threads.
- GraphBuilder skips system containers — only FMU components become runtime nodes.
- Boundary-crossing connections are skipped during wiring (not flattened yet — future enhancement).
- Connector resolution uses string-based names: `connection->source_model + "." + connection->source_connector`.

## Traceability

- Backward: Architecture component view (graph/ module area), AD-004 (analysis system responsibilities).
- Sources: `lib/include/graph/`, `lib/include/graph/graph_builder.hpp`, `lib/include/graph/graph_builder.cpp`.
- Consumer: `Simulation::init()` passes `AnalysisSystem` to `GraphBuilder`.