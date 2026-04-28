from __future__ import annotations

import csv
import json
import math
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


if not (PYTHON_API_BUILD / "pyssp4sim").exists():
    pytest.skip("pyssp4sim build artifact is missing", allow_module_level=True)

pyssp4sim = pytest.importorskip("pyssp4sim")

if not Path(pyssp4sim.__file__).resolve().is_relative_to(PYTHON_API_BUILD.resolve()):
    pytest.fail(f"pyssp4sim imported from {pyssp4sim.__file__}, not {PYTHON_API_BUILD}")


SSP_NAMESPACE = {"ssd": "http://ssp-standard.org/SSP1/SystemStructureDescription"}
EXPECTED_REFERENCE_FAILURES = {
    "dcmotor": "Hierarchical SSP systems are not supported by the current flat graph builder.",
}

GENERIC_CONFIG_PATH = SSP4SIM_ROOT / "resources" / "generic_config.json"
REFERENCE_SSP_ROOT = SSP4SIM_ROOT / "tests" / "reference_ssp" / "build" / "models"


def uses_model_exchange(model_root: Path) -> bool:
    ssd_path = model_root / "ssp" / "SystemStructure.ssd"
    root = ET.parse(ssd_path).getroot()

    for component in root.findall(".//ssd:Component", SSP_NAMESPACE):
        if component.get("implementation") == "ModelExchange":
            return True

    return False


def discover_reference_ssps() -> list[Path]:
    if not REFERENCE_SSP_ROOT.exists():
        return []

    models = []
    for model_root in sorted(path for path in REFERENCE_SSP_ROOT.iterdir() if path.is_dir()):
        if not (model_root / "ssp" / "SystemStructure.ssd").exists():
            continue

        if uses_model_exchange(model_root):
            continue

        models.append(model_root)

    return models


def reference_params() -> list[pytest.ParameterSet]:
    models = discover_reference_ssps()
    if not models:
        return [
            pytest.param(
                None,
                marks=pytest.mark.skip(reason="No unpacked reference SSP fixtures found"),
                id="no-reference-ssps",
            )
        ]

    params = []
    for model_root in models:
        model_name = model_root.name
        marks = []
        if model_name in EXPECTED_REFERENCE_FAILURES:
            marks.append(
                pytest.mark.xfail(
                    raises=RuntimeError,
                    reason=EXPECTED_REFERENCE_FAILURES[model_name],
                    strict=True,
                )
            )
        params.append(pytest.param(model_root, marks=marks, id=model_name))

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

    recording = simulation["recording"]
    recording["enable"] = True
    recording["wait_for"] = True
    recording["interval"] = 0.1
    recording["result_file"] = str(workdir / "results.csv")

    log_config = simulation["log"]
    log_config["file"] = str(workdir / "sim.log")
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
    assert 0.0 <= times[0] <= 0.1
    assert times[-1] == pytest.approx(1.0)

    signal_columns = [column for column in rows[0] if column != "time"]
    assert signal_columns, "Result file does not contain recorded signals"
    assert any(
        parse_float(row[column]) is not None
        for row in rows
        for column in signal_columns
    ), "Result file does not contain any numeric signal samples"


@pytest.mark.parametrize("model_root", reference_params())
def test_reference_ssp_fully_simulates(model_root: Path, tmp_path: Path) -> None:
    workdir = tmp_path / model_root.name
    config_path = write_config(model_root / "ssp", workdir)

    simulator = pyssp4sim.Simulator(str(config_path))
    simulator.init()
    simulator.simulate()

    assert_result_file_is_complete(workdir / "results.csv")
