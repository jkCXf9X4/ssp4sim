# Pre-step Pipeline

All types live under `ssp4sim::analysis` in the `pre/` directory tree. The `pre/` folder owns its internal types. External consumers access analysis artifacts via `pre/...` include paths.

Markdown generated with outline / structure by an LLM. This plan specifies everything needed to close the gap between "broken" and "compiling + passing". Each builder is its own concern.

## Pipeline Overview

```
SSP Package (ssp4cpp::Ssp)
         │
         ▼
┌─────────────────────────────────────────────┐
│  1. SspSystemBuilder     (1_ssp_parser/)    │
│     Parse XML schemas                        │
│     Build canonical object model (SspSystem) │
│     Expand nested systems                    │
│     Output: SspSystem                        │
└─────────────────────┬───────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────┐
│  2. SspTreeBuilder      (2_analysis/)       │
│     Build SspNode<T> tree from SspSystem    │
│     Apply parameter sets (flatten + match)  │
│     Output: SspSystemNode (tree)            │
└─────────────────────┬───────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────┐
│  3. SspGraphBuilder    (2_analysis/)        │
│     Build model-to-model graph from tree    │
│     Resolve connections across nesting      │
│     Detect algebraic loops (Tarjan SCC)     │
│     Topological sort acyclic portions       │
│     Detect direct feedthrough               │
│     Output: SspSystemNode (graph w/ edges)  │
│             + AnalysisGraphData             │
└─────────────────────┬───────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────┐
│  4. GraphBuilder       (3_simulation/)      │
│     Consume AnalysisGraphData               │
│     Create FmuModel instances               │
│     Allocate SignalStorage                  │
│     Wire connections                        │
│     Derive execution edges                  │
│     Output: graph::Graph (execution graph)  │
└─────────────────────┬───────────────────────┘
                      │
                      ▼
              Run simulation
```

---

## 1. SSP Parser — Canonical Intermediate Representation

**File:** `1_ssp_parser/ssp_parser.hpp` → `SspSystemBuilder`
**Output:** `SspSystem` (canonical IR)

Never execute directly from XML. `SspSystemBuilder::build(ssp4cpp::Ssp*)` converts everything into the internal object model in one pass. Everything downstream works only on this IR.

### Element Hierarchy

```
SspSystem (SspItem)
 ├── models: vector<SspModel>
 ├── connectors: vector<SspConnector>
 ├── connections: vector<SspConnection>
 ├── nested_systems: vector<SspSystem>
 ├── resolved_connections: vector<SspConnection>
 └── parameter_bindings: map<string, ParameterValue>

SspModel (SspItem)
 ├── connectors: vector<SspConnector>
 ├── model_variables: vector<SspModelVariable>
 ├── parameter_bindings: map<string, ParameterValue>
 ├── fmu: shared_ptr<Fmu>
 ├── delay: uint64_t
 ├── canInterpolateInputs: bool
 └── maxOutputDerivativeOrder: int

SspConnector (SspItem)
 ├── causality: Causality
 ├── value_reference: uint64_t
 ├── data_type: DataType
 ├── initial_value: ParameterValue
 └── dependencies: vector<uint64_t>  // value_references used for matching

SspConnection (SspItem)
 ├── source_model, source_connector: string
 ├── target_model, target_connector: string
 ├── delay: uint64_t
 └── is_boundary: bool

SspModelVariable (SspItem)
 ├── value_reference: unsigned int
 ├── causality: fmi2::Causality
 ├── variability: fmi2::Variability
 └── dependencies: vector<uint64_t>  // value_references used for matching
```

### Known issues to fix in this layer

- `ssp_model.hpp` line 38: `td::map` → `std::map`
- `ssp_system.cpp` line 95: `Connection(` → `SspConnection(`
- `FMI2_modelDescription_Ext.hpp` lines 47, 55: missing semicolons after struct definitions
- `SspModel::create_model_variables()` is declared but never implemented — implement it
- `ssp_parser.cpp` includes `"analysis/analysis_system_builder.hpp"` which doesn't exist — fix to self-reference
- Missing files: `ssp_component.hpp`, `ssp_parameter_bindings.hpp` must be created

---

## 2. Tree Builder — SspNode<T> Tree + Parameter Application

**File:** `2_analysis/tree_builder.hpp` → `SspTreeBuilder`
**Input:** `SspSystem` (from Layer 1)
**Output:** `SspSystemNode` (tree of `SspNode<T>` wrappers)

### 2.1 Build the tree

`SspNode<T>` wraps a pointer to an `SspItem`-derived object:

```cpp
template <std::derived_from<SspItem> T>
struct SspNode : public utils::graph::Node {
    T *source;
};
```

Convenience aliases:

| Alias | Underlying type |
|---|---|
| `SspSystemNode` | `SspNode<SspSystem>` |
| `SspModelNode` | `SspNode<SspModel>` |
| `SspConnectorNode` | `SspNode<SspConnector>` |
| `SspConnectionNode` | `SspNode<SspConnection>` |
| `SspVariableNode` | `SspNode<SspModelVariable>` |

Tree structure mirrors SSP nesting:

```
sys_1 (SspSystemNode)
 ├── sys_2 (SspSystemNode)
 │    ├── component_1 (SspModelNode)
 │    │    ├── connector_in_1 (SspConnectorNode)
 │    │    ├── connector_out_1 (SspConnectorNode)
 │    │    └── internal_variable_1 (SspVariableNode)
 │    ├── component_2 (SspModelNode)
 │    │    ├── connector_in_1 (SspConnectorNode)
 │    │    ├── connector_out_1 (SspConnectorNode)
 │    │    ├── internal_variable_1 (SspVariableNode)
 │    │    └── parameter_set_1 (via source->parameter_bindings)
 │    ├── connector_out_1 (SspConnectorNode)  ← system boundary
 │    ├── connection ...
 │    ├── connection ...
 │    ├── parameter_set_2
 │    └── parameter_set_3
 ├── component_1 (SspModelNode)
 │    ├── connector_in_1, connector_out_1
 ├── connection ...
 └── parameter_set_4
```

### 2.2 Apply parameter sets

Parameter sets can appear at component or system level. A parameter set only influences artifacts below and on the same level.

**Rules:**
1. Lower levels are applied first (DFS post-order: children before parent)
2. Same-level sets applied in order of appearance (1, 2, 3, 4 in example above)
3. Dotted names signify nesting: `"sys_2.component_1.connector_in_1" = 5`
4. Component-level params use short names: `"connector_in_1" = 5`
5. System-level params use prefix: `"sys_2.component_1.connector_in_1" = 5`

**Algorithm:**

```
flatten(node, prefix=""):
    flat_map = {}
    for child in node.children:
        child_prefix = prefix.empty() ? child.name : prefix + "." + child.name
        flat_map.merge(flatten(child, child_prefix))
    flat_map[prefix + "." + node.name] = node
    return flat_map

apply_parameters(node):
    # Post-order: children first
    for child in node.children:
        apply_parameters(child)

    # Flatten everything below this node
    flat = flatten(node)

    # Match parameter keys (dotted paths) to flat entries
    for each (key, value) in node.source->parameter_bindings:
        if flat contains a key matching key:
            apply value to flat[key]->source
```

**Note:** `SspNode<T>::flatten()` already exists. The `apply_parameters` logic is stubbed (`LOG_WARNING("Parametersets not applied!")`) — implement it by uncommenting and adapting the commented-out code in `tree_builder.cpp`.

---

## 3. Graph Builder — Model-to-Model Graph + Analysis

**File:** `2_analysis/graph_builder.hpp` → `SspGraphBuilder`
**Input:** `SspSystemNode` (tree from Layer 2)
**Output:** `AnalysisGraphData` (model→connector→connection→connector→model graph)

**IMPORTANT:** The current `graph_builder.cpp` is a broken copy-paste of `tree_builder.cpp`. It must be rewritten from scratch.

### 3.1 Build the resolved connection graph

Walk the tree bottom-up and collect all `SspModelNode` instances. For each
`SspConnectionNode`, resolve the source/target model and connector names against
the tree hierarchy, then build a navigable graph chain:

```
model_1 -> connector_out -> connection -> connector_in -> model_2
```

Each step is established via `add_child()` (non-owning raw pointer). The graph
chain enables the sim graph builder to navigate structurally with zero string
lookups:

```
for model in model_nodes:
    create FmuModel
    for connector in model.children<SspConnectorNode>:
        allocate SignalStorage  // inputs & outputs
        for connection in connector.children<SspNode<ResolvedConnection>>:
            peer_connector = connection.children<SspConnectorNode>[0]
            peer_model = peer_connector.parents<SspModelNode>[0]
            wire(connector, peer_connector, connection.source.delay)
```

Construction pseudocode:

```
model_1.add_child(connector_out.get())
connector_out.add_child(connection_1.get())
connection_1.add_child(connector_in.get())
connector_in.add_child(model_2.get())
```

`connection_1->source->delay` stores the accumulated delay over the resolved
connection path. Multiple connections may be created if the SSP connection
crosses system boundaries (each boundary crossing adds an intermediate
connection node).

**Connection resolution rules:**
- First try: exact match in the same system scope
- Second: search upward through parent systems
- Third: use fully qualified dot-separated path from root


### 3.2 Output: AnalysisGraphData

All nodes are owned by typed vectors at the `AnalysisGraphData` level. Graph
edges between nodes are non-owning raw pointers (established via `add_child`),
so there is no double-ownership issue.

`ResolvedConnection` extends `SspItem` so it can be wrapped by the existing
`SspNode<T>` template without modifying the template constraint.

```cpp
struct ResolvedConnection : public SspItem {
    uint64_t delay = 0;
    ResolvedConnection() { type = SspItemType::Connection; }
};

struct AnalysisGraphData {
    std::vector<std::unique_ptr<SspModelNode>>                    model_nodes;
    std::vector<std::unique_ptr<SspConnectorNode>>                connector_nodes;
    std::vector<std::unique_ptr<SspNode<ResolvedConnection>>>    connection_nodes;
};
```


### 3.3 Detect algebraic loops (Tarjan SCC)

Use `utils::tarjan` to find strongly connected components in the model-to-model graph.

- **SCC with >1 node** → algebraic loop (multi-component cycle)
- **SCC with 1 node + self-loop** → algebraic loop (feedthrough in that model)
- **Handling:** flag algebraic loop but continue, to be handled by the executor 
  The `SspSystem` header already includes `utils/tarjan.hpp` — the integration point is ready.

### 3.4 Topological sort acyclic portions

Collapse each SCC into a meta-node. Topologically sort the resulting DAG. This produces the legal execution order for the non-loop portions of the model graph.

### 3.5 Detect direct feedthrough / internal algebraic loops

**Deferred (Phase 3+).** For the initial compile target, feedthrough detection is
skipped and the simulation layer treats all connectors as feedthrough-capable
(correct but suboptimal scheduling).

When implemented:

A model has direct feedthrough if any output depends on an input at the same
time step. Detect by:

1. Using `SspModelVariable` dependencies (parsed from FMU ModelStructure)
2. Tracing paths from input connectors to output connectors through internal
   variables
3. If a path exists without passing through a state variable → feedthrough

Feedthrough flags live on `SspConnection` and `SspConnector`. They are resolved
by the postponed feedthrough detection pass — the graph builder creates the
connector→connection→connector graph with model variables so that algebraic
loops within a system can be evaluated.

Per-connector feedthrough flags propagate into `AnalysisGraphData.connector_nodes`
for the simulation layer to use when wiring `ConnectionInfo`.

---

## 4. Simulation Graph Builder

**File:** `3_simulation/sim_graph_builder.hpp` → `graph::GraphBuilder`
**Input:** `AnalysisGraphData` (from Layer 3)
**Output:** `graph::Graph` (execution graph)

The `GraphBuilder`:
1. Iterates `graph_data.model_nodes` to create `FmuModel` instances
2. Uses `model->get_children_of_type<SspConnectorNode>()` to discover all
   input/output connectors and allocate `SignalStorage`
3. Navigates the connector→connection→connector chain to wire connections:
   - `connector->get_children_of_type<SspNode<ResolvedConnection>>()` finds connections
   - `connection->get_children_of_type<SspConnectorNode>()` finds the peer connector
   - `peer_connector->parents` finds the owning model
   - `connection->source->delay` provides the accumulated delay
4. Derives execution edges from model-to-model dependencies in the graph
5. Produces a `graph::Graph` ready for `Graph::invoke(StepData)`

**Note:** The `GraphBuilder` currently references `analysis::AnalysisGraphData` via `#include "analysis/analysis_graph_factory.hpp"` — this include path must be updated to use the local `pre/` boundary type definition.

---

## Include Path Policy

Everything inside `pre/` uses relative-to-pre include paths:

| Current (broken) | Target |
|---|---|
| `"analysis/analysis_system_builder.hpp"` | `"pre/1_ssp_parser/ssp_parser.hpp"` |
| `"analysis/components/analysis_system.hpp"` | `"pre/1_ssp_parser/elements/ssp_system.hpp"` |
| `"analysis/components/analysis_model.hpp"` | `"pre/1_ssp_parser/elements/ssp_model.hpp"` |
| `"analysis/components/analysis_connector.hpp"` | `"pre/1_ssp_parser/elements/ssp_connector.hpp"` |
| `"analysis/components/analysis_connection.hpp"` | `"pre/1_ssp_parser/elements/ssp_connection.hpp"` |
| `"analysis/components/analysis_model_variable.hpp"` | `"pre/1_ssp_parser/elements/ssp_model_variable.hpp"` |
| `"analysis/analysis_graph_factory.hpp"` | `"pre/2_analysis/graph_builder.hpp"` (or `"pre/ssp_graph_data.hpp"`) |

External consumers (`simulation.cpp`, tests) include via `"pre/..."`.

The `CMakeLists.txt` in `lib/` adds `lib/include` as a private include directory — this is sufficient since all `pre/` files live under `lib/include/pre/`.

---

## Implementation Order

### Phase 1: Fix includes and create missing files
1. Create `ssp_graph_data.hpp` with `ResolvedConnection` and `AnalysisGraphData`
2. Create `ssp_component.hpp` — base component type
3. Create `ssp_parameter_bindings.hpp` — parameter binding helpers
4. Fix all `#include` paths in `pre/` to use `pre/...` relative paths
5. Fix CMakeLists.txt — remove non-existent include dirs

### Phase 2: Fix broken code
6. Fix `ssp_model.hpp` line 38: `td::map` → `std::map`
7. Fix `ssp_system.cpp` line 95: `Connection(` → `SspConnection(`
8. Fix `FMI2_modelDescription_Ext.hpp` missing semicolons at lines 47, 55
9. Implement `SspModel::create_model_variables()`
10. Rewrite `graph_builder.cpp` from scratch (currently broken copy-paste)

### Phase 3: Implement missing features
11. Implement parameter set application in `tree_builder.cpp`
12. Implement `SspGraphBuilder::build()` — model→connector→connection→connector→model graph + `AnalysisGraphData` output
13. Implement cross-nesting-level connection resolution (scope-based name lookup)
14. Implement algebraic loop detection via Tarjan SCC
15. Implement topological sort of acyclic SCC graph
16. Implement feedthrough detection per connector

### Phase 4: Wire the pipeline
17. Update `simulation.cpp` to use `SspSystemBuilder` → `SspTreeBuilder` → `SspGraphBuilder` → `GraphBuilder`
18. Fix `GraphBuilder` to accept `AnalysisGraphData` (from `ssp_graph_data.hpp`) instead of deleted Gen2 types
19. Update tests to use Gen1 types and `pre/...` include paths

---

## Future Considerations

- Algebraic loop handling strategy (fixed-point iteration vs. error vs. unit delay)
- Parameter sets with expressions (not just literal values)
- Direct feedthrough breaking via automatic unit-delay insertion
- Optimization: merge non-feedthrough models into a single execution step