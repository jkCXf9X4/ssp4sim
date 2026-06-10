# Analysis System (`lib/include/analysis/`)

> **Layer 1 implementation** — The analysis data model, part of the three-layer architecture (AD-005).

## Overview

The Analysis System module provides a structural, runtime-free view of an SSP's model topology. It mirrors the SSP XML hierarchy while remaining agnostic of graph traversal concerns. Key properties:

- **No `utils::graph::Node` inheritance** — data objects are plain classes.
- **`std::vector` storage** — models, connectors, and connections live in vectors, not maps.
- **String-based references** — connections use model/connector names, not raw pointers.
- **Boundary connector support** — system-level connectors are stored with `is_boundary=true`.
- **Recursive building** — `AnalysisSystemBuilder` walks nested `<ssd:System>` elements.

## Files

| File | Purpose |
|------|---------|
| `analysis_system.hpp` / `.cpp` | `AnalysisSystem` container with recursive model/connection collection |
| `analysis_system_builder.hpp` / `.cpp` | Builder with path-based and pipeline entry points |
| `analysis_model.hpp` / `.cpp` | FMU component data class |
| `analysis_connector.hpp` / `.cpp` | Variable connector data class |
| `analysis_connection.hpp` / `.cpp` | Connection wire data class |
| `analysis_model_variable.hpp` / `.cpp` | Intra-FMU variable data class |
| `analysis_graph_view.hpp` | Transient graph view for SCC/traversal |
| `analysis_graph_factory.hpp` / `.cpp` | Layer 2 graph factory — builds transient graphs for algebraic loop detection |

## Key API

```cpp
// Build an analysis system from a path
auto sys = analysis::AnalysisSystemBuilder().build("path/to/ssp.ssp");

// Recursive access
auto all_models = sys->get_all_models();
auto all_connections = sys->get_all_connections();

// Algebraic loop detection (delegates to AnalysisGraphFactory)
auto sccs = sys->detect_algebraic_loops();

// Connector resolution by path
auto conn = sys->get_connector("SuT", "SuT.edrive_mass.M_A");
```

## Architecture

See AD-005 (three-layer architecture) in `product-breakdown/02-architecture/decisions/`.