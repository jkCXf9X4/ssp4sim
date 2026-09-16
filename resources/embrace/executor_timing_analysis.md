# Executor Timing Analysis: Jacobi vs Hybrid (la2)

**Date:** 2026-09-16
**SSP:** `embrace_scen.ssp`, 0–2000 s, timestep 0.01 s, 6 models (scenario, Atmos, Consumer, ECS_HW, ECS_SW, AdaptionUnit)

Follow-up to [`executor_comparison.md`](executor_comparison.md), which documents *what* the two
executors produce. This document records *why* the hybrid is slower and verifies that the
slowdown is not caused by the forward-derivative path.

## Question 1: Should models run 4× slower at 1/4 timestep?

Control: pure jacobi at `timestep = 0.0025` (4× the steps, same step count as the hybrid's
sub-steps). Full runs (`wd/embrace_jacobi_fine/`):

| Run | Macro steps | Model walltime | Total walltime |
|---|---|---|---|
| jacobi dt=0.01 | 200k | 6.041 s | 6.997 s |
| jacobi dt=0.0025 | 800k | 19.950 s | 25.545 s |
| la2 hybrid (4× sub-step) | 200k / 800k sub | 28.560 s | 33.623 s |

**Finding: model time does NOT scale linearly with step count.** 4× the steps gives 3.30×
model time and 3.65× total, not 4×. FMU work per call is not constant (per-call overhead
does not scale with the smaller step).

## Question 2: Is the hybrid slower than plain jacobi at the same step count?

Yes, and by more than the step count explains:

| | jacobi dt=0.0025 | la2 hybrid | ratio |
|---|---|---|---|
| Model time | 19.95 s | 28.56 s | 1.43× |
| Total | 25.55 s | 33.62 s | 1.32× |

The hybrid does **fewer** total FMU calls than fine jacobi (loop SCC models 800k each, non-loop
200k → ~3.6M vs 4.8M), yet spends more model time. Per-call wall-clock cost for the SCC members
is inflated ~1.3–1.7× (ECS_HW 15.6 → 24.1 µs/call, Consumer 5.6 → 9.5 µs/call,
AdaptionUnit 1.1 → 1.4 µs/call).

## Question 3: Is the extra time caused by derivatives?

**No.** Controlled benchmark (cores pinned, 100 s window, 4 interleaved reps, tight variance):

| Config | derivatives ON | derivatives OFF |
|---|---|---|
| jacobi dt=0.01 | 0.33 s | 4.15 s (12.6× slower) |
| jacobi dt=0.0025 | 0.97 s | 2.63 s (2.7× slower) |
| la2 hybrid | 2.54 s | 3.19 s (1.26× slower) |

Removing `forward_derivatives` makes every config **slower**, not faster. Forward derivatives
are a speed-up mechanism: they let `canInterpolateInputs` FMUs interpolate inputs instead of
grinding internally (see the note in `resolver_common.cpp`). They cannot explain the hybrid's
slowdown.

## Question 4: Are the hybrid's derivatives anchored to the correct timestep?

Verified via the new `simulation.recording.record_derivatives` output (`<signal>.d1` columns in
CSV/SQLite). Comparing the recorded derivative against the forward-difference slope of each
executor's own trajectory (transient t=83–90, `ECS_HW.LHexTout`):

- In both executors `d1` matches the executor's own slope at every step/sub-step.
- Loop-SCC member derivatives are identical between executors at macro times where trajectories
  agree (`Consumer.inletTemp.d1`, `AdaptionUnit…TLiquid.d1`: max diff 0).
- The large `d1` differences (`consumerRet.p.d1`, `LHexTout.d1`) are trajectory divergence (the
  values differ, so their slopes differ) — not a timestep-misaligned derivative.

No timestep mismatch found.

## Root cause of the hybrid's per-call inflation

The sub-step executor machinery itself: `LinearSubstepExecutor` sweeps the SCC group in parallel
per sub-step (`invoke_group_parallel`), so every macro step has 4 parallel regions with barrier
sync plus resolver sampling at sub-step boundaries. This shows up as wall-clock inflation inside
the model timers (`FmuModel::walltime_ns`), not as extra FMU work.

## Artifacts

| Artifact | Location |
|---|---|
| jacobi, derivatives ON | `wd/embrace_der/result.csv` |
| hybrid, derivatives ON | `wd/embrace_la2_der/result.csv` |
| jacobi dt=0.0025 | `wd/embrace_jacobi_fine/result.csv` |
| benchmark configs (short window, csv off) | `/tmp/opencode/bench_*.json` |

## Measurement caveat

Host was under load (load average ~9 on 16 cores) during full runs; full-run walltimes quoted
above are single-shot and noisy. The derivative ON/OFF comparison was repeated 4× pinned to
cores 0–5 with tight variance and is trustworthy.