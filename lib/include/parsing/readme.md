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



# 1. Canonical Intermediate Representation (IR)

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