# Analysis Graph

> **DEPRECATED** — This module is superseded by `lib/include/analysis/`. New code should use `AnalysisSystem`, `AnalysisSystemBuilder`, and related classes in namespace `ssp4sim::analysis`. See `lib/include/analysis/analysis_system.md` for the current documentation.

## Responsibilities

The analysis graph is the structural mirror of the SSP XML. It parses all SSP-relevant information
and stores it in local graph nodes for further processing.

The analysis graph contains:

- **Models** — both FMU Components (`ModelKind::fmu`) and nested Systems (`ModelKind::system`)
- **Connectors** — variable-level input/output/parameter connectors, pointing to their owning model via `connector.model`
- **Connections** — wires between connectors, carrying delay information
- **Internal variables** — intra-FMU variable dependencies for feedthrough detection and algebraic loop analysis

## Key Design Decisions

- **No model-to-model edges in the analysis graph.** This is an enforced invariant. The analysis graph is a pure connector/connection/variable graph. Model-to-model dependency edges are derived later in the simulation graph from the analysis graph's connection set.
- **SCC operates on the connector/variable graph** for algebraic loop detection via Tarjan's algorithm on connectors and model variables, not on model nodes.
- **Naming uses hierarchical paths.** A connector on component `edrive_mass` inside system `SuT` is
  named `"SuT.edrive_mass.M_A"`, enabling unambiguous resolution across nesting levels.
- **System models have no FMU runtime.** They are structural containers with no executable counterpart.
- **Self-analysis.** Each node exposes its structure via `to_string()` and supports graph algorithms
  (Tarjan SCC, BFS feedthrough detection) directly.

## Graph Direction Convention

`A -> B`: A is the parent of B, B is the child of A.

For connectors: `input connector -> FMU -> output connectors -> input connector`

## Builder vs Object Responsibility

- **AnalysisGraphBuilder** is responsible for creating and wiring the graph structure (recursive SSP traversal).
- **AnalysisGraph objects** are responsible for self-analysis (to_string, SCC, feedthrough queries).