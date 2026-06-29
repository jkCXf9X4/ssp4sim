# Code Structure
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-STRUCTURE-001 -->

## Description

This artifact describes the high-level code organization of SSP4SIM, including the repository tree annotated by layer ownership and the module dependency graph.

## Repository Tree

```
ssp4sim/
├── CMakeLists.txt              # Root build — project definition, presets
├── CMakePresets.json            # Build presets (vcpkg, debug, release)
├── lib/                         # Core simulation engine (static library)
│   ├── CMakeLists.txt           # Module targets, include dirs, dependencies
│   ├── include/                 # Private headers per module
│   │   ├── execution/           # Executor strategies (Jacobi, Seidel, custom)
│   │   ├── graph/               # Execution graph, analysis graph
│   │   │   └── analysis/        # Model/connection analysis from SSP
│   │   ├── handler/             # FMU loading, lifecycle, fmi4c adapter
│   │   ├── model/               # FMU model wrapper, connections, connectors
│   │   ├── signal/              # Signal storage, recorder, sinks
│   │   ├── utils/               # Config, allocator, ring buffer, thread pool
│   │   └── schema_extensions/   # FMI2/SSP schema extension types
│   └── public_include/          # Public API headers (simulator.hpp, C API)
├── public/                      # Entry points
│   ├── ssp4sim_app/             # CLI binary (sim_app)
│   └── python_api/              # Python bindings (pyssp4sim)
├── tests/                       # Test suite
│   ├── lib/                     # C++ Catch2 tests
│   │   ├── core/                # Core runtime primitives
│   │   ├── utils/               # Utility tests
│   │   └── high_level/          # One smoke test
│   ├── python/                  # Python pytest tests
│   │   └── high_level/          # Reference SSP sweep
│   └── resources/               # Test fixtures, reference SSPs
├── resources/                   # Example configs, SSP scenarios
├── scripts/                     # Helper scripts (release, analysis)
├── 3rdParty/                    # Vendored dependencies
│   ├── fmi4c/                   # FMI adapter library
│   └── ssp4cpp/                 # SSP parsing library
├── containers/                  # Docker container definitions
└── docs/                        # Project documentation
```

## Module Dependency Graph

```
                    ┌──────────────┐
                    │ handler/     │
                    │ (FMU load)   │
                    └──────┬───────┘
                           │
              ┌────────────▼──────────┐
              │     analysis/         │
              │ (SSP → model graph)   │
              └────────────┬──────────┘
                           │
              ┌────────────▼──────────┐
              │      graph/           │
              │ (execution graph)     │
              └────────────┬──────────┘
                           │
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
  ┌──────────┐      ┌──────────┐      ┌──────────────┐
  │model/    │      │execution/│      │   signal/    │
  │(FMU wrap)│      │(strategy)│      │(recording)   │
  └────┬─────┘      └────┬─────┘      └──────┬───────┘
       │                 │                    │
       └─────────────────┼────────────────────┘
                         ▼
                   ┌──────────┐
                   │ utils/   │
                   │(shared)  │
                   └──────────┘
```

## Traceability

- Backward: Traces to component view in `product-breakdown/02-architecture/component-view.md`.
- Sources: `docs/development.md`, `lib/CMakeLists.txt`, `lib/include/` directory listing.