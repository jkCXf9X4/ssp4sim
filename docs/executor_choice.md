# Choosing an Executor Algorithm
<!-- Layer: 05-operation, 01-product -->

This page explains what each executor algorithm in `ssp4sim` does and helps you
pick one for a given model graph. Key reference and defaults are in
[Configuration](configuration.md); the architecture is described in
[Development](development.md).

## At a Glance

The executor is selected with `simulation.executor.method`. Every executor
evaluates every model once per macro step, but they differ in *which value a
model reads from its upstream producers* and in *how strongly coupled groups are
handled*.

| Method | Structure | Read policy (consumer → producer) | Best for |
|---|---|---|---|
| `jacobi` (default) | one sweep per macro step; optional parallel backends | `StartTime` — producer's step-start value | Feed-forward graphs, raw speed, parallel runs |
| `seidel` | one topological sweep per macro step | `EndTime` — producer's current-step output | Acyclic graphs where in-step propagation matters |
| `custom_delay`, `custom_delay_partial` | hard-coded topology (Sources / LET* / C* nodes) | `StartTime` | Delay-oriented benchmark graphs only |
| `la2` (alias `loop_aware`) | SCC condensation; loop SCCs sub-stepped; outer Gauss-Seidel | Mixed: cross-SCC `Latest`, intra-SCC `StartTime` | Algebraic loops / feedback within a timestep |

### Read policies

The read policy is the single most important difference between executors. It
decides which committed value a model sees from a producer:

- **`StartTime`** — the producer's value at the *start* of the macro step
  (Jacobi semantics). All models evaluate against last step's values; nothing
  propagates within the step. Deterministic and parallel-friendly, but coupling
  inside one step is not resolved.
- **`EndTime`** — the producer's value at the *end* of the step (Gauss-Seidel
  semantics). Downstream models see the producer's current-step output, so
  information flows along the DAG within one step.
- **`Latest`** — the newest committed value (zero-order hold), used for
  cross-component edges in `la2`.

## Jacobi

All models are evaluated once per macro step reading `StartTime` values, so no
model sees another model's current-step output. This makes the whole sweep
parallelizable; enable it with `simulation.executor.jacobi.parallel` and pick a
backend with `simulation.executor.jacobi.method` (`1` = TBB, `2` = spin pool,
`3` = futures; spin/futures use `simulation.executor.thread_pool_workers`).

- **Use when:** the graph is feed-forward (no algebraic loops), or you are
  willing to shrink `simulation.timestep` to make loop coupling small enough
  for a single pass. Fastest default.
- **Avoid when:** models form strong algebraic loops and you need the loop to
  converge within a macro step — a single Jacobi pass smooths the transient
  (see [Empirical measurements](#empirical-measurements)).

## Seidel

`seidel` runs a single topological sweep per macro step reading `EndTime`
values, so each downstream model sees its producers' current-step output. It
does **not** iterate loops — it is a single pass in topological order.

- **Use when:** the graph is acyclic and in-step propagation along the DAG is
  important.
- **Note:** `ParallelSeidel` is not implemented; `seidel.parallel` and the
  method name `parallel_seidel` throw.

## la2 (loop-aware)

`la2` (legacy method name `loop_aware`) is the executor for graphs with
**algebraic loops**: feedback paths that need several evaluations within a
macro step to converge.

### How it works

1. The model graph is partitioned into strongly connected components (SCCs).
2. Acyclic single-node SCCs stay as-is: they run **once per macro step**.
3. Each *loop* SCC (a multi-node SCC or a single node with a self-edge) is
   wrapped in a `SubstepExecutor` that sweeps the loop members over several
   **sub-steps** within the macro step. Sub-steps only advance time (models
   cannot be reset), so the loop relaxes as a moving wavefront toward the
   macro-step boundary.
4. The SCC component DAG is condensed and run by an outer `SerialSeidel`
   (Gauss-Seidel order across components).
5. Read policy: cross-SCC edges read `Latest` (each component sees the newest
   committed value of upstream components); intra-SCC edges read `StartTime`
   (each sub-step reads the previous sub-step's commitments — deterministic
   Jacobi-style relaxation inside the loop).

### Sub-step modes (`simulation.executor.la2.mode`)

- **`linear`** — the macro step is split into `simulation.executor.la2.iterations`
  equal sub-steps. `iterations` defaults to the SCC node count (`1` = no
  relaxation). Nested (loop-within-loop) SCCs need a higher count because their
  feedback path passes through the inner loop nodes as well.
- **`factor`** — each sub-step covers `simulation.executor.la2.factor` of the
  remaining time. Shrinking stops once the remaining time is at or below
  `simulation.executor.la2.threshold`, then the rest of the macro step is taken
  whole, so `[start, end]` is always fully covered. Smaller factors concentrate
  relaxation closer to the macro-step end.

Legacy aliases: `fixed` → `linear`, `geometric` → `factor`.

- **Use when:** the graph contains algebraic loops that must converge within a
  macro step, and you can pay the extra FMU calls.
- **Tune with:** `iterations` (loop depth / nested loops; the Python regression
  tests pin nested loops to 32), or `mode: factor` + `factor`/`threshold` to
  concentrate relaxation at the end of the step.

## Decision Guide

Start from the graph structure, not the performance target:

1. **Does the graph have algebraic loops?**
   - No (feed-forward): use `jacobi` (parallel if you need speed). `la2` also
     works — every SCC is acyclic, so no loop is sub-stepped.
   - Yes: use `la2`. If you cannot, shrink `simulation.timestep` and accept a
     single-pass (`jacobi` / `seidel`) approximation of the loop.
2. **Is in-step propagation along the DAG important and the graph acyclic?**
   - Use `seidel` (current-step `EndTime` reads).
3. **Is determinism of the loop relaxation important?**
   - `la2` gives deterministic Jacobi-style relaxation inside loop SCCs.
4. **Is this a delay/transport benchmark with the fixed `Sources`/`LET*`/`C*`
   topology?** Use `custom_delay` / `custom_delay_partial`.

## Known Limitations

- `ParallelSeidel` is **not implemented**: `simulation.executor.method:
  "parallel_seidel"`, `seidel.parallel: true`, and `la2.parallel: true` all
  throw a clear runtime error.
- `simulation.executor.sub_step` is reserved and not read by any executor;
  `la2` sub-stepping is configured via `simulation.executor.la2.*`.

## Empirical Measurements

Measured on `embrace_scen.ssp` (0–2000 s, 6 models, cooling-pack algebraic
loop):

- [`resources/embrace/executor_comparison.md`](../resources/embrace/executor_comparison.md) —
  jacobi vs `la2` results: steady state agrees within ~0.09% (temperatures) and
  ~0.05% (pressures); transients differ strongly during the cooling-pack
  transition (up to ~177 K in `TCool`); `la2` is ~4.4× slower wall-clock.
- [`resources/embrace/executor_timing_analysis.md`](../resources/embrace/executor_timing_analysis.md) —
  why `la2` is slower: the per-sub-step barrier and resolver sampling inflate
  per-call wall time inside loop SCC members; forward derivatives are a
  speed-up, not overhead.

## Related Documentation

- [Configuration](configuration.md) — key reference, types, defaults, and la2
  tuning keys.
- [Development](development.md) — executor stack architecture and the
  single-resolver rule.
- [Usage](usage.md) — running a simulation.
- Example la2 configs: [`resources/loop_aware_nested.json`](../resources/loop_aware_nested.json)
  (`linear`, 32 sub-steps) and
  [`resources/loop_aware_nested_geometric.json`](../resources/loop_aware_nested_geometric.json)
  (`factor`).