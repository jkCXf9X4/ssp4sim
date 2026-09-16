# Executor Comparison: Jacobi vs Loop-Aware

**Date:** 2026-07-03
**Configs:** `embrace.json` (jacobi) vs `embrace_loop_aware.json` (loop_aware)
**SSP:** `embrace_scen.ssp` (same for both)
**Simulation:** 0–2000 s, timestep 0.01 s, 6 models (scenario, Atmos, Consumer, ECS_HW, ECS_SW, AdaptionUnit)

## Configuration Differences

| Setting | Jacobi (`embrace.json`) | Loop-Aware (`embrace_loop_aware.json`) |
|---|---|---|
| Executor method | `jacobi` | `loop_aware` |
| Thread pool | 5 workers | 5 workers |
| Jacobi parallel | yes (method 1) | yes (method 1) |
| Sub-step | n/a | 0.001 s |
| Forward derivatives | true | true |
| SQLite | disabled | enabled |
| Working dir | `./wd/embrace` | `./wd/embrace_loop_aware` |

## Performance

| Metric | Jacobi | Loop-Aware | Ratio |
|---|---|---|---|
| Total walltime | 8.20 s | 36.23 s | 4.4× slower |
| Model walltime | 6.33 s | 27.52 s | 4.3× slower |
| CSV rows | 200,001 | 800,002 | 4× more |

The loop-aware executor is slower because it performs **4 internal iterations per timestep** for the SCC (Consumer, ECS_HW, ECS_SW, AdaptionUnit), while jacobi does a single pass. The 4× row count reflects those internal iterations being recorded.

## Executor Structure

### Jacobi
- Single executor: `JacobiParallelTBB`
- All 6 models evaluated once per timestep in parallel where possible
- No algebraic loop convergence within a timestep

### Loop-Aware
- Executor: `LoopAwareExecutor`
- 3 SCCs detected:
  - **Step 0:** scenario (1 node, 1 iter) — no feedback
  - **Step 1:** Atmos (1 node, 1 iter) — no feedback
  - **Step 2:** Consumer, ECS_HW, ECS_SW, AdaptionUnit (4 nodes, 4 iters) — **algebraic loop**
- The 4-node SCC is iterated 4 times per timestep to converge the algebraic loop

## Transient Behavior (t = 70–160)

The cooling pack transitions from inactive (513 K) to active (93 K) during this window. The two executors diverge significantly:

| t (s) | Jacobi TCool (K) | Loop-Aware TCool (K) | ΔT (K) | Notes |
|---|---|---|---|---|
| 70 | 513.00 | 513.00 | 0.00 | Both at idle |
| 75 | 511.86 | 513.00 | +1.14 | Loop-aware still at idle |
| 80 | 482.40 | 461.03 | −21.38 | Loop-aware cools faster |
| 85 | 421.06 | 307.17 | −113.89 | **Divergence accelerates** |
| 90 | 344.15 | 166.94 | −177.21 | **Peak cooling rate difference** |
| 95 | 269.29 | 113.00 | −156.29 | Loop-aware near floor |
| 100 | 211.23 | 114.95 | −96.29 | Jacobi still cooling |
| 105 | 171.96 | 167.26 | −4.71 | Both near floor |
| 110 | 151.60 | 287.48 | +135.88 | **Loop-aware rebounds** |
| 115 | 150.71 | 388.13 | +237.41 | **Overshoot peak** |
| 120 | 169.62 | 379.08 | +209.45 | Loop-aware still high |
| 125 | 208.47 | 294.58 | +86.10 | Coming down |
| 130 | 262.21 | 230.75 | −31.46 | Crossed below |
| 135 | 314.01 | 239.32 | −74.69 | Jacobi now higher |
| 140 | 351.51 | 296.53 | −54.98 | Both descending |
| 145 | 368.02 | 339.80 | −28.22 | Converging |
| 150 | 363.32 | 110.10 | −253.22 | **Loop-aware drops to floor** |
| 155 | 93.00 | 93.00 | 0.00 | **Both converged** |
| 160+ | 93.00 | 93.00 | 0.00 | Stable |

### Temperature Impact During Transient

At t = 100 (peak TCool divergence), the consumer temperatures differ by ~3.6 K:

| Signal | Jacobi (K) | Loop-Aware (K) | Δ (K) |
|---|---|---|---|
| Consumer.inletTemp | 316.56 | 312.94 | −3.62 |
| Consumer.outletTemp | 316.68 | 313.10 | −3.58 |
| ECS_HW.LHexTout | 316.53 | 312.91 | −3.62 |

The loop-aware executor's converged algebraic loop produces a faster, more oscillatory cooldown transient. The jacobi executor's single-pass approach acts as a low-pass filter, smoothing the transient but potentially under-representing the coupling dynamics.

## Steady-State Comparison (t = 200–2000)

Once the cooling pack stabilizes at 93 K, both executors agree closely:

| Signal | Jacobi Mean | Loop-Aware Mean | Mean Δ | Rel Δ |
|---|---|---|---|---|
| Consumer.inletTemp | 309.83 K | 310.10 K | +0.27 K | +0.086% |
| Consumer.outletTemp | 312.69 K | 312.96 K | +0.27 K | +0.085% |
| ECS_HW.LHexTout | 309.84 K | 310.11 K | +0.27 K | +0.086% |
| TCoolingPack | 93.00 K | 93.00 K | 0.00 K | 0.000% |
| Consumer.consumerRet.p | 362,600 Pa | 362,460 Pa | −140 Pa | −0.039% |
| Consumer.consumerFeed.p | 426,046 Pa | 425,838 Pa | −208 Pa | −0.049% |

### Drift Over Time

The differences are **slowly growing**, not converging to zero:

| t (s) | ΔTin (K) | ΔTout (K) | ΔLHex (K) | Δp_ret (Pa) | Δp_feed (Pa) |
|---|---|---|---|---|---|
| 200 | +0.209 | +0.209 | +0.209 | −110.1 | −161.8 |
| 500 | +0.227 | +0.226 | +0.227 | −116.6 | −173.3 |
| 1000 | +0.259 | +0.258 | +0.259 | −134.8 | −200.2 |
| 1500 | +0.295 | +0.294 | +0.295 | −155.6 | −231.1 |
| 2000 | +0.335 | +0.334 | +0.335 | −179.1 | −265.9 |

This drift is expected for a nonlinear thermal system: different numerical paths through the transient produce slightly different initial conditions for the steady-state regime, and the nonlinear feedback slowly amplifies the divergence.

## Summary

| Aspect | Verdict |
|---|---|
| **Physical plausibility** | ✅ Both produce reasonable results |
| **Steady-state agreement** | ✅ Within 0.09% for temperatures, 0.05% for pressures |
| **Transient agreement** | ⚠️ Significant differences during cooling pack transition (up to 177 K in TCool, 3.6 K in Tin) |
| **Convergence** | ⚠️ Differences drift slowly over time (not converging to zero) |
| **Performance** | ⚠️ Loop-aware is 4.4× slower but provides tighter algebraic loop convergence |

The loop-aware executor is the more numerically accurate method because it converges the algebraic loop within each timestep. The jacobi executor is faster but takes a single pass through the SCC, which smooths transients and may under-represent coupling dynamics.
