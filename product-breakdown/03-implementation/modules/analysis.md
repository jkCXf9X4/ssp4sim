# Module: analysis
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-ANALYSIS-001 -->

## Purpose

Builds a structural view of SSP models and connections before execution. The analysis graph feeds into the graph builder to create executable nodes.

## Key Components

- `AnalysisGraph`: Represents model nodes, connector definitions, and connection topology extracted from the SSP.
- `AnalysisGraphBuilder`: Transforms SSP structures (models, connectors, connections) into an AnalysisGraph.
- `AnalysisModel`, `AnalysisConnector`, `AnalysisConnection`: Domain types for the analysis representation.

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
- A researcher note exists about migrating to the LEMON graph library — currently deferred.
- The analysis graph is a read-only snapshot of the SSP structure, not a runtime execution graph.

## Traceability

- Backward: Architecture component view (`product-breakdown/02-architecture/component-view.md`).
- Sources: `lib/include/graph/analysis/`, `lib/include/graph/analysis/analysis_graph.md`.