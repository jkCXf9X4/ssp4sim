# AD-004: AnalysisSystem — Responsibilities and Data Model Separation
<!-- Layer: 02-architecture -->
<!-- Status: Accepted -->

## Context

The original `AnalysisGraph` (defined in `lib/include/graph/analysis/analysis_graph.hpp`) served dual purpose: it was both a structural mirror of the SSP XML and an intermediate graph for building the execution graph. This created several problems:

1. **Tight coupling to `utils::graph::Node`**: All data objects (`AnalysisModel`, `AnalysisConnector`, `AnalysisConnection`, `AnalysisModelVariable`) inherited from `utils::graph::Node`, coupling plain data to graph traversal concerns.

2. **Nested SSP support was fragile**: The old `AnalysisGraphBuilder` only read top-level `Components` and `Connections`, silently skipping system-level connectors and boundary connections.

3. **`std::map`-based indexing**: Models, connectors, and connections were stored in `std::map<std::string, unique_ptr<…>>`, which was unnecessary for the object model and made iteration awkward.

4. **Raw back-pointers**: `AnalysisConnector` had a `model` raw pointer back to its owning model, and `AnalysisConnection` had raw pointers to source/target connectors and models.

5. **Builder had no path-based entry point**: The old code required `Ssp*` and `FmuHandler*` to be pre-created, making standalone use outside the simulation pipeline difficult.

## Decision

Introduce a new `AnalysisSystem` data model in namespace `ssp4sim::analysis` with the following design decisions:

### (a) Standalone class hierarchy (no Node/IWritable inheritance)

- `AnalysisSystem`, `AnalysisModel`, `AnalysisConnector`, `AnalysisConnection`, and `AnalysisModelVariable` are plain data classes.
- No inheritance from `utils::graph::Node` or `types::IWritable`.
- Graph traversal (SCC, loop detection) is done via a transient `AnalysisGraphView` struct that collects `utils::graph::Node*` pointers from the relevant objects when needed.

### (b) `std::vector`-based storage

- Models, connectors, connections, and nested systems are stored in `std::vector<std::unique_ptr<…>>` instead of `std::map`.
- Public accessor methods (`get_all_models()`, `get_all_connections()`) return `std::vector<T*>` for iteration.
- Nested systems are stored in `nested_systems` with recursive collection via `get_all_models()` and `get_all_connections()`.

### (c) String-based references (no raw pointers)

- `AnalysisConnection` stores source/target model and connector names as strings, not raw pointers.
- `AnalysisConnector` has no `model` back-pointer.
- Connector resolution (`get_connector()`) uses dot-separated path traversal.

### (d) Boundary connector support

- System-level connectors (on `<ssd:Connector>` inside `<ssd:System>`) are stored on the `AnalysisSystem::connectors` vector with `is_boundary=true`.
- Boundary-crossing connections (where `startElement` or `endElement` is absent) are tagged with `is_boundary_crossing=true`.

### (e) Recursive building with two entry points

- `AnalysisSystemBuilder::build(const std::string& ssp_path)` — standalone path-based entry.
- `AnalysisSystemBuilder::build(Ssp*, FmuHandler*)` — pipeline entry for use from `Simulation::init()`.
- Recursive traversal: walks `Elements.Components` and `Elements.Systems` at every level.

### (f) Feedthrough and SCC as external operations

- `build_analysis_graph()` creates a transient graph view from connectors and model variables.
- `detect_algebraic_loops()` calls Tarjan's SCC on the graph view.
- Feedthrough is stored as `AnalysisConnector::is_feedthrough` and populated by the old `analysis_graph_builder.cpp` or by pipeline components during simulation graph construction.

## Rationale

- Removing Node inheritance decouples data modeling from graph algorithms. SCC and feedthrough analysis are now explicit operations on transient views, not intrinsic properties of the data objects.
- Vector storage simplifies iteration. Recursive collection flattens the hierarchy for flat iterations while preserving the nested structure.
- String-based references eliminate the need for pointer-wiring passes and make serialization trivial.
- Boundary connector support enables correct handling of nested SSPs (e.g., the `dcmotor` reference SSP with 3 levels of nesting).
- Two builder entry points support both standalone analysis and pipeline integration without duplication.

## Consequences

- `AnalysisGraph` and the old data classes under `lib/include/graph/analysis/components/` are deprecated but preserved for backward compatibility.
- `GraphBuilder` now accepts `const AnalysisSystem&` instead of `AnalysisGraph*`.
- `Simulation::init()` creates an `AnalysisSystem` via `AnalysisSystemBuilder` and passes it to `GraphBuilder`.
- Existing tests continue to pass without modification because the old classes remain compilable.
- The `AnalysisSystemBuilder` constructs its own `Ssp` and `FmuHandler` when called with a path, eliminating the need for the caller to set up these dependencies.
- Future work: move feedthrough computation into the analysis pipeline so it can be reused by both the simulation pipeline and standalone analysis.

## Traceability

- Backward: Supersedes AD-003 (graph responsibility separation) for the analysis layer.
- Sources: `lib/include/analysis/`, `lib/include/analysis/analysis_system_builder.hpp`, `lib/include/graph/graph_builder.hpp`, `lib/include/simulation.cpp`.
- Tests: All 58 existing test cases pass; new tests in `tests/lib/analysis/`.