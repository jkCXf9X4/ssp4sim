# Module: graph
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-GRAPH-001 -->

## Purpose

Organizes invocable nodes into a directed acyclic graph and dispatches simulation steps to the active executor. Bridges analysis results to executable runtime nodes.

## Key Components

- `Graph`: Manages the DAG of invocable nodes and delegates step execution to the selected executor.
- `GraphBuilder`: Constructs executable graph from analysis results and recorder hooks.
    - During `wire_connections()`, feedthrough metadata (`is_feedthrough`) is populated on each `ConnectionInfo` from `AnalysisConnector::is_feedthrough`, which is set during analysis graph building by `AnalysisGraphBuilder::compute_feedthrough()`. This method walks the FMU internal dependency graph (edges from `wire_internal_dependencies()`) using BFS from each output connector, marking it feedthrough if an input connector of the same FMU is reachable. Delayed connections (`delay > 0`) override feedthrough to `false`.
- `AsyncNode`: Wraps an invocable object in a dedicated worker thread.
- `InvocableNode`: Interface marker for graph nodes that can be scheduled.
- `Invocable`: Interface for objects with init/invoke entry points.

## Include Boundary

- Path: `lib/include/graph/` (6 files) + `lib/include/graph/analysis/` (13 files)

## Dependencies

- `analysis/` — for AnalysisGraph input
- `execution/` — for ExecutionBase, Jacobi, Seidel
- `model/` — for FmuModel as invocable nodes
- `signal/` — for DataRecorder hooks during graph building
- `utils/` — for node/edge iteration utilities

## Notable Patterns

- Graph is a DAG of sub-graphs → DAGs of models.
- The execution algorithm could be injected in the constructor (currently set during build).
- AsyncNode enables parallel FMU execution on dedicated threads.

## Traceability

- Backward: Architecture component view (graph/ module area).
- Sources: `lib/include/graph/`, `lib/include/graph/graph.md`.