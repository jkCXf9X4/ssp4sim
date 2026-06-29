# Module: analysis
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-ANALYSIS-002 -->

## Purpose

Provides a structural view of SSP models, connectors, and connections before execution. Supports **hierarchical** SSPs with arbitrary nesting depth through recursive traversal. The analysis system feeds into the graph builder to create executable nodes.

## Key Components

### AnalysisSystem (`lib/include/analysis/analysis_system.hpp`)

Container for a single SSP system node (including the root system). Contains:

- `models` — FMU components as `AnalysisModel` instances
- `connectors` — system-level (boundary) connectors with `is_boundary=true`
- `connections` — wires between connectors, with `is_boundary_crossing` for boundary connections
- `nested_systems` — child `AnalysisSystem` instances for nested `<ssd:System>` elements

**Key methods:**
- `get_all_models()` — recursively collects models from this system and all nested systems
- `get_all_connections()` — recursively collects connections
- `build_analysis_graph()` — creates a transient `AnalysisGraphView` for SCC/traversal
- `detect_algebraic_loops()` — runs Tarjan's SCC on the graph view
- `get_connector(system_path, connector_name)` — resolves a connector by dot-separated path

### AnalysisSystemBuilder (`lib/include/analysis/analysis_system_builder.hpp`)

Builds an `AnalysisSystem` hierarchy from an SSP:

- `build(const std::string& ssp_path)` — standalone path-based entry point
- `build(Ssp*, FmuHandler*)` — pipeline entry for integration with simulation

**Recursive traversal:** Walks `Elements.Components` (FMU models) and `Elements.Systems` (nested systems) at every SSP nesting level.

### AnalysisModel (`lib/include/analysis/analysis_model.hpp`)

Plain data class representing an FMU component. Fields: `name`, `type`, `source_file`, `connectors` (vector), `model_variables` (vector), `delay`, `fmu`, `canInterpolateInputs`, `maxOutputDerivativeOrder`. No Node inheritance — move-only.

### AnalysisConnector (`lib/include/analysis/analysis_connector.hpp`)

Plain data class for a variable-level connector. Fields: `name`, `type_str`, `causality`, `is_boundary`, `value_reference`, `data_type`, `size`, `initial_value`, `forward_derivatives`, `is_feedthrough`. No `model` back-pointer.

### AnalysisConnection (`lib/include/analysis/analysis_connection.hpp`)

Plain data class for a connection between two connectors. Fields are string-based: `source_model`, `source_connector`, `target_model`, `target_connector`, `delay`, `is_boundary_crossing`. No raw pointers.

### AnalysisModelVariable (`lib/include/analysis/analysis_model_variable.hpp`)

Plain data class for intra-FMU variables. Fields: `name`, `component`, `variable_name`, `type`, `value_reference`, `causality`, `variability`.

## Feedthrough Detection

Feedthrough metadata is stored as `AnalysisConnector::is_feedthrough`. The old `AnalysisGraphBuilder::compute_feedthrough()` (in `graph/analysis/analysis_graph_builder.cpp`) still handles feedthrough computation for backward compatibility. In the pipeline, feedthrough is read by `GraphBuilder::wire_connections()` during simulation graph construction.

## Include Boundary

- Path: `lib/include/analysis/`
- Files: `analysis_system.hpp`, `analysis_system.cpp`, `analysis_system_builder.hpp`, `analysis_system_builder.cpp`, `analysis_model.hpp`, `analysis_model.cpp`, `analysis_connector.hpp`, `analysis_connector.cpp`, `analysis_connection.hpp`, `analysis_connection.cpp`, `analysis_model_variable.hpp`, `analysis_model_variable.cpp`, `analysis_graph_view.hpp`

## Dependencies

- `handler/` — for `FmuInfo` access during builder construction
- `model/` — for model connection/connector types and initial values
- `utils/` — for Tarjan SCC and graph utilities
- `schema_extensions/` — for FMI model description parsing, SSP extensions, and start value handling

## Notable Patterns

- **No Node inheritance** — data objects are plain classes. Graph traversal uses transient views.
- **Vector storage** — models, connectors, connections stored in vectors, not maps.
- **String-based references** — connections reference models/connectors by name, not by pointer.
- **Recursive builder** — `AnalysisSystemBuilder` walks the full SSP hierarchy.
- **Two builder entry points** — path-based (standalone) and pipeline (Ssp/FmuHandler).
- **Boundary connectors** — system-level connectors are stored on the `AnalysisSystem` with `is_boundary=true`.
- **No feedthrough computation in builder** — builder only parses structure; feedthrough is computed separately (old `analysis_graph_builder.cpp` or pipeline).

## Traceability

- Backward: Architecture component view, AD-004 (analysis system responsibilities).
- Sources: `lib/include/analysis/`, `lib/include/analysis/analysis_system_builder.hpp`, `lib/include/graph/graph_builder.hpp`.
- Supersedes: The old analysis module (`lib/include/graph/analysis/`) is deprecated.