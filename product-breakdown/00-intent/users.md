# Users
<!-- Layer: 00-intent -->
<!-- Stable ID: INT-USERS-001 -->

## Description

This artifact enumerates the target user groups and their primary needs. It defines who the project is built for and what problems it solves for each group.

## Content

- **Developers testing SSP-based simulation strategies**: Need a compact, controllable simulation engine to experiment with execution strategies (Gauss-Jacobi, Gauss-Seidel) and validate SSP workflow assumptions without the overhead of a full simulation environment.
- **Researchers reproducing model runs with a compact codebase**: Need a minimal, self-contained codebase that can be built from source and run deterministically across environments, enabling reproducible research with SSP 1.0 archives and FMI 2.0 co-simulation FMUs.
- **Engineers slotting the engine into C++ or Python workflows without a large external runtime**: Need an embeddable engine with a C++23 library API, C API, and Python bindings that fits into existing toolchains without pulling in a heavyweight simulation runtime.

## Traceability

- Backward: Defined by the product-breakdown 00-intent layer framework.
- Sources: `readme.md` (lines 5-7, 54-60), `docs/overview.md` (lines 21-23).