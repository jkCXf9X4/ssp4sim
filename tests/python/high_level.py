from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import pytest

pyssp4sim = pytest.importorskip("pyssp4sim")

from pathlib import Path

def find_upwards(start: Path, target: str) -> Path:
    """
    Walk upwards from `start` until a file or directory named `target` is found.
    Returns the Path to the directory containing it.
    Raises FileNotFoundError if not found.
    """
    current = start.resolve()

    for parent in [current] + list(current.parents):
        if (parent / target).exists():
            return parent

    raise FileNotFoundError(f"Could not find '{target}' in any parent directory")


SSP4SIM_ROOT = find_upwards(Path(__file__), "__SSP4SIM_ROOT__")

GENERIC_CONFIG_PATH = SSP4SIM_ROOT / "resources" / "generic_config.json"
REFERENCE_SSP_ROOT = SSP4SIM_ROOT / "tests" / "reference_ssp" / "build" / "models"


def write_config(ssp_root: Path, workdir: Path) -> Path:
    config = json.loads(GENERIC_CONFIG_PATH.read_text())
    simulation = config["simulation"]
    recording = simulation["recording"]
    log_config = simulation["log"]

    simulation["ssp"] = str(ssp_root)
    simulation["ssd"] = "SystemStructure.ssd"
    recording["enable"] = False
    recording["result_file"] = str(workdir / "results.csv")
    log_config["file"] = str(workdir / "sim.log")

    workdir.mkdir(parents=True, exist_ok=True)
    config_path = workdir / "generic_config.json"
    config_path.write_text(json.dumps(config, indent=2))
    return config_path


def init_simulator(ssp_root: Path, workdir: Path):
    config_path = write_config(ssp_root, workdir)
    simulator = pyssp4sim.Simulator(str(config_path))
    simulator.init()
    return simulator


# def test_embrace(
#     tmp_path
# ) -> None:
#     name = "signal_step_gain"
#     workdir = SSP4SIM_ROOT / "build" / "pytest" / name
#     simulator = init_simulator(REFERENCE_SSP_ROOT / name / "ssp", workdir)
#     assert simulator is not None


def test_embrace_local(
    tmp_path
) -> None:
    # resources/embrace/embrace.json
    config = SSP4SIM_ROOT / "resources" / "embrace" / "embrace.json"
    print(config)
    sim = pyssp4sim.Simulator(config.as_posix())
    sim.init()
    sim.simulate()

