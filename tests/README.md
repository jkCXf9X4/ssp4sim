# Tests
<!-- Layer: 04-verification -->


This page describes test layout and test-specific behavior. Build setup lives in
[Build From Source](../docs/build_from_source.md).

## Quick Commands

Build and test commands are documented in [Build From Source](../docs/build_from_source.md).
This page focuses on test layout and behavior; use the commands there when you
need to run the C++ binary or the Python suite.

## Test Architecture

See breakdown/04-verification/test-strategy.md for the strategic rationale and test coverage targets.

The suite is split by layer so each test has a clear responsibility:

- `tests/lib/core/`: C++ tests for core runtime primitives such as FMU adapter,
  recorder, ring buffer, signal storage, start values, and SQLite recording.
- `tests/lib/utils/`: C++ tests for utility data structures and support code.
- `tests/lib/high_level/`: one C++ smoke test that executes a complete SSP
  through the public simulator entry point with CSV recording enabled.
- `tests/python/high_level/`: Python API tests for full SSP workflows and
  result-file validation.
- `resources/reference_ssp/`: unpacked reference SSP fixtures used by the high-level
  workflow tests.
- `tests/resources/references/` and `tests/resources/`: small static fixtures for focused
  C++ tests.

Keep the C++ high-level layer to a single smoke path through the top-level
modules. Put reference sweeps and result-comparison workflow tests in pytest so
the suite can use Python-side fixture discovery and comparison tools.

## C++ Library Tests

The C++ tests under `tests/lib/` are split into two build shapes:

- **Per-test unit binaries** (`test_<name>`): kernel, parser, and analysis/graph
  tests are compiled into their own executables, built only from their own
  transitive include cone under `lib/include/**` (implementation files sit
  adjacent to their headers, so the cone is resolved mechanically). A broken
  implementation file anywhere else in `lib/` no longer blocks these tests from
  building; one unit binary = one test = one clear result.
- **Retained integration binary** (`ssp4sim_tests`): tests that legitimately
  depend on the full pipeline (`tests/lib/high_level/`, sim/graph model
  construction, recorders, resolver) stay linked against the aggregate
  `ssp4sim_lib` and are listed explicitly in `tests/lib/CMakeLists.txt`
  (`SSP4SIM_INTEGRATION_TESTS`).

CTest is the canonical runner:

```bash
ctest --test-dir build --output-on-failure -j
ctest --test-dir build -R <test-case-name>   # targeted run
```

For direct per-binary builds and runs:

```bash
ninja -C build test_ring_buffer && ./build/tests/lib/test_ring_buffer
tests/run_unit.sh --tier T1                  # kernel/units
tests/run_unit.sh --filter test_ssp_node    # one binary
tests/run_unit.sh                           # every unit binary in the build tree
```

`tests/run_unit.sh` derives its default run set from the build tree, so a new
`test_<name>.cpp` is run as soon as it is built. The tier buckets (T1–T3) are a
curated Phase 0 grouping; a built binary that is not in any tier triggers a
warning rather than being silently dropped.

### Cone mechanics and configure-time semantics

Unit sources are computed at **configure** time as the transitive include
closure of each test file. Consequences:

- Adding a new `test_*.cpp` is picked up by `GLOB CONFIGURE_DEPENDS`; no
  cone list to maintain.
- Editing an `#include` inside an existing header does **not** retrigger a
  configure. After include edits to a cone source, re-run
  `cmake --preset=vcpkg` (or the build's incremental reconfigure) so cones
  reflect the new closure.
- A configure-time **cone audit** validates the machinery: it prints
  `[cone audit] OK: N unit cones, M lib sources ...` on success and a warning
  for any local include a cone cannot resolve (a missing directory in the
  include set, or a misspelled include). Watch configure output for it.

Prefer small, focused C++ cases named `test_*.cpp` under `tests/lib/core/`,
`tests/lib/utils/`, or `tests/lib/analysis/`. New files are picked up by the
build automatically (`GLOB CONFIGURE_DEPENDS`); they become standalone per-test
binaries unless listed in `SSP4SIM_INTEGRATION_TESTS`. Keep
`tests/lib/high_level/` to one top-level C++ smoke path and add the file to the
integration list. Put reference sweeps and detailed result comparisons in pytest
under `tests/python/`.

### Isolation tiers

The include cones of all unit tests were computed and bucketed (Phase 0).
Membership decides only the `tests/run_unit.sh` grouping; every tier builds the
same way.

| Tier | Directory | Test files |
|------|-----------|------------|
| T1 kernel/units | `tests/lib/core/`, `tests/lib/utils/` | `test_mpsc_event_queue`, `test_node`, `test_node_iterator`, `test_ring_buffer`, `test_config`, `test_thread_pool` |
| T2 parser | `tests/lib/core/` | `test_parameter_binding`, `test_schema_extensions`, `test_signal_storage`, `test_ssp_elements`, `test_start_value` |
| T3 analysis/graph | `tests/lib/analysis/`, `tests/lib/core/` | `test_graph_builder`, `test_ssp_node`, `test_tree_builder`, `test_parameter_binding_integration` |
| T4 integration | `tests/lib/high_level/`, `tests/lib/core/`, `tests/lib/graph/`, `tests/lib/model/`, `tests/lib/scheduling/`, `tests/lib/simulation/` | retained in `ssp4sim_tests` |

Note: `test_sim_graph_builder.cpp` exists in both `tests/lib/graph/` and
`tests/lib/simulation/`; both are integration tests (no target-name clash). The
Cone resolution is **soft**: an include that does not resolve within the include
set is treated as external/system. A wrong guess surfaces as a per-binary
link-time undefined reference, which is loud, not silent.

`tests/lib/graph/test_graph_analysis.cpp` covers the reusable
`ssp4sim::graph::GraphAnalysis` scheduler utility (SCC detection, component
topological sort, `SccGroup` condensed-graph construction and parent/child
placement verification) extracted from `LoopAwareExecutor`, which now builds on
it.

## High-Level SSP Tests

The high-level reference sweep lives in `tests/python/high_level/` so it can use
pytest parameterization and Python-side CSV/result comparison helpers. Pytest is
configured to collect only `tests/python`.

The test iterates the unpacked SSP fixtures under
`resources/reference_ssp/artifacts/models/*/*`, simulates each
co-simulation SSP, and checks that a complete result CSV is produced.
Model-exchange fixtures are excluded because the current runtime only supports
co-simulation FMUs.

The Python tests import `pyssp4sim` from `build/public/python_api` when that
build artifact exists. This prevents high-level tests from accidentally passing
against an unrelated installed wheel.

A separate smoke test exercises the packaged CLI through `venv/bin/pyssp4sim`
against the local `resources/embrace/embrace.json` fixture with temporary
output paths.

The `la2` executor (legacy alias `loop_aware`) is regression-tested against the algebraic-loop
reference fixtures in `test_loop_aware_nested.py`: `signal_nested_algebraic_loop`
(loop within loop) and `signal_algebraic_loop` (single loop). Each fixture is run
with both loop sub-step scheduling modes (`linear`, `factor`; legacy aliases
`fixed`, `geometric`) and the steady-state result is compared to the analytic
fixed point. Nested loop SCCs need more internal sub-iterations than the default
SCC node count to converge; the test pins that with
`simulation.executor.la2.iterations` (legacy `simulation.executor.loop_aware.iterations`
is still honored as a fallback).

See breakdown/03-implementation/dependency-policy.md for the fmi4c mode-bit issue and recommended fixture handling.

The reference sweep uses a `0.001` second simulation timestep. The `embrace`
SSP needs this smaller communication step; with a coarse `0.1` second step,
`ECS_HW` returns `fmi2Error` on the first step.

The recorder tests now cover the CSV and SQLite sinks separately.

The high-level parameter-set coverage is grouped in one parametrized test that
checks the emitted `start_values.csv` for inline system-level parameter sets,
the internal parameter-set fixture in `signal_sine_gain_add`, external `.ssv`
bindings, `.ssv + .ssm` mappings, and representative mapped fixtures with
multiple value types, including the hierarchical `dcmotor` fixture.

All known regression fixtures have been resolved. See breakdown/04-verification/regressions.md for history.

After an FMU reaches `fmi2Error` or `fmi2Fatal`, cleanup frees the instance
without calling `fmi2Terminate` so logs keep the original step failure as the
root cause.

## Reference Fixtures

Reference SSP fixtures under `resources/reference_ssp/` may contain a nested Git
repository and generated `build/` content. Do not edit generated outputs unless
the task is explicitly about fixture generation or expected reference data.
When editing this tree, follow `resources/reference_ssp/AGENTS.md`.

Use focused Catch2 tests, focused pytest tests, or one reference SSP simulation
as the smallest validation that proves a change before running broader suites.
