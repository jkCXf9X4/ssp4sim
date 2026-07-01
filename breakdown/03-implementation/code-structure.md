# Code Structure
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-STRUCTURE-001 -->

## Description

This artifact describes the high-level code organization of SSP4SIM, including the repository tree annotated by layer ownership and the module dependency graph.


## Module Dependency Graph

```
                    ┌──────────────┐
                    │ handler/     │
                    │ (FMU load)   │
                    └──────┬───────┘
                           │
              ┌────────────▼──────────┐
              │(SSP → analysis graph) │
              └────────────┬──────────┘
                           │
         ┌────────────▼──────────────────────┐
         │ analysis graph -> execution graph │
         └────────────┬──────────────────────┘
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