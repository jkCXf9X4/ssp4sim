# Module: analysis
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-ANALYSIS-001 -->

## Purpose

Builds a structural view of SSP models, connectors, and connections before execution. Supports **hierarchical** SSPs with arbitrary nesting depth through recursive traversal. The analysis graph feeds into the graph builder to create executable nodes.

## Key Components

- `AnalysisGraph`: Represents model nodes, connector definitions, and connection topology extracted from the SSP. Contains flat maps (backward-compatible) plus hierarchical parent-child relationships.
- `AnalysisGraphBuilder`: Transforms SSP structures into an AnalysisGraph using **recursive** traversal. Walks `Elements.Components` (FMU models) and `Elements.Systems` (nested systems) at every level.
- `AnalysisModel`: Now supports both FMU components (`ModelKind::fmu`) and system nodes (`ModelKind::system`) via an enum discriminant. System models contain `sub_models` for nested children.
- `AnalysisConnector`, `AnalysisConnection`: Domain types for the analysis representation. Connections use hierarchical names for unambiguous resolution.

### Feedthrough Detection

- `AnalysisConnector::is_feedthrough`: Set by `AnalysisGraphBuilder::compute_feedthrough()` after `wire_internal_dependencies()`. Uses BFS from each output connector through the internal dependency graph; if an input connector of the same FMU is reachable, the output is marked feedthrough.

## Include Boundary

- Path: `lib/include/graph/analysis/`
- 13 files covering graph, builder, model, connector, connection, and internal utilities.

## Dependencies

- `handler/` — for FmuHandler access to FMU lifecycle during analysis
- `model/` — for model connection/connector types

## Notable Patterns

- The analysis graph direction is parent→child: A → B means A is parent of B.
- **No model-to-model edges** — this is an enforced invariant, not a temporary stage. The analysis graph is a pure connector/connection/variable graph. Model-to-model edges are derived in `GraphBuilder::derive_model_edges()` from the connection set.
- System connectors (boundary connectors on `ModelKind::system` models) use hierarchical names.
- The analysis graph is a read-only snapshot of the SSP structure, not a runtime execution graph.
- Builder handles recursive traversal; graph objects handle self-analysis.

## Traceability

- Backward: Architecture component view (`product-breakdown/02-architecture/component-view.md`), AD-003 (graph responsibility separation).
- Sources: `lib/include/graph/analysis/`, `lib/include/graph/analysis/analysis_graph.md`.