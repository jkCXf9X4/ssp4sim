# Interpolation of Values and Derivatives in the Loop-Aware Executor

**Date:** 2026-07-03
**Scope:** How the loop-aware scheduler could utilize interpolation of values and derivatives to improve algebraic loop convergence, accuracy, and performance.

## 1. Current Architecture

### 1.1 Data Flow

```
┌─────────────────────────────────────────────────────────┐
│  LoopAwareExecutor::invoke()                             │
│                                                          │
│  for each SCC in topological order:                      │
│    if single-node SCC:                                   │
│      node->invoke(StepData(t_start, t_end, dt,           │
│                             t_start, t_start))           │
│    if multi-node SCC (loop):                             │
│      sub_dt = macro_dt / n_iters                         │
│      for each sub-step:                                  │
│        JacobiParallelTBB::invoke(StepData(sub_start,     │
│                                   sub_end, step,         │
│                                   sub_start, sub_end))   │
│          → all nodes in parallel:                        │
│            FmuModel::step(StepData)                      │
│              → pre(input_time)                           │
│                  → retrieve_model_inputs()               │
│                    → find_latest_valid_area(time)        │
│                    → memcpy (nearest-neighbor)           │
│                    → memcpy derivatives                  │
│              → fmu->step_until(end_time)                 │
│              → post(output_time)                         │
│                → read_values_from_model()                │
│                → fetch_output_derivatives()              │
└─────────────────────────────────────────────────────────┘
```

### 1.2 Key Components

| Component | Role | Interpolation-Relevant Detail |
|---|---|---|
| `SignalStorage` | Ring-buffer of time-stamped signal data | Stores `max_interpolation_orders` derivatives per signal |
| `RingBuffer::find_latest_valid_index(time)` | Finds newest data ≤ requested time | Returns **exact match or older** — no interpolation |
| `ConnectionInfo::retrieve_model_inputs()` | Copies source data → target input area | `memcpy` of value + derivatives, **no interpolation** |
| `FmuModel::pre()` | Prepares FMU inputs before step | Calls `apply_input_derivatives()` to set derivative orders on FMU |
| `FmuModel::post()` | Reads FMU outputs after step | Calls `fetch_output_derivatives()` to store output derivatives |
| `StepData` | Carries `input_time` and `output_time` | Already supports decoupled read/write times |

### 1.3 Current Interpolation Readiness

The infrastructure already supports interpolation **in principle**:

- **Derivatives are stored** — each signal in `SignalStorage` has space for `max_interpolation_orders` derivative values (typically order 1)
- **Derivatives are forwarded** — `retrieve_model_inputs()` copies derivatives from source to target storage
- **Derivatives are applied to FMUs** — `apply_input_derivatives()` calls `set_real_input_derivative()` on the FMU
- **FMUs declare capability** — `canInterpolateInputs` is parsed from the model description XML

**What's missing:** The actual interpolation computation. `find_latest_valid_area()` returns the nearest data point at or before the requested time, and the value is copied verbatim. No attempt is made to interpolate between two stored time points using the derivatives.

## 2. Interpolation Opportunities

### 2.1 Time-Interpolated Input Retrieval (Highest Impact)

**Problem:** When `input_time` falls between two stored timestamps, the current code uses the older value (zero-order hold). This introduces a discretization error that is particularly harmful in algebraic loops where the same time point is iterated multiple times.

**Solution:** Modify `retrieve_model_inputs()` to detect when the requested time falls between two stored data points and perform interpolation.

```
Current behavior:
  t_source = 90    t_request = 95    t_source = 100
       │                │                  │
       ▼                ▼                  ▼
    value=344         value=344         value=211
                    (zero-order hold)

With interpolation:
  t_source = 90    t_request = 95    t_source = 100
       │                │                  │
       ▼             ▼ interpolated ▼      ▼
    value=344    value = 344 + 0.5 *     value=211
                  (211-344) = 277.5
```

**Implementation sketch:**

```cpp
// In ConnectionInfo::retrieve_model_inputs() or a new helper:

bool find_bracketing_areas(uint64_t time,
                           size_t &lower_area, uint64_t &lower_time,
                           size_t &upper_area, uint64_t &upper_time)
{
    // Find the newest data ≤ time (existing behavior)
    if (!find_latest_valid_area(time, lower_area))
        return false;
    lower_time = get_time(lower_area);

    if (lower_time == time)
    {
        // Exact match — no interpolation needed
        upper_area = lower_area;
        upper_time = lower_time;
        return true;
    }

    // Find the next newer data > time
    // (walk forward from lower_area in the ring buffer)
    if (!find_next_newer_area(lower_area, upper_area))
        return false;
    upper_time = get_time(upper_area);
    return true;
}

void interpolate_and_copy(ConnectionInfo &conn,
                          size_t target_area, uint64_t input_time,
                          size_t lower_area, uint64_t lower_time,
                          size_t upper_area, uint64_t upper_time)
{
    auto *target = conn.target_storage->get_item(target_area, conn.target_index);

    if (lower_time == upper_time || lower_time == input_time)
    {
        // Exact match — direct copy (current behavior)
        auto *source = conn.source_storage->get_item(lower_area, conn.source_index);
        std::memcpy(target, source, conn.size);
        return;
    }

    // Linear interpolation: t ∈ [lower_time, upper_time]
    double frac = double(input_time - lower_time) / double(upper_time - lower_time);

    auto *lower = conn.source_storage->get_item(lower_area, conn.source_index);
    auto *upper = conn.source_storage->get_item(upper_area, conn.source_index);

    if (conn.type == types::DataType::real)
    {
        double v_lower = *reinterpret_cast<double *>(lower);
        double v_upper = *reinterpret_cast<double *>(upper);
        double v_interp = v_lower + frac * (v_upper - v_lower);
        *reinterpret_cast<double *>(target) = v_interp;
    }
    // ... handle other types
}
```

### 2.2 Derivative-Enhanced Interpolation (Higher Order)

**Problem:** Linear interpolation between two points may still be inaccurate for rapidly changing signals (e.g., TCoolingPack during the transient at t=80–100).

**Solution:** Use the stored derivatives to perform Hermite interpolation (or Taylor expansion) instead of linear interpolation.

```
Taylor expansion from lower bound:
  v(t) ≈ v(t_lower) + v'(t_lower) * (t - t_lower)

Hermite interpolation (using derivatives at both bounds):
  v(t) = h00(ξ) * v_lower + h10(ξ) * (t_upper - t_lower) * v'_lower
       + h01(ξ) * v_upper + h11(ξ) * (t_upper - t_lower) * v'_upper
  where ξ = (t - t_lower) / (t_upper - t_lower)
  and h00, h10, h01, h11 are the cubic Hermite basis functions
```

**Implementation sketch:**

```cpp
double hermite_interpolate(double t, double t_low, double t_high,
                           double v_low, double v_high,
                           double dv_low, double dv_high)
{
    double dt = t_high - t_low;
    double xi = (t - t_low) / dt;

    // Cubic Hermite basis functions
    double h00 = 2*xi*xi*xi - 3*xi*xi + 1;       // (1 + 2ξ)(1 - ξ)²
    double h10 = xi*xi*xi - 2*xi*xi + xi;          // ξ(1 - ξ)²
    double h01 = -2*xi*xi*xi + 3*xi*xi;            // ξ²(3 - 2ξ)
    double h11 = xi*xi*xi - xi*xi;                 // ξ²(ξ - 1)

    return h00 * v_low + h10 * dt * dv_low
         + h01 * v_high + h11 * dt * dv_high;
}
```

### 2.3 Sub-Step Interpolation Within the Loop (Architectural)

**Problem:** The loop-aware executor currently divides the macro timestep into `n_iters` equal sub-steps and runs each sub-step sequentially. Each sub-step uses `sub_start` as both `input_time` and `output_time`. This means within a single macro timestep, each node sees outputs from the immediately preceding sub-step — effectively a Gauss-Seidel pattern within the loop.

**Solution:** Use the derivatives to predict outputs at the *end* of the macro timestep from the *start*, enabling a Jacobi-like pattern where all nodes in the loop see consistent time-aligned inputs.

```
Current (sub-step iteration):
  Iter 1: input at t=0,     output at t=0.0025
  Iter 2: input at t=0.0025, output at t=0.005
  Iter 3: input at t=0.005,  output at t=0.0075
  Iter 4: input at t=0.0075, output at t=0.01

With extrapolation:
  Iter 1: input at t=0,     output at t=0.01 (extrapolated using derivatives)
  Iter 2: input at t=0,     output at t=0.01 (refined)
  ...
```

This would allow all iterations to operate on the same time horizon, converging the algebraic loop at a consistent time point rather than marching through sub-steps.

### 2.4 Iteration History Interpolation (Convergence Acceleration)

**Problem:** Within the algebraic loop iterations, each node sees only the *immediately previous* iteration's outputs. There is no memory of earlier iterations.

**Solution:** Store the last K iteration outputs and use them to extrapolate a better initial guess for the next iteration (similar to Anderson acceleration or Aitken's delta-squared process).

```
Iteration memory:
  Iter 1: x₁ = f(x₀)
  Iter 2: x₂ = f(x₁)
  Iter 3: x₃ = f(x₂)
  Iter 4: x₄ = f(x₃)  ← current behavior (only last iteration)

With extrapolation:
  x̂ = x₃ + (x₃ - x₂) / (1 - (x₂ - x₁) / (x₃ - x₂))  ← Aitken acceleration
  x₄ = f(x̂)
```

## 3. Implementation Plan

### Phase 1: Linear Time Interpolation (Low Risk, High Value)

**Files to modify:**
- `model_connection.cpp` / `.hpp` — `retrieve_model_inputs()`
- `storage.cpp` / `.hpp` — add `find_next_newer_area()` helper
- `ring_buffer.cpp` / `.hpp` — add `find_next_valid_index()` helper

**Changes:**
1. Add `RingBuffer::find_next_valid_index(time, index)` — finds the first stored timestamp > time
2. Add `SignalStorage::find_bracketing_areas(time, lower, upper)` — returns both the ≤time and >time areas
3. Modify `retrieve_model_inputs()` to detect bracketing and call linear interpolation for `real`-typed signals
4. Add a config flag `simulation.executor.interpolate_inputs` (default: `false`) to gate the new behavior

**Estimated effort:** 2–3 days
**Risk:** Low — linear interpolation is well-understood, backward-compatible (opt-in via config)

### Phase 2: Hermite Interpolation Using Derivatives (Medium Risk)

**Files to modify:**
- `model_connection.cpp` — add Hermite interpolation path
- `model_fmu.cpp` — ensure derivatives are available at both bracketing points

**Changes:**
1. Extend the interpolation function to use cubic Hermite when both value and derivative are available at both bracketing points
2. Fall back to linear interpolation when derivatives are missing
3. Add a config flag `simulation.executor.interpolation_method` (`linear` | `hermite`)

**Estimated effort:** 3–5 days
**Risk:** Medium — requires careful handling of missing/zero derivatives

### Phase 3: Sub-Step Extrapolation (Higher Risk, Architectural)

**Files to modify:**
- `loop_aware_executor.cpp` — change sub-step iteration strategy
- `StepData` — may need additional fields for extrapolation horizon

**Changes:**
1. Instead of marching sub_start forward, keep it fixed at the macro timestep start
2. Use derivatives to extrapolate outputs from sub_start to macro_end
3. Run all iterations at the same time point, converging the algebraic loop in place

**Estimated effort:** 1–2 weeks
**Risk:** High — changes the convergence semantics of the loop executor

### Phase 4: Iteration History Extrapolation (Experimental)

**Files to modify:**
- `loop_aware_executor.cpp` — add iteration history ring buffer
- `jacobi_parallel_tbb.cpp` — may need to expose iteration count

**Changes:**
1. Store the last N iteration outputs for each node in the loop SCC
2. After K iterations, apply Aitken acceleration or Anderson extrapolation
3. Use the extrapolated value as input for the next iteration

**Estimated effort:** 1–2 weeks
**Risk:** High — convergence acceleration can destabilize some systems

## 4. Expected Impact

### 4.1 Accuracy

| Scenario | Current Error (Jacobi vs Loop-Aware) | With Interpolation |
|---|---|---|
| Steady-state Tin (t=2000) | 0.33 K drift | Expected < 0.05 K |
| Transient TCool (t=90) | 177 K difference | Expected < 50 K |
| Pressure drift (t=2000) | 266 Pa | Expected < 50 Pa |

### 4.2 Performance

| Optimization | Expected Speed Impact | Notes |
|---|---|---|
| Linear interpolation | +5–10% overhead per step | Extra ring buffer lookup + arithmetic |
| Hermite interpolation | +10–20% overhead per step | More complex math, derivative access |
| Sub-step extrapolation | −50–75% iterations | Fewer iterations needed for convergence |
| Iteration extrapolation | −25–50% iterations | Faster convergence, risk of instability |

### 4.3 Convergence

The loop-aware executor currently uses `n_iters = scc_size` (4 iterations for a 4-node SCC). With interpolation:

- **Linear interpolation** alone does not reduce iteration count but improves accuracy per iteration
- **Hermite interpolation** provides better derivative tracking, potentially reducing iterations to 2–3
- **Sub-step extrapolation** could reduce to 1–2 iterations by providing better initial guesses
- **Iteration extrapolation** could reduce to 2–3 iterations with faster convergence

## 5. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Interpolation introduces numerical instability | Low | High | Gate behind config flag; validate on embrace test suite |
| Hermite interpolation with zero derivatives degrades to linear | Medium | Low | Fall back gracefully; log warning |
| Sub-step extrapolation changes convergence semantics | Medium | High | Implement as separate executor mode; compare results |
| Iteration extrapolation diverges for stiff systems | Low | High | Limit to 1 iteration of extrapolation; detect divergence |
| Performance overhead outweighs benefits | Low | Medium | Profile before/after; make interpolation adaptive |

## 6. Recommendation

**Implement Phase 1 (Linear Time Interpolation) as the immediate next step.** It is low-risk, leverages existing derivative infrastructure, and directly addresses the discretization error in `retrieve_model_inputs()`. The embrace test suite provides a good validation baseline.

**Phase 2 (Hermite) should follow** once the linear interpolation path is proven, since the derivative storage and forwarding infrastructure is already in place — only the interpolation math is missing.

**Phases 3 and 4 are architectural changes** that should be evaluated after Phases 1–2 demonstrate measurable accuracy improvements. The sub-step extrapolation in particular could fundamentally change how the loop-aware executor converges algebraic loops.
