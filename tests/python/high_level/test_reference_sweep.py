from __future__ import annotations

from pathlib import Path

import pytest

from ._helpers import (
    assert_has_log_file,
    reference_params,
    run_reference_ssp,
)


@pytest.mark.parametrize("ssp_root", reference_params())
def test_reference_ssp_fully_simulates(ssp_root: Path, tmp_path: Path) -> None:
    workdir = run_reference_ssp(ssp_root, tmp_path)
    assert_has_log_file(workdir)
