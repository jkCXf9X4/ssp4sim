from __future__ import annotations

import csv
import json
import subprocess
from pathlib import Path


def repository_root(start: Path) -> Path:
    current = start.resolve()

    for parent in [current] + list(current.parents):
        if (parent / "venv").exists() and (parent / "resources" / "embrace" / "embrace.json").exists():
            return parent

    raise FileNotFoundError("Could not find the repository root")


def build_embrace_config(root: Path, workdir: Path) -> Path:
    config_path = root / "resources" / "embrace" / "embrace.json"
    config = json.loads(config_path.read_text())

    simulation = config["simulation"]
    simulation["stop_time"] = 1.0
    simulation["working_dir"] = str(workdir)
    simulation["log"]["file"] = str(workdir / "sim.log")

    recording = simulation["recording"]
    recording["enable"] = True
    recording["result_file"] = str(workdir / "result.csv")
    recording["csv"] = {
        "enable": True,
        "file": str(workdir / "result.csv"),
    }

    workdir.mkdir(parents=True, exist_ok=True)
    smoke_config = workdir / "embrace_smoke.json"
    smoke_config.write_text(json.dumps(config, indent=2))
    return smoke_config


def assert_result_file_has_rows(result_file: Path) -> None:
    assert result_file.exists(), f"Missing result file: {result_file}"

    with result_file.open(newline="") as input_file:
        rows = list(csv.DictReader(input_file))

    assert rows, "Result file does not contain any samples"
    assert "time" in rows[0], "Result file does not contain a time column"


def assert_has_log_file(workdir: Path) -> None:
    assert any(path.is_file() for path in workdir.glob("sim*.log")), "Missing log file matching sim*.log"


def test_pyssp4sim_smoke_runs_local_embrace(tmp_path: Path) -> None:
    root = repository_root(Path(__file__))
    smoke_config = build_embrace_config(root, tmp_path)
    command = [str(root / "venv" / "bin" / "pyssp4sim"), str(smoke_config)]

    completed = subprocess.run(
        command,
        cwd=root,
        capture_output=True,
        text=True,
    )

    assert completed.returncode == 0, (
        f"pyssp4sim failed with exit code {completed.returncode}\n"
        f"stdout:\n{completed.stdout}\n"
        f"stderr:\n{completed.stderr}"
    )

    assert_result_file_has_rows(tmp_path / "result.csv")
    assert_has_log_file(tmp_path)
