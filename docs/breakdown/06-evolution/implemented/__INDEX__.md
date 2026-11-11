---
title: Implemented
summary: Closed improvements — a lookup of what was done, never a backlog
---

# Implemented

## Contents

<!-- pb:index:start -->
<!-- GENERATED FILE — do not edit. Regenerate with `pb registers`. -->
- **IMP-001** [Deduplicate profiling content between PBS and docs](IMP-001.md) — Remove duplicated profiling commands from the operation-layer profiling leaf, keeping docs/profiling.md canonical
- **IMP-002** [Deduplicate configuration schema tables between PBS and docs](IMP-002.md) — Remove the reproduced schema reference tables from the implementation configuration leaf, keeping docs/configuration.md canonical
- **IMP-003** [Deduplicate contributing and development workflow between PBS and docs](IMP-003.md) — Replace duplicated contributing/development workflow text with a policy summary cross-linking docs/development.md
- **IMP-004** [Tighten release-pipeline PBS boundary to design-only](IMP-004.md) — Remove the concrete install command from the operation release-pipeline leaf so docs owns runnable commands
- **IMP-005** [Deduplicate generated-data policy between docs and PBS](IMP-005.md) — Replace the generated-data policy text in docs/development.md with a cross-link to the authoritative dependency-policy leaf
- **IMP-006** [Reduce docs/overview.md to a pure landing page with PBS links](IMP-006.md) — Condense product positioning in the docs overview to a landing page that links to the product layer
- **IMP-010** [Remove logging guidelines from the signal module leaf](IMP-010.md) — Delete project-wide logging sections from the signal module leaf; docs/logging_guidlines.md stays canonical
- **IMP-011** [Remove coding style rule list from the implementation decision](IMP-011.md) — Replace the enumerated style rules in the implementation decision with a cross-link to docs/development.md
- **IMP-012** [Consolidate the "ctest is unreliable" note to one location](IMP-012.md) — Keep the ctest-unreliable caveat only in tests/README.md and cross-link from the other two files
- **IMP-013** [Consolidate fmi4c mode-bit documentation to one location](IMP-013.md) — Keep the fmi4c mode-bit explanation in the dependency-policy leaf and cross-link from test docs
- **IMP-014** [Deduplicate fmi2Error/fmi2Fatal cleanup documentation](IMP-014.md) — Keep post-error cleanup behavior in tests/README.md; the handler module leaf cross-links
- **IMP-015** [Deduplicate xfail documentation between tests/README and the regressions tracker](IMP-015.md) — Keep strict xfail fixture status in the structured regressions tracker; tests/README cross-links
- **IMP-016** [Canonicalize project purpose statement to the intent layer](IMP-016.md) — Keep the purpose statement in the intent layer; readme and overview link instead of repeating it
- **IMP-017** [Deduplicate test pyramid description between tests/README and the strategy leaf](IMP-017.md) — Split strategic rationale (strategy leaf) from concrete layout (tests/README) for the three-layer test pyramid
- **IMP-018** [Fix the inline ParameterSet namespace bug in ssp1_ssd.toml](IMP-018.md) — Fix the ssd/ssv namespace prefix so inline system-level parameter sets deserialize, resolving REG-001
- **IMP-019** [Fix component-level parameter bindings in get_parameter_bindings()](IMP-019.md) — Extend parameter binding collection to traverse component-level bindings, resolving REG-002
- **IMP-022** [Internal node connections in the analysis graph](IMP-022.md) — Wire intra-FMU variable feedthrough edges into the analysis graph DAG from the dormant commented-out code
- **IMP-023** [Variable internal feedthrough storage in the sim graph](IMP-023.md) — Carry per-connection feedthrough flags into the sim graph so consumers can narrow algebraic-loop iteration
- **IMP-025** [Seidel-Jacobi hybrid executor](IMP-025.md) — Design a hybrid executor splitting the graph by Tarjan SCC; superseded by the la2 hybrid
- **IMP-027** [Config item for input storage in output artifacts](IMP-027.md) — Add an opt-in config flag so CSV/SQLite artifacts include FMU input signals alongside outputs
- **IMP-028** [Restructure thread pools with tradeoff documentation](IMP-028.md) — Document the two thread pool implementations and their tradeoffs so consumers pick correctly
- **IMP-029** [SQLite WAL local database sink](IMP-029.md) — Add a SQLite WAL local database sink for live one-writer many-reader viewer workflows
- **IMP-032** [Per-simulation SQLite database files for concurrent write parallelism](IMP-032.md) — Give each simulation run its own SQLite file so concurrent simulations do not serialize on the write lock
- **IMP-033** [Refactoring and cleanup of graph builder files](IMP-033.md) — Clean up analysis_graph_builder and graph_builder: dead code, shadowing, monolithic build methods, and consistency
- **IMP-034** [Strip the DuckDB result feature](IMP-034.md) — Remove all DuckDB recording infrastructure with no migration path; the feature was experimental
- **IMP-038** [Clarify analysis graph vs simulation graph responsibilities](IMP-038.md) — Remove model-to-model edges from the analysis graph; derive them in the sim graph; add recursive connection flattening
- **IMP-039** [Evolve the analysis graph into an AnalysisSystem with nested-SSP support](IMP-039.md) — Replace the flat AnalysisGraph with a hierarchical AnalysisSystem whose objects are plain data, enabling nested SSPs
- **IMP-040** [Three-layer architecture — Analysis System, Analysis Graph, Simulation Graph](IMP-040.md) — Complete the three-layer split with an explicit AnalysisGraph factory and eager-loading contract
- **IMP-041** [Integration test for external SSV/SSM loading](IMP-041.md) — Add ssp4sim-layer integration tests that load real SSP archives and verify external/inline parameter binding resolution
- **IMP-042** [Nested system graph support](IMP-042.md) — Resolve boundary-crossing connections with path-prefixed model naming so hierarchical SSPs simulate end-to-end
- **IMP-044** [Refactor test_sim_graph_builder.cpp](IMP-044.md) — Extract helpers, fix test gaps, and clarify scope in the ConnectionInfo/ConnectorInfo test file
<!-- pb:index:end -->
