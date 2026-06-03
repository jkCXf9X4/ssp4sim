# Module: graph
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-GRAPH-001 -->

## Purpose

Organizes invocable nodes into a directed acyclic graph and dispatches simulation steps to the active executor. Bridges analysis results to executable runtime nodes by **flattening** the hierarchical analysis graph into a flat model-to-model dependency graph.

## Key Components

- `Graph`: Manages the DAG of invocable nodes and delegates step execution to the selected executor.
- `GraphBuilder`: Constructs executable graph from analysis results and recorder hooks.
    - Only creates `FmuModel` instances for `ModelKind::fmu` analysis models (skips `ModelKind::system`).
    - During `wire_connections()`, skips system-boundary connections (those involving `ModelKind::system` as source or target). Future enhancement: flatten hierarchical connections through system boundaries.
    - Feedthrough metadata (`is_feedthrough`) is populated on each `ConnectionInfo` from `AnalysisConnector::is_feedthrough`.
- `AsyncNode`: Wraps an invocable object in a dedicated worker thread.
- `InvocableNode`: Interface marker for graph nodes that can be scheduled.
- `Invocable`: Interface for objects with init/invoke entry points.

## Include Boundary

- Path: `lib/include/graph/` (6 files) + `lib/include/graph/analysis/` (13 files)

## Dependencies

- `analysis/` — for AnalysisGraph input (consumes hierarchical model/connector/connection maps)
- `execution/` — for ExecutionBase, Jacobi, Seidel
- `model/` — for FmuModel as invocable nodes
- `signal/` — for DataRecorder hooks during graph building
- `utils/` — for node/edge iteration utilities

## Notable Patterns

- Graph is a DAG of sub-graphs → DAGs of models.
- The execution algorithm could be injected in the constructor (currently set during build).
- AsyncNode enables parallel FMU execution on dedicated threads.
- GraphBuilder skips system models — only FMU components become runtime nodes.

## Traceability

- Backward: Architecture component view (graph/ module area), AD-003 (graph responsibility separation).
- Sources: `lib/include/graph/`, `lib/include/graph/graph.md`.