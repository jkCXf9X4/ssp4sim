# Module: model
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-MODEL-001 -->

## Purpose

Adapts FMUs into graph-invocable simulation models with input and output signal storage. Defines connection topology for signal exchange.

## Key Components

- `FmuModel`: Wraps an FMU instance with signal buffers, connection info, and invocation logic. Implements the Invocable interface.
- `ModelConnection`: Represents a connection between an output connector and an input connector.
    - `ConnectionInfo::is_feedthrough`: Copied from `AnalysisConnector::is_feedthrough` during graph building. Set during analysis phase by BFS traversal of FMU internal dependency graph; delayed connections (`delay > 0`) are forced to `false`.
- `ModelConnector`: Represents a typed connector (input or output) on a model.
- `InitialValue`: Handles initial value processing for signal areas.

## Include Boundary

- Path: `lib/include/model/`
- 8 files covering model FMU, connections, connectors, and initial values.

## Dependencies

- `handler/` — for FmuInfo and FMU access
- `signal/` — for SignalStorage buffer management
- `graph/` — for Invocable interface

## Notable Patterns

- FmuModel bridges the handler's FMU lifecycle to the graph's invocation loop.
- Models read inputs from and write outputs to SignalStorage areas.
- Connection resolution supports complex SSP topologies (hierarchical connectors).

## Traceability

- Backward: Architecture component view (model/ module area).
- Sources: `lib/include/model/`, `lib/class_description.md`.