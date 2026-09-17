# Design Sketch: Resolver-Side Two-Point Input Interpolation (Mechanisms B and C)

**Investigation:** `06-evolution/investigations/interpolated-inputs`
**Tracking:** [`task.md`](task.md)
**Date:** 2026-09-17 (session 1)
**Status:** Draft — design only, no code changes made

This document sketches the integration of two-point input interpolation into the resolver
read path. Mechanism **B** = linear interpolation between two committed areas; mechanism
**C** = cubic Hermite interpolation using the stored order-1 derivatives at both bounds.
The concrete exercising case is the la2 cross-SCC sub-step read; delay/heterogeneous-timestep
reads on any family are latent beneficiaries.

## 1. Recap: Current Read Path

```
FmuModel::pre(step_start, step_end)                 model_fmu.cpp:149
  └─ access_resolver->copy_model_inputs(target, target_area, step_start, step_end)
       └─ for each connection i:                    data_access_resolver.cpp:290
            r = resolve(model_id, i, step_start, step_end)     // ResolvedRead
            copy_connection(c, target_area, r)      resolver_common.cpp:109
                 ├─ r.is_area  → pinned area (Latest/Index)
                 ├─ r.time     → find_latest_valid_area(r.time)   // ZOH, verbatim memcpy
                 └─ forward derivatives (if c.forward_derivatives)
```

- `AccessMode::StartTime`/`EndTime` → `resolve_time()`: `ref = base + time_offset − delay`,
  clamped to the producer's committed frontier; **not** an exact grid guarantee.
- `AccessMode::Latest` → `resolve_latest()`: pinned newest committed area.
- `time_offset` is never set non-zero today; the only time shift in use is `delay`.

## 2. When Interpolation Is Active (and When It Is a No-Op)

`copy_connection` on the time-resolved path currently does
`find_latest_valid_area(r.time, source_area)` and memcpys. The bracketing extension:

```
lower = find_latest_valid_area(r.time)            // area with time ≤ r.time
if get_time(lower) == r.time:  exact match → memcpy (frac = 0, no-op — SAFE)
upper = find_next_valid_area(r.time)              // area with time > r.time (exists already)
if no upper:                     → memcpy (ZOH fallback, today's behavior)
else:                            → interpolate v(r.time) ∈ [v(lower), v(upper)]
```

Key properties:
- **Exact-match reads are numerically unchanged** (frac = 0 ⇒ interpolant = lower value).
  This is what makes the feature backward compatible for the flat families and the la2
  intra-SCC path (both exact-match by construction — see task.md findings 1–2).
- **Interpolation only activates strictly between two committed areas.** Today that happens
  for la2 cross-SCC sub-step reads (task.md finding 3: both `T` and `T+dt` are committed
  before the SCC runs in the serial outer sweep) and for `delay`-shifted reads.
- **`find_next_valid_area` / `find_next_valid_index` already exist** but are dead code
  (no production use, no tests).

## 3. Mechanism B — Linear

For `c.type == real` only (other types keep memcpy/ZOH):

```
frac = (r.time − t_lower) / (t_upper − t_lower)
v = v_lower + frac * (v_upper − v_lower)
```

## 4. Mechanism C — Cubic Hermite (uses stored derivatives)

For `c.type == real` **and** `c.forward_derivatives && c.forward_derivatives_order ≥ 1`
(guarantees the source's order-1 derivative slots are populated for this signal — they are
filled by `fetch_output_derivatives`, model_connector.cpp:165):

```
ξ = (r.time − t_lower) / (t_upper − t_lower)
h00 = 2ξ³ − 3ξ² + 1      h10 = ξ³ − 2ξ² + ξ
h01 = −2ξ³ + 3ξ²         h11 = ξ³ − ξ²
v = h00·v_l + h10·dt·dv_l + h01·v_u + h11·dt·dv_u
```

Fallbacks (graceful, in order): derivative slots missing → **linear**; non-real type or no
bracketing pair → **memcpy**. Note the ring buffer is zero-initialized
(`make_aligned_buffers` memsets 0, allocator.cpp:40), so Hermite with unfetched slots
degenerates to exactly linear even if the gate is missed — but the explicit `forward_derivatives`
gate is the intended path.

Derivative forwarding to the target continues unchanged for both mechanisms (the FMU-side
interpolation, mechanism A, is untouched).

## 5. The la2 Cross-SCC Policy Change (the one behavioral switch)

**Decision (2026-09-17): a dedicated `AccessMode::Interp` mode** (not a `Latest`→`StartTime`
re-stamp). `Interp` resolves a reference time exactly like `resolve_time` (frontier-clamped,
floored at 0, shifted by `delay`/`time_offset`) but is a distinct mode so the la2 cross-SCC
policy stays explicit and the `Latest` contract ("newest committed index, no time lookup",
uc-14 stale-only intent) is untouched.

| Config gate | cross-SCC edge stamp | sub-step input seen by SCC |
| --- | --- | --- |
| `interpolate_inputs: false` (default) | `Latest` (unchanged) | v(T+dt) held for all sub-steps — current la2 numerics |
| `interpolate_inputs: true` | `AccessMode::Interp` | interpolated estimate v(t_k), t_k ∈ (T, T+dt) |

The stamp is chosen at resolver construction from the gate, so the gate-off path is
bit-identical to today. `Interp` only exists when the gate is on; defensively it falls back to
ZOH (newest committed) when no upper bound is committed yet. This matches IMP-046's
"opt-in via existing la2 config keys; default behavior unchanged until proven".

Flat families: mode unchanged (`StartTime`); the gate only enables the bracketing math, which
is a no-op on exact-match reads and activates only for delay-shifted reads.

## 6. Config Surface

New keys (naming follows the 2026-07-03 `interpolation_evaluation.md` plan):

```
simulation.executor.interpolate_inputs   bool, default false    // master gate (B + C)
simulation.executor.interpolation_method string, default "hermite" // "linear" | "hermite"
```

**Decision (2026-09-17): default method is `hermite`.** Parse site:
`ExecutorOptions::load()` (shared_config.hpp:59), exposed alongside `la2`. The method value is
normalized once; `hermite` is internally downgraded to `linear` per-wire when
`forward_derivatives` is absent (derivative slots unfetched), and to memcpy for non-real types
or missing brackets.

## 7. Code Touch Points

| File | Change |
| --- | --- |
| `shared_config.hpp` | parse `interpolate_inputs` (default false) + `interpolation_method` (default `"hermite"`) in `ExecutorOptions::load()`; carry in `ExecutorOptions` |
| `resolver/resolver_common.hpp` | add `AccessMode::Interp`; `EdgeAccessRules` gains `bool interpolate = false` and an interpolation-method enum; `copy_connection` signature extended with the interpolate policy (or reads it from `c`) |
| `resolver/resolver_common.cpp` | `resolve_edge` routes `Interp` through `resolve_time` (same frontier-clamped recipe); bracketing branch in `copy_connection` time path: exact-match fast path, `find_next_valid_area` upper bound, Hermite math (default) with linear → memcpy fallbacks |
| `resolver/data_access_resolver.hpp/.cpp` | constructor/`install_flat_resolver` accept the gate; `copy_model_inputs` threads the per-edge `interpolate` flag into `copy_connection`; `DataAccessResolver` default rules carry `interpolate` |
| `resolver/la2_data_access_resolver.hpp/.cpp` | constructor accepts the gate; when on, stamp cross-SCC edges `AccessMode::Interp` (instead of `Latest`); when off, `Latest` unchanged |
| `executor/loop_aware/la2_builder.cpp` | pass the gate/method into `La2DataAccessResolver` |
| `executor_builder.cpp` | pass the gate into `install_flat_resolver` calls |
| `docs/configuration.md` | document the two keys |

No changes to `FmuModel`, `ConnectorInfo::*`, storage layout, `canInterpolateInputs` wiring,
or the executor families' invoke paths.

## 8. Verification Plan

1. **Unit (new tests).** Ring-buffer/storage `find_next_valid_area` (currently untested);
   `copy_connection` interpolation: linear math, Hermite math, exact-match no-op, missing
   upper bound → ZOH, non-real type → ZOH, missing derivatives → linear, delay-shifted read.
2. **Integration (embrace, la2, `interpolate_inputs: true` vs baseline `false`):**
   - Steady state t=2000 within 0.09% (temperatures) / 0.05% (pressures) envelopes
     (`executor_comparison.md:119`).
   - Transient `TCool` t≈90 envelope (same doc).
   - Per-call model walltime vs baseline la2 (`executor_timing_analysis.md`); record FMU
     internal-step behavior via `canInterpolateInputs` consumers.
   - `simulation.recording.record_derivatives` `.d1` anchoring at sub-step times.
   - Determinism: two identical runs bit-identical.
3. **Regression:** default config (`interpolate_inputs` absent/false) produces numerically
   identical outputs to today on jacobi and la2 (the feature must be a strict no-op).

## 9. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation |
| --- | --- | --- | --- |
| Cross-SCC `Interp` mode shifts la2 numerics (by design) | Certain (when gate on) | Medium | Gate off by default; verify against embrace envelopes before any default flip |
| Hermite reads unpopulated derivative slots | Low | Low | Gate on `c.forward_derivatives`; zero-init buffer already degrades Hermite→linear |
| Per-read cost of bracketing lookup | Medium | Low | One extra ring scan only when interpolation on; can add a cheap `find_next` from the lower index |
| Interpolation on delay edges changes flat-family behavior unexpectedly | Low (no delay fixtures in embrace) | Low | Unit-tested; exact-match fast path keeps default runs identical |
| Multi-wire derivative slot sharing (two consumers, one qualifies) | Low | Low | Hermite gate is per-wire (`forward_derivatives`); non-qualifying wire falls back to linear |

## 10. Decisions (2026-09-17)

All four open decisions resolved by the investigation owner. Recorded here as the canonical
state; implementation follows these unless new evidence overrides.

1. **Default `interpolation_method`: `hermite`.** Cubic Hermite using stored order-1
   derivatives is the default method when the gate is on and the key is absent. Degrades
   gracefully (→ linear → memcpy) per §4/§6.
2. **la2 cross-SCC re-stamp: dedicated `AccessMode::Interp`.** New mode that resolves a
   reference time (same frontier-clamped recipe as `resolve_time`) without reusing
   `StartTime` or touching the `Latest` contract. Stamp is chosen at resolver construction:
   gate off → `Latest` (unchanged), gate on → `Interp`. See §5.
3. **IMP-046: keep whole.** Do not split the backlog item. The generic B/C design is recorded
   as subsuming IMP-046's *interpolation* half; the *warm-start* half stays la2-specific. A
   subsumption note goes in `IMP-046.md` when the implementation lands (not before — the
   "subsumes" claim is design-level until proven by code).
4. **Derivative slope check: do it (per recommendation).** Before trusting Hermite's bound
   derivative, verify the FMU-reported output derivative
   (`get_real_output_derivative`, model_connector.cpp:197) against a forward-difference of the
   recorded trajectory on a smooth segment, using `record_derivatives` `.d1` columns. This is
   the empirical prerequisite for the Hermite default; it is queued as the next investigation
   task (task.md).