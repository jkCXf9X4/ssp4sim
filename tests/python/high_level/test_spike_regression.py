from __future__ import annotations

import csv
import json
import math
import os
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


def write_signal_sine_gain_add_spike_config(ssp_root: Path, workdir: Path) -> Path:
    config: dict[str, Any] = json.loads(GENERIC_CONFIG_PATH.read_text())
    simulation = config["simulation"]

    simulation["ssp"] = str(ssp_root)
    simulation["ssd"] = "SystemStructure.ssd"
    simulation["start_time"] = 0.0
    simulation["stop_time"] = 1.0
    simulation["timestep"] = 0.001
    simulation["tolerance"] = 1e-4
    simulation["realtime"] = False
    simulation["working_dir"] = str(workdir)

    executor = simulation["executor"]
    executor["method"] = "jacobi"
    executor["thread_pool_workers"] = 5
    executor["forward_derivatives"] = True
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
    config_path = workdir / "generic_config.json"
    config_path.write_text(json.dumps(config, indent=2))
    return config_path


def calculate_signal_sine_gain_add_spike_metrics(
    result_file: Path,
) -> tuple[float, str, float, str]:
    assert result_file.exists(), f"Missing result file: {result_file}"

    with result_file.open(newline="") as input_file:
        rows = list(csv.DictReader(input_file))

    assert len(rows) >= 2, "Result file does not contain enough samples"
    for column in ["time", "gain.y", "sine.y"]:
        assert column in rows[0], f"Result file does not contain {column}"

    max_err = 0.0
    max_err_time = ""
    max_delta = 0.0
    max_delta_time = ""
    previous_gain = 0.0

    for row in rows[1:]:
        gain = parse_float(row["gain.y"])
        sine = parse_float(row["sine.y"])
        assert gain is not None and math.isfinite(gain)
        assert sine is not None and math.isfinite(sine)

        err = abs(gain - 3.0 * sine)
        if err > max_err:
            max_err = err
            max_err_time = row["time"]

        delta = abs(gain - previous_gain)
        if delta > max_delta:
            max_delta = delta
            max_delta_time = row["time"]

        previous_gain = gain

    return max_err, max_err_time, max_delta, max_delta_time


def test_signal_sine_gain_add_jacobi_parallel_has_no_output_spikes(
    tmp_path: Path,
) -> None:
    """Description: Runs 100 simulations (configurable via env var), checks gain
    output error <= 0.02 and delta <= 0.1.
    Rationale: Regression test for Jacobi parallel solver output quality.
    Creep flag: (a) 100 runs is the longest test in the suite. (b) Thresholds
    (0.02, 0.1) are empirically determined. (c) Couples to specific solver config
    (Jacobi parallel, 5 workers, forward derivatives). (d) Checks aggregate error
    bounds, not just transient spikes despite "spike" name.
    """
    runs = int(os.environ.get("SSP4SIM_SPIKE_RUNS", "100"))
    max_allowed_err = float(os.environ.get("SSP4SIM_SPIKE_MAX_ERR", "0.02"))
    max_allowed_delta = float(os.environ.get("SSP4SIM_SPIKE_MAX_DELTA", "0.1"))

    fixture_root = REFERENCE_SSP_ROOT / "signal_sine_gain_add" / "baseline"
    if not fixture_root.exists():
        pytest.skip(f"Missing fixture: {fixture_root}")

    workdir = tmp_path / "signal_sine_gain_add_spike_regression"
    runtime_ssp_root = workdir / "ssp"
    shutil.copytree(fixture_root, runtime_ssp_root)
    config_path = write_signal_sine_gain_add_spike_config(runtime_ssp_root, workdir)
    result_file = workdir / "result.csv"

    worst_err = 0.0
    worst_err_run = 0
    worst_err_time = ""
    worst_delta = 0.0
    worst_delta_run = 0
    worst_delta_time = ""

    for run in range(1, runs + 1):
        simulator = pyssp4sim.Simulator(str(config_path))
        simulator.init()
        simulator.simulate()

        max_err, max_err_time, max_delta, max_delta_time = (
            calculate_signal_sine_gain_add_spike_metrics(result_file)
        )

        if max_err > worst_err:
            worst_err = max_err
            worst_err_run = run
            worst_err_time = max_err_time

        if max_delta > worst_delta:
            worst_delta = max_delta
            worst_delta_run = run
            worst_delta_time = max_delta_time

        assert max_err <= max_allowed_err and max_delta <= max_allowed_delta, (
            "Spike detected in signal_sine_gain_add Jacobi parallel run "
            f"{run}/{runs}: max_err={max_err:.9f} at t={max_err_time}, "
            f"max_delta={max_delta:.9f} at t={max_delta_time}, "
            f"thresholds=({max_allowed_err}, {max_allowed_delta})"
        )

    assert worst_err <= max_allowed_err, (
        f"Worst max_err={worst_err:.9f} on run {worst_err_run} at t={worst_err_time}"
    )
    assert worst_delta <= max_allowed_delta, (
        f"Worst max_delta={worst_delta:.9f} on run {worst_delta_run} at t={worst_delta_time}"
    )
