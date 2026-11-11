# Task: Enable Interpolated Data as Model Inputs

Investigation tracker. Continuously updated as the investigation progresses.

## Objective

Investigate the integration and enablement of the capability to use *interpolated data*
as input to models within a simulation. Scope covers:

1. What interpolation means in the current data flow (value-only ZOH vs. derivative-aided
   FMU-side interpolation).
2. Where the capability is already enabled vs. where it is missing.
3. The integration points (resolver read path, config surface, builder wiring) and the
   enablement surface (config keys, per-connection flags).
4. Verification approach and relationship to in-flight work (IMP-046).

## Status

**In progress — session 1 of 2026-09-17 (map, gap analysis, verified read-path timing,
integration design for B+C drafted, design decisions recorded).**

Decisions recorded (design-bc.md §10): (1) default method **`hermite`**; (2) la2 cross-SCC
re-stamp via a **dedicated `AccessMode::Interp`**; (3) **keep IMP-046 whole** (note
subsumption at implementation time); (4) run the **derivative slope check** (empirical,
queued as next task).

Artifacts in this directory:
- [`task.md`](task.md) — this tracker.
- [`design-bc.md`](design-bc.md) — integration design sketch for mechanisms B (linear) and
  C (Hermite): bracketing copy path, la2 cross-SCC `Interp` mode, config surface, code touch
  points, verification plan, risks, decisions.

## Context and Prior Work

| Artifact | Date | Relevance |
| --- | --- | --- |
| `resources/embrace/interpolation_evaluation.md` | 2026-07-03 | 4-phase plan: (1) linear time interpolation, (2) Hermite using derivatives, (3) sub-step extrapolation, (4) iteration-history extrapolation. Phase 1 partially scaffolded, never completed. |
| `resources/embrace/executor_timing_analysis.md` | 2026-09-16 | Forward derivatives are a speed-up mechanism (`canInterpolateInputs` FMUs interpolate instead of grinding). Derivative anchoring verified via `record_derivatives`. |
| `breakdown/06-evolution/backlog/under_development/IMP-046.md` | 2026-09-16 | In-development la2 work: warm-started SCC sub-step iteration **and sub-step input interpolation** (cross-SCC inputs sampled once per sub-step). |
| `breakdown/03-implementation/modules/data_access_and_scheduling_design.md` | — | Read-path design (Design C chosen); notes `find_next_valid` traversal as part of the read machinery. |

## Progress Log

### 2026-09-17 — Session 1: Repository map and gap analysis

**What was done**

- Mapped the model-input read path end to end.
- Audited interpolation-relevant infrastructure (storage, ring buffer, resolvers, builder wiring).
- Compared current state against `interpolation_evaluation.md` phases and IMP-046.
- Confirmed prior-plan status against git history (`git log`: "Derivate copy bug",
  "record derivate", "Documentation for la2" are the recent interpolation-adjacent commits).

**Key findings**

1. **The read path is zero-order hold, by design.** `FmuModel::pre()`
   (`model_fmu.cpp:149`) pushes an input area at `step_start` and calls
   `copy_model_inputs()`; `DataAccessResolver::copy_model_inputs()`
   (`data_access_resolver.cpp:290`) resolves each edge, and `copy_connection()`
   (`resolver_common.cpp:109`) picks the newest committed area `<=` the resolved
   time via `find_latest_valid_area()` and memcpys the value verbatim. No
   interpolation computation exists anywhere.
2. **Bracketing helpers exist but are dead scaffolding.** `RingBuffer::find_next_valid_index()`
   (`ring_buffer.cpp:214`) and `SignalStorage::find_next_valid_area()` (`storage.cpp:189`)
   were added (the Phase-1 scaffold from `interpolation_evaluation.md`) but are unused in
   production and referenced by **no test**.
3. **Derivative infrastructure is complete and enabled by default.**
   `SignalStorage` reserves per-variable derivative slots (`max_interpolation_orders`),
   `copy_connection` forwards them, `FmuModel::pre()` applies them
   (`apply_input_derivatives`), `FmuModel::post()` fetches output derivatives. Wired
   per-connection when the target FMU declares `canInterpolateInputs`
   (`sim_graph_builder_wiring.cpp:155-165`). Config: `simulation.executor.forward_derivatives`
   (default true).
4. **There is no enablement key for value interpolation.** `shared_config.hpp` has
   `forward_derivatives`, `record_derivatives`, and the la2 keys — no
   `interpolate_inputs` / `interpolation_method` config exists (the Phase-1 plan's
   proposed keys were never landed).
5. **Applicability window is narrower than the plan assumed.** `AccessMode` semantics:
   `StartTime`/`EndTime` resolve to a *time* (then ZOH search); `Latest`/`Index` pin a
   physical area (no time, interpolation impossible). With a uniform macro timestep,
   back-to-back flat stepping makes every `StartTime` read an **exact timestamp match**
   (producer committed at the same grid point), so generic time-interpolation would be a
   no-op there. Real gaps arise only when:
   - the producer's committed cadence is coarser than the consumer's request grid
     (the la2 sub-step case, IMP-046's target), or
   - edge `delay`/`time_offset` shift the sample off the producer's grid, or
   - the producer has already committed *newer* data within the macro step (serial seidel)
     — interpolating there would leak future data across the schedule and needs a
     semantics decision.
6. **IMP-046 (in development) is the active interpolation work.** Its "sub-step input
   interpolation" interpolates cross-SCC inputs *between* sub-steps inside the la2 SCC
   (using the forwarded order-1 derivative). That is la2-specific. The generic
   Phase-1/2 linear/Hermite time-interpolation (resolver-level, all families) is a
   separate, unlanded scope.

### 2026-09-17 — Session 1 (cont.): read-path timing verified, la2 sub-step traced

**What was done**

- Traced `JacobiSerial`/`JacobiParallel`/`SeidelBase` + `LinearSubstepExecutor` + `la2_builder`
  call sequences against the resolver's `AccessMode` semantics to determine where a read can
  actually fall *between* two committed areas.
- Read the derivative wiring rule (`sim_graph_builder_wiring.cpp:152-166`) and the derivative
  apply/fetch code (`model_connector.cpp:133-194`).

**Findings**

1. **Flat stepping always hits exact timestamp matches.** All flat families
   (`jacobi_serial`, `jacobi_parallel_*`, `seidel_serial`) step every model with the same
   `StepData(start, end)`, and `StartTime`/`EndTime` reads resolve to the producer's committed
   frontier on the same grid (producer committed at previous `end == current start`). So
   `find_latest_valid_area(ref)` is always an exact match — generic two-point value
   interpolation would be a **no-op** on the flat path with uniform timestep and no
   `delay`/`time_offset`. The only ways to hit a true gap there: heterogeneous timesteps, or
   `delay`/`time_offset` shifts (edges with delay are sampled as feedthrough wiring today).
2. **la2 intra-SCC reads are exact matches too.** Intra-SCC edges are stamped `StartTime`
   (`la2_data_access_resolver.cpp:50`); each SCC member commits at the previous sub-step's
   `end == current sub_start`, so the read is again exact. The 4× sub-step sweep is *not*
   where a bracketing gap appears.
3. **la2 cross-SCC reads ARE the real gap — and a bracketing pair exists.** Cross-SCC edges
   are stamped `Latest` (`la2_data_access_resolver.cpp:57`) → pinned to the producer's
   *newest committed* area = the macro **end** value v(T+dt) (Gauss-Seidel "latest committed"
   semantics), held constant across all sub-steps. **Correction vs. earlier draft:** the
   serial outer sweep runs cross-SCC producers first, so at every sub-step read t_k ∈ (T, T+dt)
   the producer has already committed **both** areas: T (previous macro end) and T+dt (this
   macro end). `find_latest_valid_area(t_k)` → T, `find_next_valid_area(t_k)` → T+dt. If the
   edge were time-sampled instead of Latest-pinned, a genuine two-point bracketing pair with
   values **and stored order-1 derivatives at both ends** would be available for every
   sub-step read. The pin is what currently makes the interpolation impossible, not missing
   committed data.
4. **Flat-path future-data concern is moot for interpolation at the lower bound.**
   `resolve_time` clamps ref to the committed frontier and `find_latest_valid_area` returns
   the exact area when ref lands on the producer's grid; linear/Hermite interpolation at the
   exact lower bound reproduces the lower value (frac = 0), so enabling two-point
   interpolation cannot leak future data on exact-match reads. It only activates when ref is
   strictly interior to a bracketing pair: la2 cross-SCC sub-step reads (finding 3), delay /
   `time_offset`-shifted edges, or heterogeneous timesteps.
5. **Three mechanisms, not two.** The capability decomposes into:
   - *A — FMU-internal input interpolation*: `canInterpolateInputs` FMUs interpolate between
     their internal solver steps when fed `set_real_input_derivative`. **Enabled by default**
     (`simulation.executor.forward_derivatives=true`), wired per-connection when
     `source.maxOutputDerivativeOrder>0 && target.canInterpolateInputs` (both real-typed).
     Measured as a speed-up, not a cost (timing analysis Q3). Note: 4 of 6 embrace models
     declare `canInterpolateInputs="true"` (ECS_HW, Consumer, Atmos, AdaptionUnit);
     `scenario` is `false`; `ECS_SW` omits it.
   - *B — resolver-side two-point value interpolation* (linear between committed areas):
     **not implemented**; the bracketing pair exists for la2 cross-SCC sub-step reads (finding
     3) and for delay/heterogeneous-timestep reads on any family. Scaffolding
     (`find_next_valid_area`) exists but is dead code.
   - *C — resolver-side two-point Hermite interpolation using stored derivatives* (value +
     order-1 derivative at both bounds): **not implemented**; a strict refinement of B using
     data that is already stored and forwarded. IMP-046's "sub-step input interpolation" is
     this mechanism scoped to the la2 cross-SCC edges — but it can be implemented generically
     in the resolver copy path.
6. **IMP-046 overlap resolved.** IMP-046's sub-step input interpolation is exactly mechanism
   C applied to la2 cross-SCC inputs. Implementing B+C generically in the resolver read path
   **does subsume** IMP-046's interpolation item (the warm-start item remains la2-specific);
   it also covers delay/heterogeneous-timestep reads on the flat families for free.

## Findings (Consolidated)

- **Capability is ~80% infrastructurally present, 0% functionally present.**
  Storage of derivatives, forwarding, application, and ring-buffer bracketing all exist.
  The missing piece is the interpolation *computation* in the copy path plus its
  enablement/config surface.
- **Semantic risk is low for two-point interpolation when correctly anchored.** Because
  interpolation at the exact lower bound reproduces the lower value (frac = 0), enabling it
  cannot alter exact-match reads. It only activates when the resolved read time is strictly
  interior to a bracketing pair of committed areas.
- **Three mechanisms must not be conflated:**
  - *A — FMU-internal interpolation* (`canInterpolateInputs` + forwarded derivatives):
    enabled, default on, measured as a perf enabler (timing analysis Q3).
  - *B — resolver-side two-point linear value interpolation*: not implemented; the concrete
    exercising case is la2 cross-SCC sub-step reads (edge currently Latest-pinned) plus
    delay/heterogeneous-timestep reads. Scaffolding (`find_next_valid_area`) exists.
  - *C — resolver-side two-point Hermite interpolation using stored derivatives*: not
    implemented; same bracketing pair as B with both values and order-1 derivatives.
    This is IMP-046's "sub-step input interpolation" item; a generic resolver-level
    implementation subsumes it.

## Open Questions

1. (resolved) Which edge configurations genuinely hit a "between two committed areas" read?
   → la2 cross-SCC sub-step reads (T and T+dt both committed before the SCC runs — finding 3),
   delay/`time_offset`-shifted reads on any family, heterogeneous-timestep reads. Flat
   exact-match reads reproduce the lower bound exactly (frac = 0).
2. (decided 2026-09-17) la2 cross-SCC policy: interpolation is opt-in behind the gate via a
   **dedicated `AccessMode::Interp`** (not a `Latest`→`StartTime` re-stamp). Gate off →
   `Latest` pin, current la2 numerics; gate on → `Interp`, interpolated sub-step estimates.
   Numerical-path shift must still be verified against the embrace envelopes (steady state
   0.09%/0.05%, transient TCool t=90, `executor_comparison.md`) — see verification plan.
3. (decided 2026-09-17) Gate surface: global keys `simulation.executor.interpolate_inputs`
   (default false) + `simulation.executor.interpolation_method` (default `hermite`), parsed
   in `ExecutorOptions::load()`, threaded through the resolver construction. Per-connection
   `forward_derivatives` wiring is reused as the Hermite availability gate.
4. (open — empirical) Verify the FMU-reported output derivative
   (`get_real_output_derivative`, model_connector.cpp:197) is the right slope for Hermite at
   both macro bounds: compare `record_derivatives` `.d1` against a forward-difference of the
   recorded trajectory on a smooth segment. This is the next investigation task (decision 4).
5. (decided 2026-09-17) IMP-046: keep whole; the generic B/C design subsumes its
   interpolation half, the warm-start half stays la2-specific. A subsumption note goes into
   `IMP-046.md` only when the implementation lands.

## Next Steps

1. **Scope decided (user): both B and C.** Design the generic resolver-level two-point
   interpolation (linear = B, Hermite = C) behind a config gate, with the la2 cross-SCC
   sub-step read as the concrete exercising case and delay/heterogeneous-timestep reads as
   latent beneficiaries.
2. **Integration sketch drafted + decisions recorded** → [`design-bc.md`](design-bc.md):
   dedicated `AccessMode::Interp` for la2 cross-SCC edges (gate-off path unchanged);
   bracketing copy path in `copy_connection`; config surface (`interpolate_inputs`,
   `interpolation_method` default `hermite`); code touch points; verification plan; risks.
3. **Next task — derivative slope check (decision 4):** compare `record_derivatives` `.d1`
   against forward-difference slopes on a smooth segment (e.g., embrace steady-state ramp) to
   confirm the FMU-reported output derivative is the Hermite bound derivative.
4. Implement the design (config → resolver → copy path → unit + integration verification)
   and, only after code lands, add the subsumption note to `IMP-046.md`.
5. Close the loop into `06-evolution`: IMP-046 bookkeeping (interpolation half subsumed,
   warm-start half stays), and propose a new backlog candidate for generic enablement if the
   design lands.

## Definition of Done (for the investigation)

- A written map of the interpolation capability: what exists, what is missing, where it
  integrates, how it is enabled.
- A scope decision (generic resolver interpolation vs. la2-only vs. both) grounded in the
  read-path analysis above.
- A proposed integration sketch (config + code touch points) and a verification plan.
- Findings linked into `06-evolution` (backlog candidates / IMP-046 notes) as appropriate.