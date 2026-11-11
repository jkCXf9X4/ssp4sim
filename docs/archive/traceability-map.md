<!-- GENERATED FILE — do not edit. Regenerate with `pb registers`. -->

# Traceability Map

Generated from record front-matter: each decision names the leaf that
holds its outcome (`state:`). The breakdown never references a record —
the leaf is the source of truth; the record is how the state got there.

### Intent

- **D-INT-001** — State: `00-intent/constraints.md` (Constraints) · Artifacts: `docs/development.md`, `docs/linux_binary_distribution.md`, `docs/build_from_source.md`

### Product

- **PD-001** — State: `01-product/result-artifact-strategy.md` (Result Artifact Strategy) · Artifacts: `docs/usage.md`, `docs/configuration.md`
- **PD-002** — State: `01-product/logging-output-artifact-strategy.md` (Logging Output Artifact Strategy) · Artifacts: `docs/logging_guidlines.md`, `docs/development.md`
- **PD-003** — State: `01-product/result-artifact-strategy.md` (Result Artifact Strategy) · Artifacts: `docs/usage.md`, `docs/configuration.md`
- **PD-004** — State: `01-product/logging-output-artifact-strategy.md` (Logging Output Artifact Strategy) · Artifacts: `docs/logging_guidlines.md`, `docs/configuration.md`
- **PD-005** — State: `01-product/logging-output-artifact-strategy.md` (Logging Output Artifact Strategy) · Artifacts: `docs/logging_guidlines.md`, `docs/configuration.md`
- **PD-006** — State: `01-product/logging-output-artifact-strategy.md` (Logging Output Artifact Strategy) · Artifacts: `docs/logging_guidlines.md`, `docs/configuration.md`

### Architecture

- **AD-001** — State: `02-architecture/component-view.md` (Component view) · Artifacts: `lib/CMakeLists.txt`, `lib/public_include`
- **AD-002** — State: `02-architecture/component-view.md` (Component view) · Artifacts: `3rdParty/fmi4c`, `lib/include/utils/fmi/fmi4c_adapter.hpp`, `tests/lib/core/test_fmi4c_adapter.cpp`
- **AD-003** — State: `02-architecture/component-view.md` (Component view) · Artifacts: `lib/include/pre`, `lib/include/simulation`
- **AD-004** — State: `02-architecture/component-view.md` (Component view) · Artifacts: `lib/include/pre`, `lib/include/simulation`
- **AD-005** — State: `02-architecture/component-view.md` (Component view) · Artifacts: `lib/include/pre`, `lib/include/simulation`
- **AD-006** — State: `02-architecture/component-view.md` (Component view) · Artifacts: `lib/include/pre`, `lib/include/simulation`, `tests/lib/analysis`

### Implementation

- **DR-005** — State: `02-architecture/component-view.md` (Component view) · Artifacts: `lib/include/pre`, `lib/include/simulation`
- **IMD-001** — State: `03-implementation/build-environment.md` (Build environment) · Artifacts: `CMakeLists.txt`, `docs/development.md`
- **IMD-002** — State: `03-implementation/code-structure.md` (Code structure) · Artifacts: `lib/CMakeLists.txt`, `lib/public_include`
- **IMD-003** — State: `03-implementation/code-structure.md` (Code structure) · Artifacts: `docs/development.md`
- **IMD-004** — State: `03-implementation/modules/signal.md` (signal module) · Artifacts: `vcpkg.json`, `lib/CMakeLists.txt`, `lib/public_include/shared_config.hpp`, `lib/include/simulation/signal/sinks/sqlite_recorder_sink.hpp`, `lib/include/simulation/signal/sinks/sqlite_recorder_sink.cpp`, `lib/include/simulation/signal/sinks/sqlite_recorder_utils.hpp`, `lib/include/simulation/signal/sinks/sqlite_recorder_storage.hpp`, `lib/include/simulation.cpp`, `tests/lib/core/test_sqlite_recorder.cpp`, `docs/configuration.md`
- **TD-001** — State: `04-verification/test-strategy.md` (Test Strategy) · Artifacts: `tests/lib`, `tests/lib/test_main.cpp`, `docs/build_from_source.md`
- **TD-002** — State: `04-verification/test-strategy.md` (Test Strategy) · Artifacts: `tests/python`, `tests/python/conftest.py`

### Verification

- **VD-001** — State: `04-verification/test-strategy.md` (Test Strategy) · Artifacts: `tests/README.md`, `docs/build_from_source.md`
- **VD-002** — State: `04-verification/regressions.md` (Regressions) · Artifacts: `tests/README.md`, `docs/breakdown/04-verification/regressions.md`
- **VD-003** — State: `04-verification/coverage-targets.md` (Coverage Targets) · Artifacts: `tests/README.md`, `docs/breakdown/04-verification/coverage-targets.md`
- **VD-004** — State: `04-verification/test-strategy.md` (Test Strategy) · Artifacts: `resources/reference_ssp/AGENTS.md`, `tests/README.md`

### Operation

- **OD-001** — State: `01-product/result-artifact-strategy.md` (Result Artifact Strategy) · Artifacts: `docs/usage.md`, `docs/configuration.md`
- **OD-002** — State: `01-product/result-artifact-strategy.md` (Result Artifact Strategy) · Artifacts: `docs/usage.md`, `docs/configuration.md`
- **OD-003** — State: `05-operation/monitoring.md` (Monitoring) · Artifacts: `docs/logging_guidlines.md`, `docs/configuration.md`
- **OD-004** — State: `01-product/result-artifact-strategy.md` (Result Artifact Strategy) · Artifacts: `docs/usage.md`, `docs/configuration.md`
- **OD-005** — State: `05-operation/monitoring.md` (Monitoring) · Artifacts: `docs/logging_guidlines.md`
