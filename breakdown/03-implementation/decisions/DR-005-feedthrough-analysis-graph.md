# DR-005: Feedthrough Detection via Analysis Graph Traversal

## Status

Accepted

## Context

IMP-023 requires per-variable feedthrough metadata on sim graph connections. The initial approach (IMP-023 v1) used the FMI `get_dependencies_variables()` API in `GraphBuilder::wire_connections()` to check if source outputs have `DependenciesKind::dependent` dependencies. This had two problems:

1. It overcounts feedthrough: the FMI API returns whether an output depends on ANY other variable (including internal state), not just inputs. An output that only depends on internal state (e.g., a discrete integrator) would be incorrectly marked feedthrough.

2. It duplicated work: the analysis graph already parsed FMI dependencies and wired them as Node edges in `wire_internal_dependencies()`. Computing feedthrough again in the sim graph layer was redundant.

## Decision

Move feedthrough detection to the analysis graph layer. Use BFS traversal of the existing FMU internal dependency graph (`wire_internal_dependencies()` edges) from each output connector through its children. If an input connector of the same FMU is reachable, mark the output as feedthrough.

## Consequences

- Positive: Correct feedthrough semantics — only outputs with transitive dependency on an input are marked feedthrough.
- Positive: No redundant FMI XML parsing — the analysis graph's dependency edges are reused.
- Positive: Cleaner separation — analysis layer owns the structural computation, sim layer reads the precomputed flag.
- Neutral: `AnalysisConnector` gains a `bool is_feedthrough` field.
- Negative: BFS traversal adds O(outputs × internal_edges) at build time, but this is negligible for realistic FMUs.

## Files Changed

- `lib/include/graph/analysis/components/analysis_connector.hpp` — added `is_feedthrough` field
- `lib/include/graph/analysis/analysis_graph_builder.hpp` — added `compute_feedthrough()` declaration
- `lib/include/graph/analysis/analysis_graph_builder.cpp` — added `compute_feedthrough()` implementation and call in `build()`
- `lib/include/graph/graph_builder.cpp` — removed FMI feedthrough precompute, now reads from `source_connector->is_feedthrough`