# AD-003: Graph Responsibility Separation
<!-- Layer: 02-architecture -->
<!-- Status: Superseded by AD-004 -->

> **Status: Superseded by AD-004** — See [AD-004-analysis-system-responsibilities.md](AD-004-analysis-system-responsibilities.md) for the current architecture decision covering the analysis data model, recursive building, boundary connection handling, and the builder interface.

## Context

The original codebase had a single `AnalysisGraph` that served both as a structural mirror of the SSP XML and as an intermediate step toward building the execution graph. This created two problems:

1. **Nested systems were not parseable**: The `AnalysisGraphBuilder` only read top-level `Components` and `Connections`, silently skipping system-level connectors and boundary connections.
2. **Unclear separation**: It was not well-defined what belonged in the analysis graph vs. the simulation graph, leading to mixed responsibilities.

The `dcmotor` reference SSP (3 levels of nesting: DC-Motor → SuT → edrive_mass/emachine_model) demonstrated the need for hierarchical system support.

## Decision

Introduce a clear separation of responsibilities between two graph layers:

### Analysis Graph (`analysis_graph`)

**Responsibility**: Parse all information relevant from the SSP and store it in local graph nodes.

- Nodes exist for **models** (both FMU `Component` and nested `System`), **connectors** (variable-level), **connections** (with delay information), and **internal variables** (for algebraic loop detection via Tarjan's SCC).
- The analysis graph mirrors the SSP's hierarchical structure: system models contain sub-models via `AnalysisModel::sub_models`.
- **No model-to-model edges** — this is an enforced invariant, not a temporary stage. Model-to-model edges are never built in the analysis graph. Connectors point to their owning model via `connector.model`.
- Connector naming uses hierarchical paths (e.g., `"SuT.edrive_mass.M_A"`) for unambiguous resolution.

### Simulation Graph (`graph`)

**Responsibility**: Set up models, storage areas, and the runtime execution graph from the analysis graph.

- Creates the final **model-to-model connection graph** by flattening the analysis graph's hierarchy.
- Only FMU components (`ModelKind::fmu`) become runtime `FmuModel` instances — system nodes (`ModelKind::system`) are structural containers, not executable.
- `GraphBuilder::wire_connections()` transforms hierarchical connections into direct FMU→FMU edges.

### Graph Builders

**Responsibility**: Create and connect up the graph structures.

- `AnalysisGraphBuilder` performs recursive SSP traversal, producing a flat map of all models/connectors/connections plus hierarchical parent-child relationships.
- `GraphBuilder` consumes the analysis graph and produces the runtime execution graph, skipping system models and transforming hierarchical connections.

### Graph Objects

**Responsibility**: Self-analysis.

- Each graph node is responsible for its own analysis: `to_string()`, SCC computation, feedthrough queries.
- Objects expose their structure for analysis but do not mutate their own topology.

## Rationale

- Recursive traversal maps naturally to the SSP schema: `TSystem → Elements → {Components, Systems}`.
- Avoiding model-to-model edges in the analysis graph keeps it close to the SSP structure and avoids premature flattening.
- The simulation graph is the sole place where model-to-model dependency edges are created, ensuring a single source of truth for execution ordering.
- System connector models (e.g., `"SuT.U"`) provide a natural resolution point for boundary connections.

## Consequences

- The analysis graph now supports nested SSPs with arbitrary nesting depth.
- `connect_fmus()` has been removed from `AnalysisGraphBuilder`. Model-to-model edges are now derived in `GraphBuilder::derive_model_edges()` from the analysis graph's connection set, ensuring a single source of truth.
- Backward compatibility is preserved: flat SSPs (no nesting) produce identical analysis and simulation graphs.
- The free functions `create_models()`, `create_connectors()`, `create_connections()` are now dead code and can be removed in a follow-up cleanup.
- New recursive helper functions (`get_all_resources()`, `get_all_fmu_connections()`) were added to the SSP extension layer.

## Traceability

- Backward: Quality attributes — maintainability (clear separation), extensibility (nested system support).
- Sources: `lib/include/graph/analysis/analysis_graph_builder.cpp`, `lib/include/graph/graph_builder.cpp`, `lib/include/graph/analysis/components/analysis_model.hpp`.
- Tests: `tests/lib/graph/test_analysis_graph_components.cpp` (component data types and builder instantiation).