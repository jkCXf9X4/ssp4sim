from __future__ import annotations

import csv
import json
import math
import shutil
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Any

import pytest


def find_upwards(start: Path, target: str) -> Path:
    current = start.resolve()

    for parent in [current] + list(current.parents):
        if (parent / target).exists():
            return parent

    raise FileNotFoundError(f"Could not find '{target}' in any parent directory")


SSP4SIM_ROOT = find_upwards(Path(__file__), "__SSP4SIM_ROOT__")
PYTHON_API_BUILD = SSP4SIM_ROOT / "build" / "public" / "python_api"
PYTHON_TEST_RESULTS = SSP4SIM_ROOT / "build" / "python_high"


if not (PYTHON_API_BUILD / "pyssp4sim").exists():
    pytest.skip("pyssp4sim build artifact is missing", allow_module_level=True)

pyssp4sim = pytest.importorskip("pyssp4sim")

if not Path(pyssp4sim.__file__).resolve().is_relative_to(PYTHON_API_BUILD.resolve()):
    pytest.fail(f"pyssp4sim imported from {pyssp4sim.__file__}, not {PYTHON_API_BUILD}")


SSP_NAMESPACE = {"ssd": "http://ssp-standard.org/SSP1/SystemStructureDescription"}

GENERIC_CONFIG_PATH = SSP4SIM_ROOT / "resources" / "generic_config.json"
REFERENCE_SSP_ROOT = (
    SSP4SIM_ROOT / "resources" / "reference_ssp" / "artifacts" / "models"
)


def uses_model_exchange(ssp_root: Path) -> bool:
    ssd_path = ssp_root / "SystemStructure.ssd"
    root = ET.parse(ssd_path).getroot()

    for component in root.findall(".//ssd:Component", SSP_NAMESPACE):
        if component.get("implementation") == "ModelExchange":
            return True

    return False


def discover_reference_ssps() -> list[Path]:
    if not REFERENCE_SSP_ROOT.exists():
        return []

    models = []
    for model_dir in sorted(path for path in REFERENCE_SSP_ROOT.iterdir() if path.is_dir()):
        for experiment_root in sorted(
            path for path in model_dir.iterdir() if path.is_dir()
        ):
            if not (experiment_root / "SystemStructure.ssd").exists():
                continue

            if uses_model_exchange(experiment_root):
                continue

            models.append(experiment_root)

    return models


def reference_params() -> list[pytest.ParameterSet]:
    models = discover_reference_ssps()
    if not models:
        return [
            pytest.param(
                None,
                marks=pytest.mark.skip(
                    reason="No unpacked reference SSP fixtures found"
                ),
                id="no-reference-ssps",
            )
        ]

    params = []
    for ssp_root in models:
        model_name = ssp_root.parent.name
        experiment_name = ssp_root.name
        params.append(pytest.param(ssp_root, id=f"{model_name}/{experiment_name}"))

    return params


def write_config(ssp_root: Path, workdir: Path) -> Path:
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

    recording = simulation["recording"]
    recording.pop("enable", None)
    recording.pop("result_file", None)
    recording["csv"] = {
        "enable": True,
        "file": str(workdir / "result.csv"),
    }
    recording["wait_for"] = True
    recording["interval"] = 0.1
    log_config = simulation["log"]
    log_config["level_terminal"] = "error"
    log_config["level_file"] = "info"
    log_config["level_json"] = "info"

    workdir.mkdir(parents=True, exist_ok=True)
    config_path = workdir / "generic_config.json"
    config_path.write_text(json.dumps(config, indent=2))
    return config_path


def parse_float(value: str) -> float | None:
    value = value.strip()
    if not value:
        return None
    return float(value)


def assert_result_file_is_complete(result_file: Path) -> None:
    assert result_file.exists(), f"Missing result file: {result_file}"

    with result_file.open(newline="") as input_file:
        rows = list(csv.DictReader(input_file))

    assert rows, "Result file does not contain any samples"
    assert "time" in rows[0], "Result file does not contain a time column"

    times = [parse_float(row["time"]) for row in rows]
    assert all(time is not None and math.isfinite(time) for time in times)
    assert times == sorted(times)

    signal_columns = [column for column in rows[0] if column != "time"]
    assert signal_columns, "Result file does not contain recorded signals"
    assert any(
        parse_float(row[column]) is not None
        for row in rows
        for column in signal_columns
    ), "Result file does not contain any numeric signal samples"


def assert_has_log_file(workdir: Path) -> None:
    assert any(path.is_file() for path in workdir.glob("sim*.log")), (
        "Missing log file matching sim*.log"
    )


def run_reference_ssp(ssp_root: Path, tmp_path: Path) -> Path:
    workdir = tmp_path / ssp_root.parent.name / ssp_root.name
    workdir.mkdir(parents=True, exist_ok=True)
    runtime_ssp_root = workdir / "ssp"
    shutil.copytree(ssp_root, runtime_ssp_root)
    config_path = write_config(runtime_ssp_root, workdir)

    simulator = pyssp4sim.Simulator(str(config_path))
    simulator.init()
    simulator.simulate()

    assert_result_file_is_complete(workdir / "result.csv")
    return workdir


@pytest.mark.parametrize("ssp_root", reference_params())
def test_reference_ssp_fully_simulates(ssp_root: Path, tmp_path: Path) -> None:
    workdir = run_reference_ssp(ssp_root, tmp_path)
    assert_has_log_file(workdir)

def assert_start_values_contain(
    start_values_text: str, expected_lines: list[str]
) -> None:
    for expected_line in expected_lines:
        assert expected_line in start_values_text, (
            f"Missing start value entry: {expected_line}"
        )


@pytest.mark.parametrize(
    "ssp_root, expected_lines",
    [
        pytest.param(
            REFERENCE_SSP_ROOT / "signal_step_gain" / "baseline",
            [
                "0, gain.k, 3.000000",
                "0, step.height, 2.000000",
                "0, step.offset, 1.000000",
                "0, step.startTime, 0.250000",
            ],
            id="signal_step_gain/baseline",
        ),
        pytest.param(
            REFERENCE_SSP_ROOT / "scenario" / "baseline",
            [
                "0, scenario.scenario_input, Alt;L;0,0;300,5000",
                "Mach;L;0,0;300,1.2",
                "heat_load;ZOH;0,50;150,4000",
                "Wgt_On_Whl;ZOH;0,1;150,0",
                "Aircraft_state;ZOH;0,0;150,4",
            ],
            id="scenario/baseline",
        ),
        pytest.param(
            REFERENCE_SSP_ROOT / "signal_sine_gain_add" / "baseline",
            [
                "0, sine.amplitude, 1.000000",
                "0, sine.f, 1.000000",
                "0, sine.offset, 0.000000",
                "0, sine.phase, 0.000000",
                "0, sine.startTime, 0.000000",
                "0, step.height, 2.000000",
                "0, step.offset, 0.000000",
                "0, step.startTime, 0.500000",
                "0, gain.k, 3.000000",
                "0, add.k1, 1.000000",
                "0, add.k2, 1.000000",
            ],
            id="signal_sine_gain_add/baseline",
        ),
        pytest.param(
            REFERENCE_SSP_ROOT / "signal_step_product" / "baseline",
            [
                "0, step.height, 1.000000",
                "0, step.offset, 0.000000",
                "0, step.startTime, 0.250000",
                "0, sine.amplitude, 1.000000",
                "0, sine.f, 1.000000",
                "0, sine.offset, 0.000000",
                "0, sine.phase, 0.000000",
                "0, sine.startTime, 0.000000",
            ],
            id="signal_step_product/baseline",
        ),
        pytest.param(
            REFERENCE_SSP_ROOT / "VanDerPol" / "fast",
            ["0, fmu.mu, 2.000000"],
            id="VanDerPol/fast",
        ),
        pytest.param(
            REFERENCE_SSP_ROOT / "dcmotor" / "baseline",
            [
                "0, SuT_edrive_mass.M_gain.k, -1.000000",
                "0, SuT_edrive_mass.inertia.J, 0.002000",
                "0, SuT_emachine_model.emf.k, 0.100000",
                "0, SuT_emachine_model.resistor.R, 0.500000",
                "0, stimuli_model.Voltage_step.height, 12.000000",
                "0, stimuli_model.MLoad.k, -0.500000",
            ],
            id="dcmotor/baseline",
        ),
    ],
)
def test_reference_ssp_applies_parameter_set_variants(
    ssp_root: Path, expected_lines: list[str], tmp_path: Path
) -> None:
    tmp_path =  PYTHON_TEST_RESULTS / ssp_root.relative_to(REFERENCE_SSP_ROOT)
    workdir = run_reference_ssp(ssp_root, tmp_path)
    if expected_lines:
        start_values_text = (workdir / "start_values.csv").read_text()
        assert_start_values_contain(start_values_text, expected_lines)
