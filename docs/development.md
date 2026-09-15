# Development Guide
<!-- Layer: 03-implementation, 02-architecture, 05-operation -->


This page collects contributor conventions. Build commands live in
[Build From Source](build_from_source.md), test-suite details live in
[tests/README.md](../tests/README.md), and runtime configuration lives in
[Configuration](configuration.md).

## Repository Layout

For the complete repository structure, see the [code structure overview](../product-breakdown/03-implementation/code-structure.md).
For detailed architecture information, see the [architecture layer](../product-breakdown/02-architecture/).

## Core Development Loop

1. Build with the smallest relevant source-build workflow.
2. Run the focused test that proves the change.
3. Run one example simulation when behavior changes user-visible runtime output.
4. Update the relevant documentation when behavior, commands, architecture, or
   workflow assumptions change.

Use [Build From Source](build_from_source.md) for exact build commands and
[tests/README.md](../tests/README.md) for test selection.

## C++ Test Build Isolation

Kernel, parser, and analysis/graph tests under `tests/lib/` are standalone
per-test executables (`test_<name>`) whose sources are computed at configure
time as the transitive include cone of the test file. The cone rule relies on
the invariant that implementation `.cpp` files sit adjacent to their `.hpp`
under `lib/include/**` in normalized layout; keep that invariant when adding
implementation files. Do not hand-maintain cone lists — new test files are
picked up automatically. Integration tests (full-pipeline dependencies) are
listed explicitly in `SSP4SIM_INTEGRATION_TESTS` in `tests/lib/CMakeLists.txt`
and remain in the retained `ssp4sim_tests` binary. A bug in one `lib/` file
therefore blocks only the tests whose cones reach it.

## Coding Style

- C++ formatting follows a 4-space indent with braces on the next line.
- Class names use `PascalCase`; functions and files generally use
  `snake_case`.
- Keep includes ordered and local headers grouped consistently with nearby
  files under `lib/include/**`.
- No enforced formatter is checked in; match surrounding style closely.
- Keep changes direct and explicit. Minimize duplication, but do not introduce
  abstractions that hide important simulation or memory-layout details.
- Prefer repository-owned code in `lib/`, `public/`, `tests/`, `resources/`,
  and `resources/scripts/` over patching vendored dependencies in `3rdParty/`, unless
  the task is explicitly about vendored behavior.
- Treat this as active experimental software: clear root-cause fixes are
  preferred over compatibility shims and broad defensive workarounds.

## Dependencies And Generated Data

- CMake presets (`CMakePresets.json`) expect vcpkg; see
  [vcpkg.md](../vcpkg.md) for setup.
- Common optional build flags: `SSP4SIM_BUILD_TEST`,
  `SSP4SIM_BUILD_PYTHON_API`, and `SSP4SIM_LOG_HOT_PATH`.
- Use the repo-local `venv` for Python commands when it exists. Prefer
  `. venv/bin/activate && <command>` or `venv/bin/python <command>` over the
  system Python for workflow scripts and pytest.
See [Dependency Policy — Generated Data Policy](../product-breakdown/03-implementation/dependency-policy.md#generated-data-policy) for the authoritative policy on generated and fixture data, including Python environment guidelines.

## Configuration Work

Update the configuration reference when adding, removing, renaming, or changing
defaults for keys read from `utils::Config`.

## Executor Stacks And The Single-Resolver Rule

Executors nest: `ExecutorBase` is an `Invocable` whose children may themselves be
executors (e.g. `MacroExecutor -> SerialSeidel ->
LinearSubstepExecutor -> FmuModel`). When composing a stack:

- **Dispatch is a strategy map.** `ExecutorBuilder` holds a
  `method name -> Factory` map (`ExecutorBuilder::builders_`), where a `Factory`
  builds the specialized executor for one `simulation.executor.method` value.
  Each registered factory is the single place that reads its family's
  `simulation.executor.*` tuning keys and hands the resolved values to the
  executors' constructors (constructor injection); the executors themselves
  never read the global `utils::Config`. Unknown methods fail the lookup;
  `parallel_seidel` / `parallel-seidel` register a throwing factory.
- **la2 is a factory, not a class.** There is no `La2Scheduler` type;
  `make_la2_stack(nodes, La2Options)` (`executor/loop_aware/la2_builder.hpp`)
  is a config-free, pure construction function that SCC-partitions the graph,
  wraps each loop SCC in a `LinearSubstepExecutor` /
  `GeometricSubstepExecutor`, condenses the component DAG
  (`utils/graph/rewire.hpp`), and returns the outer `SerialSeidel` over the
  condensed graph. `ExecutorBuilder`'s `la2` / `loop_aware` strategies fill a
  `La2Options` from config and call it. `options.parallel` requests
  `ParallelSeidel` as the outer executor; while that stub is unimplemented it
  throws during assembly.
- **The stack-wide resolver is broken out of the executors.** `make_la2_stack`
  derives the single `La2DataAccessResolver` from the SCC partition and
  installs it over every model itself (an explicit loop, the assembly-side
  counterpart of `ExecutorBase::set_resolver`). No executor owns the read
  policy. Inner executors are resolver-neutral when they have no meaningful
  default (the `LinearSubstepExecutor` / `GeometricSubstepExecutor` classes) —
  they never call `set_resolver`. Because the resolver is installed over all
  models after each inner executor ran its own constructor, it overwrites any
  default resolver an inner executor set on its direct `FmuModel` children.

Reusable building blocks that serve la2 but live outside it:

- `utils/graph/graph.hpp` — `ssp4sim::utils::graph::Graph` is the common
  graph-analysis interface (SCC detection, per-SCC loop classification,
  component DAG, topological sort, `verify_placement()`), composed from the
  building blocks below; `component_dag()` exposes the SCC DAG as
  `scc index -> successor scc indices`. Results are a snapshot of the
  adjacency at `analyze()` time — composition changes go through
  `utils/graph/rewire.hpp`, then `analyze()` is run again.
- `utils/graph/rewire.hpp` — connection-mutation building block:
  `condense_component_dag()` rewires a component DAG onto representative nodes
  (only representative adjacency is rebuilt; model data edges
  (`model->connections`) are untouched for resolver lookups), plus the
  `disconnect()` / `redirect()` primitives used to change graph composition.
- `utils/graph/topological_sort.hpp` — `topological_sort()` Kahn-orders the
  component DAG (shared `scc index -> successor scc indices` representation)
  and throws on a cycle.
- `LinearSubstepExecutor` / `GeometricSubstepExecutor` — self-contained executors
  (`executor/substep/`) that relax any group of nodes over a linear or shrinking
  sub-step schedule. Each owns its schedule construction (`build_schedule`
  static) and sweeps the group in parallel per sub-step; use them outside la2 to
  sub-step groups on equal or shrinking schedules.

When reusing `SeidelBase` with executors as nodes, remember that its node array
is positional (`index_of_id` maps the process-wide `Node` id -> array index), so executors can
sit alongside models in one outer graph.

## Profiling And Logging

- Build profiling notes: [docs/profiling.md](profiling.md)
- Logging conventions: [docs/logging_guidlines.md](logging_guidlines.md)

## Release Process

The release pipeline and packaging details are documented in
[docs/linux_binary_distribution.md](linux_binary_distribution.md).

Tagging example:

```bash
git tag v0.1.1
git push origin v0.1.1
```

## Contributing

Welcome to the project!
Open an issue or pull request with:

- As clear as possible problem/solution description
- Tests run
- Before/after output when simulation behavior changes

Commit messages should be short, imperative, and sentence case, for example
`Add substeps` or `Split jacobi implementations`.
