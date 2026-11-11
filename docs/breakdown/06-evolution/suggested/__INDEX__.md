---
title: Suggested
summary: Improvement candidates not yet selected for development
---

# Suggested

## Contents

<!-- pb:index:start -->
<!-- GENERATED FILE — do not edit. Regenerate with `pb registers`. -->
- **IMP-007** [Rename jacobi_serial to Gauss-Seidel plus hybrid executor sketch](IMP-007.md) — Rename the misnamed jacobi_serial executor to Gauss-Seidel and sketch a hybrid executor on the renamed family
- **IMP-021** [Implement a working ParallelSeidel DAG scheduler](IMP-021.md) — Replace the throwing ParallelSeidel stub with a thread-safe parallel DAG scheduler over the serial Seidel algorithm
- **IMP-026** [Direct rerun pipelined Gauss-Seidel executor evaluation](IMP-026.md) — Evaluate pipelining a Gauss-Seidel DAG so node N+1 overlaps node N; FMI analysis shows it is not viable
- **IMP-030** [Remote result ingestion via InfluxDB Line Protocol](IMP-030.md) — Add a remote result sink that emits simulation results over InfluxDB Line Protocol for multi-source ingestion
- **IMP-031** [OpenTelemetry distributed observability](IMP-031.md) — Add OpenTelemetry tracing so cross-process simulations are observable as one correlated trace
- **IMP-035** [XML signal list with per-sink behavior](IMP-035.md) — Let a standalone XML signal list select recorded signals and override per-sink behavior
- **IMP-037** [Pause, play, stop and set-speed lifecycle controls for the public API](IMP-037.md) — Add interactive lifecycle control (pause/play/stop/set_speed) to the C++, C and Python public API
- **IMP-043** [Unit test coverage for boundary connection resolution](IMP-043.md) — Add focused unit tests for resolve_model_key and resolve_connection, which currently have zero direct coverage
- **IMP-045** [Investigate signal_sine_gain_add tree/graph output discrepancy](IMP-045.md) — Re-run signal_sine_gain_add and determine whether the tree_output/graph_output discrepancy is a real output-generation bug
- **IMP-047** [Replace vendored reference SSPs with small in-repo e2e fixtures](IMP-047.md) — Drop the resources/reference_ssp submodule, add small in-repo fixtures, and outsource full e2e to ssp_references
- **IMP-048** [Dependency-ordered FMU initialization for SSPs needing connected inputs at init](IMP-048.md) — Initialize co-simulation FMUs source-first so downstream FMUs see committed upstream outputs before exiting init
- **IMP-049** [Remove the recorder from the executor](IMP-049.md) — Remove the recorder from the executor and inject it into models earlier in the construction path
- **IMP-050** [Split graph_executor into macro and realtime macro step executors](IMP-050.md) — Make graph_executor.cpp a pure macro step executor and split out a realtime macro step executor
- **IMP-051** [Set the read target resolver during the pipeline](IMP-051.md) — Design and set the read target resolver during the pipeline build before patching
- **IMP-052** [Executors take ownership of invocables](IMP-052.md) — All executors should take ownership of the invocables passed to them, propagating the top-layer refactor downward
- **IMP-053** [Atomic id counter and static max_id on Node for vector indexing](IMP-053.md) — Give Node an atomic id and static max_id so node lookup becomes vector indexing, replacing the Invocable id
<!-- pb:index:end -->
