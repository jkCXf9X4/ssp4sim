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

# Analytic fixed point for signal_nested_algebraic_loop.
# Sine: offset + amplitude * sin(2*pi*f*t) = 1 + 0.5*sin(4*pi*t)
# Fixed point (see FIXTURE.md):
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

# Single loop fixed point:
#   add.y = 2 * sine.y, gain.y = sine.y
SINGLE_SIGNALS = {
    "add.y": 2.0,
    "gain.y": 1.0,
}


def write_loop_aware_config(
    ssp_root: Path,
    workdir: Path,
    mode: str,
    iterations: int,
    factor: float = 0.8,
) -> Path:
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
    executor["method"] = "loop_aware"
    executor["thread_pool_workers"] = 5
    executor["forward_derivatives"] = True
    executor["loop_aware"] = {"mode": mode, "iterations": iterations, "factor": factor}
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
    config_path = workdir / "loop_aware_config.json"
    config_path.write_text(json.dumps(config, indent=2))
    return config_path


def steady_state_max_error(
    result_file: Path,
    macro_dt: float,
    signals: dict[str, float],
    skip_macros: int = 1,
) -> float:
    """Max |value - gain*sine(t)| over the last recorded sample of each macro step
    after the initial `skip_macros` macro steps.

    The loop-aware executor sub-steps the macro step, so only the final sample of
    each macro step is compared against the analytic fixed point. The first macro
    step is excluded because the feed-in source writes its output at macro end,
    leaving the loop to read a stale input for one step.
    """
    with result_file.open(newline="") as input_file:
        rows = list(csv.DictReader(input_file))

    assert rows, f"Result file is empty: {result_file}"

    sine_amp = 1.0 if signals is SINGLE_SIGNALS else 0.5
    sine_offset = 0.0 if signals is SINGLE_SIGNALS else 1.0
    sine_freq = 2.0

    max_err = 0.0
    macro_bucket: int = -1
    for row in rows:
        time = parse_float(row["time"])
        assert time is not None and math.isfinite(time)
        macro = int((time + 1e-9) // macro_dt)
        if macro <= skip_macros:
            continue
        if macro != macro_bucket:
            macro_bucket = macro
            last_sample = row
        else:
            last_sample = row  # keep last sample of this macro step

        sine_y = sine_offset + sine_amp * math.sin(2.0 * math.pi * sine_freq * time)
        for signal, gain in signals.items():
            value = parse_float(last_sample[signal])
            assert value is not None and math.isfinite(value), (
                f"Non-finite {signal} at t={time}"
            )
            max_err = max(max_err, abs(value - gain * sine_y))

    return max_err


@pytest.mark.parametrize("mode", ["fixed", "geometric"])
@pytest.mark.parametrize(
    "fixture,signals",
    [
        ("signal_nested_algebraic_loop", NESTED_SIGNALS),
        ("signal_algebraic_loop", SINGLE_SIGNALS),
    ],
)
def test_loop_aware_converges_algebraic_loop(
    fixture: str,
    signals: dict[str, float],
    mode: str,
    tmp_path: Path,
) -> None:
    """Description: Runs the loop-aware executor on the (nested) algebraic-loop
    reference fixtures with 32 internal sub-iterations per macro step and checks
    the steady-state analytic fixed point is reached within 0.05.

    Rationale: Regression anchor for nested loop handling. With the default
    iteration count (SCC node count) the nested loop under-converges
    (error ~0.36); only when the loop SCC is relaxed with enough internal
    sub-iterations does the fixed point emerge. Both sub-step scheduling modes
    (fixed equal steps, geometric shrinking steps) must converge.
    """
    fixture_root = REFERENCE_SSP_ROOT / fixture / "baseline"
    if not fixture_root.exists():
        pytest.skip(f"Missing fixture: {fixture_root}")

    workdir = tmp_path / f"{fixture}_{mode}"
    runtime_ssp_root = workdir / "ssp"
    workdir.mkdir(parents=True, exist_ok=True)
    shutil.copytree(fixture_root, runtime_ssp_root)
    config_path = write_loop_aware_config(runtime_ssp_root, workdir, mode, 32)

    simulator = pyssp4sim.Simulator(str(config_path))
    simulator.init()
    simulator.simulate()

    max_err = steady_state_max_error(workdir / "result.csv", 0.001, signals)
    assert max_err <= 0.05, (
        f"{fixture} loop-aware ({mode}) steady-state fixed-point error "
        f"{max_err:.5f} exceeds 0.05"
    )