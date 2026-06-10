# Improvement Backlog Overview

subfolders:
candidates - not selected for development
selected - items to be incorporated
done - historical tracking of items



Use this template as the landing area for accepted continuous-improvement candidates.

Generated from improvement workflow:

```text
intake -> broad read-only discovery -> architecture/requirement pressure analysis -> backlog candidates -> final report
```

Each candidate is proposed. None is approved for implementation until it has a scoped task contract.

## Usage

Place the overview at the repository's chosen evolution or backlog location, such as:

```text
product-breakdown/06-evolution/backlog/improvement-backlog.md
```

Put each candidate in its own file beside the overview or in a local `candidates/` directory. Move completed implementations to a local `completed/` directory when the repository uses one.

## Individual Candidates

| File | ID | Theme | Status | Priority | Blast radius |
| --- | --- | --- | --- | --- | --- |
| `candidates/IMP-001.md` | IMP-001 | Deduplicate profiling content between PBS and docs | Proposed | High | 2 files (PBS 05-operation/profiling.md, docs/profiling.md); low risk, text removal only |
| `candidates/IMP-002.md` | IMP-002 | Deduplicate configuration schema tables between PBS and docs | Proposed | High | 2 files (PBS 03-implementation/configuration.md, docs/configuration.md); medium risk, schema tables duplicated |
| `candidates/IMP-003.md` | IMP-003 | Deduplicate contributing/development workflow between PBS and docs | Proposed | Medium | 2 files (PBS 06-evolution/contributing.md, docs/development.md); low risk |
| `candidates/IMP-004.md` | IMP-004 | Tighten release-pipeline PBS boundary to design-only | Proposed | Low | 2 files (PBS 05-operation/release-pipeline.md, docs/linux_binary_distribution.md); low risk |
| `candidates/IMP-005.md` | IMP-005 | Deduplicate generated-data policy between docs and PBS | Proposed | Low | 2 files (docs/development.md, PBS 03-implementation/dependency-policy.md); low risk |
| `candidates/IMP-006.md` | IMP-006 | Reduce docs/README.md to pure landing page with PBS links | Proposed | Low | 1 file (docs/README.md); low risk |
| `candidates/IMP-007.md` | IMP-007 | Rename jacobi_serial to Gauss-Seidel + hybrid executor design sketch | Proposed | Medium | Rename: executor_builder.cpp, docs/configuration.md, module docs (moderate). Design sketch: documentation only (low) |
| `candidates/IMP-008.md` | IMP-008 | Newton iteration executor feasibility assessment for algebraic-loop resolution | Proposed | Low | Design sketch only; no implementation (low). If later implemented: large (new solver, interface changes) |
| `candidates/IMP-009.md` | IMP-009 | SignalStorage pointer indirection — close risk as non-bottleneck | Closed | N/A | None — analysis only |
| `candidates/IMP-010.md` | IMP-010 | Remove logging guidelines from PBS signal.md — domain mismatch | Proposed | High | 2 files (PBS signal.md, docs/logging_guidlines.md); text removal only |
| `candidates/IMP-011.md` | IMP-011 | Remove coding style rules list from PBS architecture decision | Proposed | High | 1 file (PBS IMD-003.md); text removal only |
| `candidates/IMP-012.md` | IMP-012 | Consolidate "ctest is unreliable" note to one canonical location | Proposed | Medium | 3 files (docs/build_from_source.md, tests/README.md, PBS test-strategy.md); minor text changes |
| `candidates/IMP-013.md` | IMP-013 | Consolidate fmi4c mode-bit docs to one canonical location | Proposed | Medium | 3 files (PBS dependency-policy.md, PBS test-strategy.md, tests/README.md); minor text changes |
| `candidates/IMP-014.md` | IMP-014 | Deduplicate fmi2Error/fmi2Fatal cleanup documentation | Proposed | Medium | 2 files (PBS handler.md, tests/README.md); one-line change |
| `candidates/IMP-015.md` | IMP-015 | Deduplicate xfail docs between tests/README.md and PBS regressions | Proposed | Low | 2 files (tests/README.md, PBS regressions.md); text removal only |
| `candidates/IMP-016.md` | IMP-016 | Canonicalize project purpose statement to PBS intent layer | Proposed | Low | 3 files (readme.md, docs/README.md, PBS purpose.md); minor text changes |
| `candidates/IMP-017.md` | IMP-017 | Deduplicate test pyramid description between tests/README.md and PBS strategy | Proposed | Low | 2 files (tests/README.md, PBS test-strategy.md); minor text changes |
| `candidates/IMP-018.md` | IMP-018 | Fix inline ParameterSet namespace bug in ssp1_ssd.toml | Proposed | High | 1 TOML file, generated code, test xfail removal, regression doc; fixes correctness bug |
| `candidates/IMP-019.md` | IMP-019 | Fix component-level parameter bindings in get_parameter_bindings() for dcmotor/baseline (REG-002) | Proposed | Medium | ssp.cpp core rewrite, composite SSP test fix, regression doc; medium to high risk — affects all component-level parameter resolution |
| `candidates/IMP-020.md` | IMP-020 | Init-Phase Algebraic Loop Iteration — feasibility evaluation | Proposed | Medium | executor.cpp init sequence, FmuModel suppress_recorder flag, no step-phase impact; low risk, FMI-standard endorsed |
| `candidates/IMP-021.md` | IMP-021 | Implement a working ParallelSeidel DAG scheduler | Proposed | High | seidel_parallel.hpp/cpp, seidel_base.hpp, executor_builder.cpp, test file; moderate blast radius, data race risk if atomics omitted |
| `done/IMP-022.md` | IMP-022 | Internal node connections in the analysis graph | Done | Medium | analysis_graph_builder.cpp only; low risk, uncomment and fix dormant code |
| `candidates/IMP-023.md` | IMP-023 | Variable internal feedthrough storage in sim graph connections | Proposed | Medium | model_connection.hpp, graph_builder.cpp, model_fmu.hpp; moderate blast radius, no behavioral change |
| `candidates/IMP-024.md` | IMP-024 | Init-phase algebraic loop iteration — gap analysis on IMP-020 | Proposed — Design Evaluation Only | Medium | executor.cpp, executor.hpp, model_fmu.cpp, model_fmu.hpp, graph.cpp; low risk, FMI-standard endorsed |
| `candidates/IMP-025.md` | IMP-025 | Seidel-Jacobi hybrid executor (building on IMP-021) | Proposed | Medium | New execution/hybrid/ module, executor_builder.cpp; moderate blast radius, depends on IMP-021 |
| `candidates/IMP-026.md` | IMP-026 | Direct rerun (pipelined) Gauss-Seidel executor | Design Evaluation Only | Low | No code changes; design document only; highly speculative, FMI-incompatible |
| `candidates/IMP-027.md` | IMP-027 | Config item for input storage in output artifacts (CSV/SQLite) | Proposed | Medium | shared_config.hpp, model_fmu.hpp/cpp, graph_builder.cpp; small blast radius, opt-in via config |
| `done/IMP-028.md` | IMP-028 | Restructure thread pools with tradeoff documentation | Done | Medium | Documentation only (README); deferred file move; moderate blast radius if moved |
| `done/IMP-029.md` | IMP-029 | SQLite WAL local database sink | Done | Medium | Recording config, recorder sink interfaces, docs; CSV compatibility must be preserved |
| `candidates/IMP-030.md` | IMP-030 | Remote result ingestion via InfluxDB Line Protocol | Proposed | Medium | Remote sink config, write path, retry/backpressure, docs; local outputs preserved |
| `candidates/IMP-031.md` | IMP-031 | OpenTelemetry distributed observability | Proposed | Medium | Trace emission, context propagation, exporter config, docs; local logging preserved |
| `done/IMP-032.md` | IMP-032 | Per-simulation SQLite database files for concurrent write parallelism | Done | High | SQLite sink, config, simulation orchestration, 6+ docs, 7 tests; opt-in mode mitigates breakage |
| `candidates/IMP-033.md` | IMP-033 | Refactoring & Cleanup of Graph Builder Files | Proposed | Medium | analysis_graph_builder.cpp/.hpp, graph_builder.cpp/.hpp, test files; low-medium risk per finding |
| `candidates/IMP-034.md` | IMP-034 | Strip DuckDB result feature | Proposed | High | 5 source files deleted, 1 test deleted, 2 Python scripts deleted, build system (3 files), config (2 files), 30+ doc artifacts; large blast radius but mechanical removal |
| `candidates/IMP-035.md` | IMP-035 | XML signal list with per-sink behavior | Proposed | Medium | Config pipeline, recorder interface, DataRecorder, new XML file format, docs; backward compatible when absent |
| `candidates/IMP-036.md` | IMP-036 | Unified analysis graph node vector with Tarjan SCC for feedthrough detection | Proposed — Design Evaluation Only | Low-Medium | analysis_graph.hpp, analysis_graph.cpp, analysis_graph_builder.cpp, analysis_graph_builder.hpp |
| `candidates/IMP-037.md` | IMP-037 | Pause, Play, Stop, and Set-Speed Lifecycle Controls for the Public API | Proposed | Medium | 10+ source files, C API, Python bindings, 5+ doc artifacts; large blast radius, threading and thread-safety risk |
| `candidates/IMP-038.md` | IMP-038 | Clarify and Streamline Analysis Graph vs Simulation Graph Responsibilities | Proposed | Medium-High | analysis_graph.hpp/cpp, analysis_graph_builder.hpp/cpp, graph_builder.hpp/cpp, graph.hpp/cpp, AD-003, module docs; large blast radius, high implementation complexity |
| `candidates/IMP-039.md` | IMP-039 | Evolve Analysis Graph into an Analysis System with Nested-SSP Support | Proposed | Medium-High | analysis_graph.hpp/cpp, analysis_graph_builder.hpp/cpp, graph_builder.hpp/cpp, simulation.cpp, SSP parser, AD-003, module docs; large blast radius, high implementation complexity |
 
    
## Summary

| ID | Theme | Priority | Prerequisite | Blast radius |
| --- | --- | --- | --- | --- |
| IMP-001 | Deduplicate profiling content between PBS and docs | High | None | 2 files, text removal only |
| IMP-002 | Deduplicate configuration schema tables between PBS and docs | High | None | 2 files, section removal in PBS |
| IMP-003 | Deduplicate contributing/development workflow between PBS and docs | Medium | None | 2 files, minor text changes |
| IMP-004 | Tighten release-pipeline PBS boundary to design-only | Low | None | 2 files, minor text changes |
| IMP-005 | Deduplicate generated-data policy between docs and PBS | Low | None | 2 files, minor text changes |
| IMP-006 | Reduce docs/README.md to pure landing page with PBS links | Low | None | 1 file, minor text changes |
| IMP-007 | Rename jacobi_serial to Gauss-Seidel + hybrid executor design sketch | Medium | None | Rename: configuration + docs (moderate). Design sketch: docs only (low) |
| IMP-008 | Newton iteration executor feasibility assessment | Low | None | Design sketch only (low). Implementation deferred |
| IMP-009 | SignalStorage pointer indirection — close risk | N/A | None | None — analysis only |
| IMP-010 | Remove logging guidelines from PBS signal.md — domain mismatch | High | None | 2 files, text removal |
| IMP-011 | Remove coding style rules list from PBS architecture decision | High | None | 1 file, text removal |
| IMP-012 | Consolidate "ctest is unreliable" note to one canonical location | Medium | None | 3 files, minor text changes |
| IMP-013 | Consolidate fmi4c mode-bit docs to one canonical location | Medium | None | 3 files, minor text changes |
| IMP-014 | Deduplicate fmi2Error/fmi2Fatal cleanup documentation | Medium | None | 2 files, minor text changes |
| IMP-015 | Deduplicate xfail docs between tests/README.md and PBS regressions | Low | None | 2 files, minor text changes |
| IMP-016 | Canonicalize project purpose statement to PBS intent layer | Low | None | 3 files, minor text changes |
| IMP-017 | Deduplicate test pyramid description between tests/README.md and PBS strategy | Low | None | 2 files, minor text changes |
| IMP-018 | Fix inline ParameterSet namespace bug in ssp1_ssd.toml | High | None | 1 TOML file + generated code + test xfail removal; correctness fix |
| IMP-019 | Fix component-level parameter bindings in get_parameter_bindings() for dcmotor/baseline (REG-002) | Medium | None | ssp.cpp core rewrite + fixture test fix + regression doc; medium-high risk |
| IMP-020 | Init-Phase Algebraic Loop Iteration — feasibility evaluation | Medium | None | executor.cpp init sequence + FmuModel flag; low risk, FMI-standard endorsed |
| IMP-021 | Implement a working ParallelSeidel DAG scheduler | High | None | seidel_parallel.hpp/cpp, seidel_base.hpp, executor_builder.cpp, test file; moderate blast radius, data race risk if atomics omitted |
| IMP-022 | Internal node connections in the analysis graph | Medium | None | analysis_graph_builder.cpp; low risk, uncomment/commented-out dormant code |
| IMP-023 | Variable internal feedthrough storage in sim graph connections | Done | None | model_connection.hpp, graph_builder.cpp; no behavioral change, enabler for IMP-024/025 |
| IMP-024 | Init-phase algebraic loop iteration — gap analysis on IMP-020 | Medium | None | executor.cpp, model_fmu.cpp; low risk, FMI-standard endorsed; IMP-022/023 optional (see candidate) |
| IMP-025 | Seidel-Jacobi hybrid executor (building on IMP-021) | Medium | IMP-021 | New execution/hybrid/ module; moderate blast radius |
| IMP-026 | Direct rerun (pipelined) Gauss-Seidel executor | Low | None | No code changes; design evaluation only — FMI-incompatible, not viable |
| IMP-027 | Config item for input storage in output artifacts (CSV/SQLite) | Medium | None | shared_config.hpp, model_fmu.hpp/cpp, graph_builder.cpp; small blast radius |
| IMP-028 | Restructure thread pools with tradeoff documentation | Medium | None | Documentation only (README); deferred file move; enabler for IMP-021/025 |
| IMP-029 | SQLite WAL local database sink | Medium | None | Recording config + local database sink; CSV compatibility preserved |
| IMP-030 | Remote result ingestion via InfluxDB Line Protocol | Medium | None | Remote sink config + write path; local outputs preserved |
| IMP-031 | OpenTelemetry distributed observability | Medium | None | Trace emission + exporter config; local logging preserved |
| IMP-032 | Per-simulation SQLite database files for concurrent write parallelism | High | None | SQLite sink, config, simulation orchestration, 6+ docs, 7 tests; opt-in mode mitigates breakage |
| IMP-033 | Refactoring & Cleanup of Graph Builder Files | Medium | None | analysis_graph_builder.cpp/.hpp, graph_builder.cpp/.hpp, test files; low-medium risk per finding |
| IMP-034 | Strip DuckDB result feature | High | None | 5 source files + 1 test + 2 scripts deleted; 30+ doc artifacts updated; mechanical removal, no behavioral change |
| IMP-035 | XML signal list with per-sink behavior | Medium | None | Config pipeline + recorder filtering + XML parsing; backward compatible when absent |
| IMP-036 | Unified analysis graph node vector with Tarjan SCC for feedthrough detection | Low-Medium | None | analysis_graph.hpp, analysis_graph.cpp, analysis_graph_builder.cpp, analysis_graph_builder.hpp |
| IMP-037 | Pause, Play, Stop, and Set-Speed Lifecycle Controls for the Public API | Medium | None | 10+ source files, C API, Python bindings, 5+ doc artifacts; large blast radius, threading risk |
| IMP-038 | Clarify and Streamline Analysis Graph vs Simulation Graph Responsibilities | Medium-High | None | analysis_graph.hpp/cpp, analysis_graph_builder.hpp/cpp, graph_builder.hpp/cpp, graph.hpp/cpp, AD-003, module docs; large blast radius, high implementation complexity |
| IMP-039 | Evolve Analysis Graph into an Analysis System with Nested-SSP Support | Medium-High | None | analysis_graph.hpp/cpp, analysis_graph_builder.hpp/cpp, graph_builder.hpp/cpp, simulation.cpp, SSP parser, AD-003, module docs; large blast radius, high implementation complexity |
    
## Cross-Cutting Constraints

1. **Layer ownership**: `product-breakdown/` holds "what the product is, who it is for, scope, stable decisions." `docs/` holds "runnable guidance: usage, install, build, development workflow, examples, profiling commands."
2. **No duplication**: If a doc needs product context, it must link to `product-breakdown/` rather than copying text. If a PBS file needs runnable commands, it must link to `docs/` rather than reproducing them.
3. **Traceability preservation**: Every PBS file's Sources field that references a `docs/` file is correct and must be preserved. The deduplication removes content but keeps the cross-reference.
4. **Stay out of feature diffs**: None of these candidates should be bundled into unrelated feature or bug-fix work. Each candidate is a standalone documentation-hygiene task.
5. **`AGENTS.md` and `readme.md`**: These files reference both directories and may need link updates if files referenced in their link lists change. Any such changes must be scoped within the candidate's task contract, not added as incidental edits to other work.
