Input: SSP package
        │
        ▼
1. Parse XML schemas / Build canonical object model / Expand nested systems
        │
        ▼
Resolve parameter bindings / Validate connectors and interfaces
        │
        ▼
Build dependency graph
        │
        ▼
Detect algebraic loops (Tarjan SCC)
        │
        ▼
Topologically schedule acyclic portions
        │
        ▼
Generate execution graph
        │
        ▼
Initialize FMUs
        │
        ▼
Run simulation


each of the sections should be its own builder to separate concerns

---- ssp_parser ----

# 1. Canonical Intermediate Representation (IR)

Based on class SspItem

Never execute directly from XML. Instead, convert everything into an internal object model.

System
 ├── Components
 ├── Connectors
 ├── Parameters
 ├── Nested Systems
 ├── Signal Connections
 └── Metadata

Example

class System
{
    string name;

    vector<Component> components;

    vector<System> subSystems;

    vector<Connection> connections;

    ParameterSet parameters;
};

Likewise every FMU becomes

FMU
    Variables
    Inputs
    Outputs
    Parameters
    States
    Capabilities

Everything downstream works only on this IR.

---- analysis ----

# 2. build a tree

Use a Node that wrapps a pointer ta a SspItem as SspNode
this way we can create a tree/graph from the SspItem


sys_1
- sys_2
-- component_1
--- connector_in_1
--- connector_out_1
--- internal_variable_1
-- component_2
--- connector_in_1
--- connector_out_1
--- internal_variable_1
--- parameter_set_1
-- connector_out_1 <- system boundary connector
-- connection component_1.connector_out_1 to component_2.connector_in_1
-- connection component_2.connector_out_1 to sys_2.connector_out_1
-- parameter_set_2
-- parameter_set_3
- component_1
-- connector_in_1
-- connector_out_1
- connection sys_2.connector_out_1 to component_1.connector_in_1
- parameter_set_4


# 3. apply parameter sets 

Parametersets can be applied on a component or system level
- a parameter set can only influence artifacts below and on the same level

Lower levels are applied first, if on the same level they are applied in the order of appearance
in this case 1, 2, 3, 4

parametersets use '.' to signify nesting
ex: 
a sys_1 level system parameter set could have the assignment: "sys_2.component_1.connector_in_1" = 5
a sys_2 level system parameter set could have the assignment: "component_1.connector_in_1" = 5
a component parameterset would only have "connector_in_1" = 5

Method
walk the structure and apply parametersets

recursive(node):
  for child: children:
    recursive(child)
    if parameterset:
      flatten/map everything below and apply the parameter set



map/flatten the tree would produce:
"level1.level2.level3" : node
The parameters can then be matched to the correct node and applied

# 4. Create component graph

Create a analysis model to model graph using SspNode

use the tree to find the model to model connections and create a SspModelNode graph

Evaluate if the SspModelNode can be used as a mediator for the 

----

Not in scope as of yet

# 5. create a direct feedthru graph using the internal variables

Find direct feedthru items by linking all internal variables


---


---- simulation artifacts ----

# Build up simulation graph

the analysis graph builder is responsible to build the analysis model-model graph, utilize this for building the final simulation graph 