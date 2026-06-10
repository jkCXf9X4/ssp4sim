# AD-005: Three-Layer Architecture — Analysis System, Analysis Graph, and Simulation Graph Separation
<!-- Layer: 02-architecture -->
<!-- Status: Accepted -->

## Context

The analysis layer evolved through two generations:

1. **First generation (pre-IMP-039)**: A flat `AnalysisGraph` (under `lib/include/graph/analysis/`) that served dual purpose as both an SSP structural mirror and an intermediate graph for building the execution graph. All data objects inherited from `utils::graph::Node`, coupling data modeling to graph algorithms.

2. **Second generation (IMP-039/IMP-040)**: A hierarchical `AnalysisSystem` (under `lib/include/analysis/`) with plain data classes, vector storage, string-based references, and recursive nesting support. This addressed the coupling and flatness problems of the first generation.

The second generation introduced the concept of three distinct layers, which AD-005 formalizes:

- **Layer 1 (Analysis System Layer)**: Complete SSP ingestion, configuration inference, and pure-data handoff.
- **Layer 2 (Analysis Graph Layer)**: Transient graph factory that projects views from the fully-loaded Layer 1 data.
- **Layer 3 (Simulation Graph Layer)**: Existing simulation graph that consumes analysis data via `GraphBuilder`.

## Decision

### Layer 1 — AnalysisSystem (Pure Data Handoff)

- `AnalysisSystem` is the fully-loaded, runtime-free view of the SSP model topology.
- It mirrors the SSP XML hierarchy while remaining agnostic of graph traversal concerns.
- Key properties:
  - Plain data classes (`AnalysisModel`, `AnalysisConnector`, `AnalysisConnection`, `AnalysisModelVariable`).
  - **No** `utils::graph::Node` inheritance — objects are graph-agnostic.
  - `std::vector` storage for models, connectors, connections, and nested systems.
  - String-based references — connections use model/connector names, not raw pointers.
  - Boundary connector support — system-level connectors stored with `is_boundary=true`.
  - Recursive building — `AnalysisSystemBuilder` walks nested `<ssd:System>` elements.
  - Always loads the full SSP: all systems, components, connectors, connections, parameters, FMU model descriptions, variables, and dependency data.
- The handoff boundary is formalized: Layer 2 receives `const AnalysisSystem&` as its input after Layer 1 completes loading.
- Builder entry points:
  - `build(const std::string& ssp_path)` — standalone path-based entry (creates own `Ssp` and `FmuHandler`).
  - `build(Ssp*, FmuHandler*)` — pipeline entry for use from `Simulation::init()`.

### Layer 2 — AnalysisGraphFactory (Transient Graph Views)

- `AnalysisGraphFactory` creates transient graph views from an already-loaded `AnalysisSystem`.
- It is **not** owned or stored — created per call and destroyed after producing results.
- Produces:
  - `build_transient_graph()` — builds a connection-graph of `AnalysisNode` objects.
  - `find_algebraic_loops()` — runs Tarjan's SCC and returns cycles (SCCs with size > 1).
  - `AnalysisGraphView` — lightweight struct with a `std::vector<utils::graph::Node*>` for SCC/traversal.
- `AnalysisNode` inherits from `utils::graph::Node` for Tarjan SCC compatibility:
  - Wraps a connector (`is_connector=true`) or variable via a `source` pointer.
  - Transient — destroyed after the factory call completes.
- `AnalysisSystem::detect_algebraic_loops()` internally creates an `AnalysisGraphFactory` and delegates. This is the orchestrating method on Layer 1 that calls Layer 2.
- Feedthrough stays in Layer 3 (simulation graph) — the old `AnalysisGraphBuilder::compute_feedthrough()` is preserved in `graph/analysis/` for backward compatibility.

### Layer 3 — Simulation Graph (Existing, Unchanged)

- `GraphBuilder` consumes `const AnalysisSystem&` as its analysis input.
- No structural changes — the existing simulation graph is unchanged.
- `GraphBuilder::create_fmu_models()`, `wire_connections()`, and `derive_model_edges()` use `get_all_models()`, `get_all_connections()`, and `find_connector()` from the AnalysisSystem API.
- Feedthrough is computed during simulation graph construction, not in the analysis layer.

### SCC Delegation Pattern

```
AnalysisSystem::detect_algebraic_loops()
  → creates AnalysisGraphFactory(system)
  → factory.build_transient_graph()
  → factory.find_algebraic_loops()
  → returns sccs (vector of cycles)
```

The factory is transient — no persistent state is stored on `AnalysisSystem` for graph operations.

### Eager-Loading Contract

| SSP Part | Layer 1 Requirement |
|---|---|
| SSD structure | Load completely before returning AnalysisSystem |
| Systems and nested systems | Load recursively |
| Components | Load every component in every system |
| Component and system connectors | Load every connector |
| Connection topology | Load every connection at every system level |
| Parameter bindings and initial values | Resolve during AnalysisSystem construction |
| FMU modelDescription.xml | Load for every FMU component |
| Model variables | Load every variable for every FMU component |
| Dependency lists | Load dependency data needed by graph and algebraic-loop analysis |

## Rationale

1. **Separation of concerns**: Layer 1 owns data modeling and SSP ingestion. Layer 2 owns graph algorithms. Layer 3 owns simulation construction. No single layer mixes responsibilities.

2. **Testability**: The pure-data Layer 1 handoff (`const AnalysisSystem&`) is independently testable without graph objects. Graph algorithm tests (Layer 2) can use pre-built `AnalysisSystem` fixtures.

3. **Transient graph lifetime**: Graph nodes are created per-operation and destroyed afterward. No long-lived graph state leaks into the analysis data model.

4. **SCC delegation is pragmatic**: `AnalysisSystem::detect_algebraic_loops()` is the public API entry point (discoverable on the system object), but the implementation lives in the factory. This gives users a single entry point while keeping graph code isolated.

5. **Feedthrough stays in Layer 3**: Feedthrough computation requires connection-level data from the simulation graph builder. Moving it to Layer 2 would duplicate logic or introduce a dependency from analysis to simulation types.

## Consequences

- `AnalysisSystem` (Layer 1) plus `AnalysisGraphFactory`/`AnalysisGraphView` (Layer 2) replace the old `AnalysisGraph` entirely.
- Old files under `lib/include/graph/analysis/` are deleted (superseded by new implementation).
- `AnalysisSystem` exposes `detect_algebraic_loops()` which delegates to `AnalysisGraphFactory`.
- `AnalysisSystem` exposes `build_analysis_graph()` which returns an `AnalysisGraphView`.
- `GraphBuilder` (Layer 3) receives `const AnalysisSystem&` and uses its query API.
- The `AnalysisNode` type (Layer 2 transient) inherits from `utils::graph::Node` — this is acceptable because the nodes are short-lived and never escape the factory.
- Sub-object creation may be mixed between the builder and analysis objects where that keeps code maintainable; the invariant is that the handoff is complete before Layer 2 runs.

### File Changes

| File | Change |
|---|---|
| `lib/include/analysis/components/analysis_system.hpp` | Declares `detect_algebraic_loops()`, `build_analysis_graph()`, all query methods |
| `lib/include/analysis/components/analysis_system.cpp` | Implements all methods; `detect_algebraic_loops()` creates transient `AnalysisGraphFactory` |
| `lib/include/analysis/analysis_graph_factory.hpp` | New — Layer 2 factory class |
| `lib/include/analysis/analysis_graph_factory.cpp` | New — `build_transient_graph()`, `find_algebraic_loops()` |
| `lib/include/analysis/analysis_graph_view.hpp` | Clean — removed commented-out code, lightweight struct |
| `lib/include/analysis/analysis_system_builder.hpp` | Added `build(const std::string& ssp_path)` |
| `lib/include/analysis/analysis_system_builder.cpp` | Implemented path-based build; removed broken `override_start_values()` |
| `lib/include/schema_extensions/SSP1_SystemStructureDescription_Ext.hpp` | Fixed `walk_system`/`walk_component` templates |
| `lib/include/schema_extensions/SSP1_SystemStructureParameter_Ext.cpp` | Fixed type references |
| `lib/include/graph/analysis/*` | Deleted (fully replaced) |

## Traceability

- **Supersedes**: AD-004 (AnalysisSystem Responsibilities).
- **Sources**: `lib/include/analysis/` (new module), `lib/include/graph/graph_builder.hpp`, `lib/include/simulation.cpp`.
- **Verification**: All 93 test cases pass (18642 assertions).
- **Implementation**: IMP-040.