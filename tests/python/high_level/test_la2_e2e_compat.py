from __future__ import annotations

import csv
import json
import math
import shutil
from pathlib import Path
from typing import Any

import pytest

from ._helpers import (
    GENERIC_CONFIG_PATH,
    REFERENCE_SSP_ROOT,
    parse_float,
    pyssp4sim,
)

# ============================================================================
# P7 — end-to-end config-compatibility coverage for the la2/loop-aware refactor.
#
# These tests target the PATCHED (post-refactor) behavior, which arrives via a
# set of concurrent patches:
#   (1) ExecutorBuilder accepts BOTH executor methods: "la2" (new primary name)
#       and "loop_aware" (legacy alias) — see executor_builder.cpp dispatch.
#   (2) Config keys: `simulation.executor.la2.*` is primary;
#       `simulation.executor.loop_aware.*` is honored as a legacy fallback —
#       see la2_scheduler.cpp `getConfigOrLegacy()`.
#   (3) Mode values: "linear" | "fixed"  -> fixed/equal sub-steps;
#                         "factor" | "geometric" -> shrinking (free-shrink
#                         factor) sub-steps.
# The pre-existing test_loop_aware_nested.py already covers the legacy names
# end-to-end; it is left untouched (it will pass once the patches land). This
# file adds the *explicit* legacy-vs-primary precedence and alias assertions.
#
# The fixture math below is copied from test_loop_aware_nested.py (same
# analytic fixed points for the algebraic-loop reference fixtures).
# ============================================================================

# Analytic fixed point for signal_nested_algebraic_loop.
# Sine: offset + amplitude * sin(2*pi*f*t) = 1 + 0.5*sin(4*pi*t)
#   add_outer.y  = 2 * sine.y
#   add_inner.y  = sine.y
#   gain_outer.y = 0.5 * sine.y
#   gain_inner.y = 0.5 * sine.y
NESTED_SIGNALS = {
    "add_outer.y": 2.0,
    "add_inner.y": 1.0,
    "gain_outer.y": 0.5,
    "gain_inner.y": 0.5,
}

# Single loop fixed point: add.y = 2 * sine.y, gain.y = sine.y
SINGLE_SIGNALS = {
    "add.y": 2.0,
    "gain.y": 1.0,
}

# Fixed-point tolerance used by the original nested-loop regression test.
STEADY_STATE_TOLERANCE = 0.05


def write_la2_compat_config(
    ssp_root: Path,
    workdir: Path,
    *,
    method: str = "loop_aware",
    mode: str = "fixed",
    iterations: int = 32,
    factor: float = 0.8,
    use_legacy_keys: bool = True,
    extra_executor: dict[str, Any] | None = None,
) -> Path:
    """Build a loop-aware configuration.

    `use_legacy_keys=True` writes the keys under
    `simulation.executor.loop_aware.*` (legacy namespace); `False` writes them
    under `simulation.executor.la2.*` (primary namespace). `method` selects the
    executor method string ("loop_aware" legacy alias or "la2" primary name).

    `extra_executor` is merged into the executor section afterwards, which lets
    a test set BOTH namespaces simultaneously (precedence probe).
    """
    config: dict[str, Any] = json.loads(GENERIC_CONFIG_PATH.read_text())
    simulation = config["simulation"]

    simulation["ssp"] = str(ssp_root)
    simulation["ssd"] = "SystemStructure.ssd"
    simulation["start_time"] = 0.0
    simulation["stop_time"] = 0.25
    simulation["timestep"] = 0.001
    simulation["tolerance"] = 1e-4
    simulation["realtime"] = False
    simulation["working_dir"] = str(workdir)

    executor = simulation["executor"]
    executor["method"] = method
    executor["thread_pool_workers"] = 5
    executor["forward_derivatives"] = True
    if use_legacy_keys:
        executor["loop_aware"] = {
            "mode": mode,
            "iterations": iterations,
            "factor": factor,
        }
    else:
        executor["la2"] = {
            "mode": mode,
            "iterations": iterations,
            "factor": factor,
        }
    if extra_executor:
        executor.update(extra_executor)
    executor["jacobi"] = {"parallel": True, "method": 1}
    executor["seidel"] = {"parallel": False}

    recording = simulation["recording"]
    recording.pop("enable", None)
    recording.pop("result_file", None)
    recording["csv"] = {
        "enable": True,
        "file": str(workdir / "result.csv"),
        "interval": 0.0,
    }
    recording["wait_for"] = True

    log_config = simulation["log"]
    log_config["fmu"] = False
    log_config["level_terminal"] = "error"
    log_config["level_file"] = "error"
    log_config["level_json"] = "error"
    log_config["level_cutelog"] = "disable"

    workdir.mkdir(parents=True, exist_ok=True)
    config_path = workdir / "la2_compat_config.json"
    config_path.write_text(json.dumps(config, indent=2))
    return config_path


def steady_state_max_error(
    result_file: Path,
    macro_dt: float,
    signals: dict[str, float],
    skip_macros: int = 1,
) -> float:
    """Max |value - gain*sine(t)| over the last recorded sample of each macro
    step after the initial `skip_macros` macro steps.

    The loop-aware executor sub-steps the macro step, so only the final sample
    of each macro step is compared against the analytic fixed point. Same
    semantics as steady_state_max_error in test_loop_aware_nested.py; the first
    macro step is excluded because the feed-in source writes its output at
    macro end, leaving the loop to read a stale input for one step.
    """
    with result_file.open(newline="") as input_file:
        rows = list(csv.DictReader(input_file))

    assert rows, f"Result file is empty: {result_file}"

    if signals is SINGLE_SIGNALS:
        sine_amp, sine_offset = 1.0, 0.0
    else:
        sine_amp, sine_offset = 0.5, 1.0
    sine_freq = 2.0

    max_err = 0.0
    macro_bucket: int = -1
    last_sample: dict[str, str] = {}
    for row in rows:
        time = parse_float(row["time"])
        assert time is not None and math.isfinite(time)
        macro = int((time + 1e-9) // macro_dt)
        if macro <= skip_macros:
            continue
        if macro != macro_bucket:
            macro_bucket = macro
        last_sample = row  # keep last sample of this macro step

        sine_y = sine_offset + sine_amp * math.sin(2.0 * math.pi * sine_freq * time)
        for signal, gain in signals.items():
            value = parse_float(last_sample[signal])
            assert value is not None and math.isfinite(value), (
                f"Non-finite {signal} at t={time}"
            )
            max_err = max(max_err, abs(value - gain * sine_y))

    return max_err


def _prepare_fixture(tmp_path: Path, fixture: str) -> tuple[Path, Path] | None:
    """Copy a reference fixture into a temp ssp root and return (root, workdir).

    Returns None when the fixture is not unpacked (caller skips), mirroring the
    skip idiom of test_loop_aware_nested.py.
    """
    fixture_root = REFERENCE_SSP_ROOT / fixture / "baseline"
    if not fixture_root.exists():
        return None
    workdir = tmp_path / fixture
    runtime_ssp_root = workdir / "ssp"
    workdir.mkdir(parents=True, exist_ok=True)
    shutil.copytree(fixture_root, runtime_ssp_root)
    return runtime_ssp_root, workdir


def _run_and_check(config_path: Path, result_file: Path, signals: dict[str, float]) -> float:
    simulator = pyssp4sim.Simulator(str(config_path))
    simulator.init()
    simulator.simulate()
    return steady_state_max_error(result_file, 0.001, signals)


# ---------------------------------------------------------------------------
# (i) + (ii) — fully LEGACY configuration end-to-end.
# method="loop_aware" (legacy alias) + simulation.executor.loop_aware.* keys.
# Passes only after the concurrent patches: executor_builder must accept the
# alias and la2_scheduler must fall back to the legacy key namespace.
# ---------------------------------------------------------------------------
@pytest.mark.parametrize("mode", ["fixed", "geometric"])
@pytest.mark.parametrize(
    "fixture,signals",
    [
        ("signal_nested_algebraic_loop", NESTED_SIGNALS),
        ("signal_algebraic_loop", SINGLE_SIGNALS),
    ],
)
def test_legacy_names_end_to_end_converges(
    fixture: str,
    signals: dict[str, float],
    mode: str,
    tmp_path: Path,
) -> None:
    """P7: a loop-aware simulation configured ENTIRELY with legacy names
    ("loop_aware" method + `simulation.executor.loop_aware.iterations/mode/
    factor`) must run and reach the steady-state fixed point.

    Mirrors test_loop_aware_nested.py (same fixtures, same 32-sub-step
    relaxation, same tolerance) but is an explicit compat anchor for the
    patched alias + legacy-key fallback. Both legacy mode spellings are
    exercised: "fixed" (legacy alias of "linear" -> equal sub-steps) and
    "geometric" (legacy alias of "factor" -> shrinking sub-steps).
    """
    prepared = _prepare_fixture(tmp_path, fixture)
    if prepared is None:
        pytest.skip(f"Missing fixture: {REFERENCE_SSP_ROOT / fixture / 'baseline'}")
    runtime_ssp_root, workdir = prepared

    config_path = write_la2_compat_config(
        runtime_ssp_root,
        workdir,
        method="loop_aware",
        mode=mode,
        iterations=32,
        factor=0.8,  # legacy default shrink factor
        use_legacy_keys=True,
    )
    max_err = _run_and_check(config_path, workdir / "result.csv", signals)
    assert max_err <= STEADY_STATE_TOLERANCE, (
        f"legacy-name loop-aware ({fixture}, {mode}) fixed-point error "
        f"{max_err:.5f} exceeds {STEADY_STATE_TOLERANCE}"
    )


# ---------------------------------------------------------------------------
# (iv) — method alias: executor_builder must accept BOTH method strings.
# ---------------------------------------------------------------------------
@pytest.mark.parametrize("method", ["loop_aware", "la2"])
def test_executor_method_alias_accepted(method: str, tmp_path: Path) -> None:
    """P7: `Simulator.init()` + `simulate()` succeed for method "loop_aware"
    (legacy alias) AND for the primary "la2" name — i.e. ExecutorBuilder does
    not throw "Unknown executor method" for either spelling.

    Uses la2.* keys (primary namespace) so the alias is isolated from the
    legacy key-fallback path.
    """
    prepared = _prepare_fixture(tmp_path, "signal_algebraic_loop")
    if prepared is None:
        pytest.skip("Missing fixture: signal_algebraic_loop/baseline")
    runtime_ssp_root, workdir = prepared

    config_path = write_la2_compat_config(
        runtime_ssp_root,
        workdir,
        method=method,
        mode="fixed",
        iterations=16,
        factor=0.5,
        use_legacy_keys=False,
    )

    simulator = pyssp4sim.Simulator(str(config_path))
    simulator.init()  # would raise "Unknown executor method" if the alias is missing
    simulator.simulate()

    result_file = workdir / "result.csv"
    assert result_file.exists(), f"Missing result file: {result_file}"
    with result_file.open(newline="") as input_file:
        rows = list(csv.DictReader(input_file))
    assert rows, "Result file does not contain any samples"
    assert "time" in rows[0], "Result file does not contain a time column"


# ---------------------------------------------------------------------------
# (iii) — config precedence: with BOTH namespaces set, la2.* must win.
# ---------------------------------------------------------------------------
def test_la2_keys_take_precedence_over_legacy(tmp_path: Path) -> None:
    """P7: when `simulation.executor.la2.*` AND the legacy
    `simulation.executor.loop_aware.*` keys are both present, the la2.* values
    must win (la2_scheduler reads la2.* first, falls back to loop_aware.* only
    when the la2 key is absent).

    Observable: with la2.iterations=32 the loop converges to the analytic fixed
    point (<= 0.05); with only iterations=1 the loop under-relaxes and the
    error is ~0.36 (see FIXTURE.md / test_loop_aware_nested.py rationale). The
    legacy namespace here is deliberately set to iterations=1, so a correct
    implementation (la2 wins) converges and a buggy one (legacy wins, or a
    merge that prefers legacy) fails loudly.
    """
    prepared = _prepare_fixture(tmp_path, "signal_nested_algebraic_loop")
    if prepared is None:
        pytest.skip("Missing fixture: signal_nested_algebraic_loop/baseline")
    runtime_ssp_root, workdir = prepared

    # Legacy namespace: a deliberately under-converged configuration.
    legacy = {"mode": "geometric", "iterations": 1, "factor": 0.8}
    # Primary namespace: the converging configuration.
    primary = {"mode": "fixed", "iterations": 32, "factor": 0.5}
    # base config written with la2.* keys (use_legacy_keys=False), then the
    # legacy namespace is merged in so BOTH are present.
    config_path = write_la2_compat_config(
        runtime_ssp_root,
        workdir,
        method="la2",
        mode=primary["mode"],
        iterations=primary["iterations"],
        factor=primary["factor"],
        use_legacy_keys=False,
        extra_executor={"loop_aware": legacy},
    )

    max_err = _run_and_check(config_path, workdir / "result.csv", NESTED_SIGNALS)
    assert max_err <= STEADY_STATE_TOLERANCE, (
        f"la2.* precedence: fixed-point error {max_err:.5f} exceeds "
        f"{STEADY_STATE_TOLERANCE} (legacy iterations=1 would give ~0.36 — "
        f"the la2.iterations=32 value must have won)"
    )


# ---------------------------------------------------------------------------
# Companion — primary names end to end (la2 method + la2.* keys).
# ---------------------------------------------------------------------------
@pytest.mark.parametrize("mode", ["fixed", "geometric"])
def test_la2_primary_names_end_to_end_converges(mode: str, tmp_path: Path) -> None:
    """P7: the primary config path (method "la2" + `simulation.executor.la2.*`
    keys, factor default 0.8) also reaches the fixed point. Guards against the
    refactor regressing the primary namespace while the legacy fallback works.
    """
    prepared = _prepare_fixture(tmp_path, "signal_nested_algebraic_loop")
    if prepared is None:
        pytest.skip("Missing fixture: signal_nested_algebraic_loop/baseline")
    runtime_ssp_root, workdir = prepared

    config_path = write_la2_compat_config(
        runtime_ssp_root,
        workdir,
        method="la2",
        mode=mode,
        iterations=32,
        factor=0.8,  # primary default shrink factor (aligned with legacy 0.8)
        use_legacy_keys=False,
    )
    max_err = _run_and_check(config_path, workdir / "result.csv", NESTED_SIGNALS)
    assert max_err <= STEADY_STATE_TOLERANCE, (
        f"la2 primary ({mode}) fixed-point error {max_err:.5f} exceeds "
        f"{STEADY_STATE_TOLERANCE}"
    )