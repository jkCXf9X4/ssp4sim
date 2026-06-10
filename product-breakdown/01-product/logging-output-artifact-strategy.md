# Logging Output Artifact Strategy
<!-- Layer: 01-product -->
<!-- Stable ID: PRO-ARTIFACT-002 -->

## Description

This artifact describes the logging output strategy — the tiers of log output artifacts SSP4SIM produces for operator insight and offline analysis.

## Logging Tiers

### 1. Terminal / File Overview

- **Format**: Plain-text (configurable level)
- **Purpose**: Real-time operator awareness during simulation
- **Configuration**: `logging.console.*`
- **Use Case Traceability**: Serves operator awareness during simulation (general use), supplementary to PRO-UC-008 and PRO-UC-010. Not the primary tool for either.

### 2. Full JSON Archival Log (Canonical Structured Artifact)

- **Backend**: Full JSON file (PD-005)
- **Purpose**: Machine-readable offline inspection, filtering, correlation, and automation
- **Tooling**: `jq`, Python scripts, any JSON-consuming pipeline
- **Configuration**: `logging.json.*`
- **Use Case Traceability**: Serves **PRO-UC-008** (Analyze Full JSON Logs After a Run) — provides the structured JSON output that developers filter with `jq`/scripts. Also serves as the baseline structured log source for future PRO-UC-010 correlation.

### 3. Cutelog Live Navigation

- **Backend**: Python socket handler for the cutelog viewer
- **Purpose**: Interactive runtime log inspection with filtering and search
- **Status**: Optional — requires cutelog running on the system
- **Use Case Traceability**: Serves interactive debugging needs. Supplementary to PRO-UC-008 and PRO-UC-010; provides real-time inspection capability.

### 4. OpenTelemetry Distributed Trace Export (Future / Proposed)

- **Status**: Proposed — not yet implemented (IMP-031)
- **Backend**: OpenTelemetry exporter (configurable protocol/endpoint)
- **Purpose**: Distributed trace export for correlating observability across multiple simulation instances in multi-process or multi-service simulations
- **Configuration**: OpenTelemetry exporter settings (independent of logging.* sinks)
- **Use Case Traceability**: Serves **PRO-UC-010** (Correlate Distributed Simulation Behavior with OpenTelemetry) — provides the trace export that enables cross-process timing and causality analysis. Relies on **OD-005** (OpenTelemetry as the Distributed Observability Tier) for the architectural contract.
- **Further Detail**: See `../06-evolution/backlog/candidates/IMP-031.md` for the full candidate specification, including span emission, trace context propagation, metadata design, and acceptance criteria.

## Design Rationale

### Terminal/File Overview

A plain-text tier exists for real-time operator awareness during simulation — it has the lowest latency and widest compatibility (no external viewer needed). The tradeoff is that plain text is human-readable but not machine-parsable, with limited filtering.

### Full JSON Archival (Canonical Structured Artifact)

Structured JSON is the canonical archival format rather than plain text because machine-readable offline inspection enables filtering, correlation, automation, and integration with standard tooling like `jq`. The cost is larger volume and structured overhead.

### Cutelog Live Navigation

An interactive tier exists as optional for runtime log inspection with filtering and search — essential for debugging complex simulations without post-hoc analysis. It is optional because it requires an external viewer running on the system, and not every workflow needs it.

### OpenTelemetry Distributed Trace Export (Future / Proposed)

A distributed trace export tier exists for correlating observability across process boundaries — something local terminal, JSON, and cutelog sinks cannot achieve. OpenTelemetry is chosen (OD-005) because it provides a standard, backend-agnostic protocol for span emission, context propagation, and cross-service trace correlation. The tradeoff is additional protocol and backend configuration complexity compared with local sinks, and the requirement for trace collection infrastructure. This tier is complementary to the local tiers, not a replacement.

### Why This Set of Tiers

Each tier targets a different consumption mode: real-time awareness (terminal), offline machine analysis (JSON), interactive debugging (Cutelog), and distributed cross-process observability (OpenTelemetry). A single log format cannot serve all three without either over-engineering the simple case or under-serving the complex case.
The OpenTelemetry tier addresses a consumption mode — distributed observability across simulation instances — that no local sink can serve, which is why it exists as a separate export rather than an extension of the JSON or terminal tiers.

## Hot-Path Compile-Time Disable

Hot-path logging can be compiled out at build time via the `SSP4SIM_LOG_HOT_PATH` CMake option (PD-002). When set to `OFF`, hot-path log statements are removed entirely with zero runtime overhead. See `docs/logging_guidlines.md` (Hot Paths section) and `docs/development.md` for build flag details.

## Traceability

- Backward: PD-002, PD-005, CAP-007, CAP-008, PRO-UC-008, PRO-UC-010, OD-005, IMP-031
- Sources: `docs/logging_guidlines.md`, `docs/configuration.md` (logging section), `../06-evolution/backlog/candidates/IMP-031.md`
- Per-category traceability: Documented inline in the Logging Tiers section above
