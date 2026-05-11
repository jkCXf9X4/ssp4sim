# Overview

SSP4SIM is a small experimental SSP simulation engine. Compared with broader
simulation environments, it focuses on a direct path from SSP input to an
executable run and keeps the runtime surface area intentionally narrow.

Notable characteristics:

- Native support for SSP 1.0 archives and unpacked SSP directories
- FMI 2.0 co-simulation models
- Explicit execution strategies, including Gauss-Jacobi and Gauss-Seidel
- Parallel and sequential executor variants, plus custom delay-oriented
  executors in the engine internals
- A recording pipeline that copies completed signal updates into recorder-owned
  buffers before writing CSV output or direct DuckDB tables
- Configurable logging with terminal, file, JSON, and cutelog sinks
- A C++23 library, CLI, C API, and Python API for embedding or scripting

These traits make the project useful when the goal is to test SSP-based
simulation strategies, reproduce a model run with a compact codebase, or slot
the engine into a C++ or Python workflow without a large external runtime.

For installation, build, and usage details, see the linked documentation pages
from the project README.
