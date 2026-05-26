# Module: handler
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-HANDLER-001 -->

## Purpose

Loads FMU archives, prepares FMI 2.0 instances for execution, and manages the FMU lifecycle (instantiate → init → step → terminate → free).

## Key Components

- `FmuHandler`: Manages the map of FMU instances. Orchestrates loading and initialization.
- `FmuInfo`: Stores metadata and prepared runtime objects for a single FMU. Holds model description, FMI instance, and co-simulation model handle.
- `Fmi4cAdapter`: Wraps fmi4c library calls for FMU loading, function pointer resolution, and lifecycle management.
- `FmuInstance`: Represents a single instantiated FMU with its resource handle.

## Include Boundary

- Path: `lib/include/handler/`
- 4 files: fmi4c_adapter.hpp, fmi4c_adapter.cpp, fmu_handler.hpp, fmu_handler.cpp

## Dependencies

- `model/` — for CoSimulationModel type
- `utils/` — for config access
- External: fmi4c (vendored in 3rdParty/)

## Notable Patterns

- After fmi2Error or fmi2Fatal, cleanup frees the instance without calling fmi2Terminate (so logs keep the root cause).
- FMU libraries loaded via dlopen — no sandboxing for vendor code.
- Mode bit changes on tracked fixtures require copy-to-tmp workaround.

## Traceability

- Backward: Architecture component view, decision AD-002 (fmi4c adapter).
- Sources: `lib/include/handler/`, `tests/lib/core/test_fmi4c_adapter.cpp`.