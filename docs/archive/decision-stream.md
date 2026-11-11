<!-- GENERATED FILE — do not edit. Regenerate with `pb registers`. -->

# Decision Stream

Flat, dated history of every committed design choice. Current state lives
in the layer leaves named by each record's `state:` field.

Records live in the cold-storage archive; extract one with `pb archive read <ID>`.

## By Layer

### Intent

- **D-INT-001** — Linux-First Platform Focus · accepted · 2026-11-11 · `D-INT-001`
  Linux (Ubuntu 22.04 x86_64) is the sole supported platform; portability is out of scope.

### Product

- **PD-001** — Local Database as Primary Result Artifact · accepted · 2026-11-11 · `PD-001`
  Local database output is the primary result-artifact category; CSV is a portable export, remote DB is deferred.
- **PD-002** — Hot-Path Logging Compile-Time Disable · accepted · 2026-11-11 · `PD-002`
  Hot-path logging is compiled out at build time via the SSP4SIM_LOG_HOT_PATH CMake option.
- **PD-003** — DuckDB as Local Database Backend · superseded · 2026-11-11 · `PD-003`
  DuckDB was an optional local database recording backend, since removed in favor of SQLite WAL.
- **PD-004** — Terminal and File as the Overview Logging Tier · accepted · 2026-11-11 · `PD-004`
  Terminal output and a plain text file form the overview logging tier for local runs.
- **PD-005** — Full JSON File as the Deep-Analysis Logging Tier · accepted · 2026-11-11 · `PD-005`
  A full JSON file is the canonical local structured log artifact for offline analysis and automation.
- **PD-006** — Cutelog as the Live Structured Log Navigation Tier · accepted · 2026-11-11 · `PD-006`
  Cutelog is the live structured log navigation tier over the same JSON event stream used for deep analysis.

### Architecture

- **AD-001** — Modular Monolith with Static Library Boundary · accepted · 2026-11-11 · `AD-001`
  Engine modules build into a single static library with an explicit public header API boundary.
- **AD-002** — fmi4c for FMI Adapter Layer · accepted · 2026-11-11 · `AD-002`
  FMU .so files are loaded via the vendored fmi4c C adapter, which wraps FMI 2.0 function resolution and lifecycle.
- **AD-003** — Graph Responsibility Separation · superseded · 2026-11-11 · `AD-003`
  Separated the analysis graph (SSP mirror, no model-to-model edges) from the simulation graph (flattened runtime graph).
- **AD-004** — AnalysisSystem — Responsibilities and Data Model Separation · superseded · 2026-11-11 · `AD-004`
  Introduced a standalone AnalysisSystem data model with vector storage and string-based references, decoupled from graph traversal.
- **AD-005** — Three-Layer Architecture — Analysis System, Analysis Graph, and Simulation Graph · superseded · 2026-11-11 · `AD-005`
  Formalized three layers: AnalysisSystem (pure data), transient AnalysisGraphFactory (graph views), and the simulation graph.
- **AD-006** — Pre-Simulation Pipeline — Parsing, Analysis Projection, and Simulation Graph · accepted · 2026-11-11 · `AD-006`
  A staged pre-simulation pipeline hands a resolved connection graph to simulation-graph construction; algebraic-loop detection runs at execution time.

### Implementation

- **DR-005** — Feedthrough Detection via Analysis Graph Traversal · accepted · 2026-11-11 · `DR-005`
  Feedthrough is detected by BFS over the FMU internal dependency graph in the analysis layer, not via FMI API calls in the sim graph.
- **IMD-001** — Use C++23 Standard · accepted · 2026-11-11 · `IMD-001`
  SSP4SIM is written in C++23, requiring GCC 13+ or Clang 16+.
- **IMD-002** — Static Library with Minimal Public Headers · accepted · 2026-11-11 · `IMD-002`
  All engine modules compile into a single static library with public headers separated in lib/public_include/.
- **IMD-003** — Coding Conventions · accepted · 2026-11-11 · `IMD-003`
  The codebase follows the coding conventions documented in docs/development.md, with no enforced formatter.
- **IMD-004** — Introduce SQLite WAL Recorder Sink · accepted · 2026-11-11 · `IMD-004`
  A SqliteWALRecorderSink writes simulation data to a SQLite database file with WAL journal mode.
- **TD-001** — Use Catch2 for C++ Tests · accepted · 2026-11-11 · `TD-001`
  C++ unit and integration tests use the Catch2 framework with test_*.cpp naming.
- **TD-002** — Use pytest for Python Workflow Tests · accepted · 2026-11-11 · `TD-002`
  Python workflow tests use pytest, with reference SSP fixtures discovered through parameterization.

### Verification

- **VD-001** — C++ for Unit Tests, Python for Workflow Tests · accepted · 2026-11-11 · `VD-001`
  C++ Catch2 covers unit/integration tests; Python pytest covers workflow-level tests and result validation.
- **VD-002** — Known Xfails Tracked as Pytest Markers · accepted · 2026-11-11 · `VD-002`
  Known failures are tracked as strict pytest xfail markers with rationale; XPASS detection flags newly passing tests.
- **VD-003** — CSV Recorder Coverage Gap · accepted · 2026-11-11 · `VD-003`
  The CSV recorder has no dedicated unit test; coverage through the smoke and workflow tests is accepted for the experimental phase.
- **VD-004** — Reference SSP Fixtures Are Static and Version-Controlled · accepted · 2026-11-11 · `VD-004`
  Reference SSP fixtures under resources/reference_ssp/ are static, version-controlled artifacts following the fixture management policy.

### Operation

- **OD-001** — Local Database as Default Recording Format · accepted · 2026-11-11 · `OD-001`
  The local database is the default recording format; CSV is an optional portable export path.
- **OD-002** — File-Based Output Only For Current Releases · accepted · 2026-11-11 · `OD-002`
  Current releases write file-based output only; no streaming, socket, or network output; remote database ingestion is a future category.
- **OD-003** — Logging Levels Configurable at Runtime · accepted · 2026-11-11 · `OD-003`
  Logging levels are configurable per-sink at runtime via the JSON configuration file; hot-path logging can additionally be compiled out.
- **OD-004** — InfluxDB Line Protocol as Remote Result Interface · accepted · 2026-11-11 · `OD-004`
  InfluxDB Line Protocol is the canonical wire format for remote result ingestion, independent of the InfluxDB product.
- **OD-005** — OpenTelemetry as the Distributed Observability Tier · accepted · 2026-11-11 · `OD-005`
  OpenTelemetry is the distributed observability contract for correlating spans and trace context across simulation components.
