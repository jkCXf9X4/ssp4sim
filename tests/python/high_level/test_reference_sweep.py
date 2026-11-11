from __future__ import annotations

from pathlib import Path

import pytest

from ._helpers import (
    SOURCE_SSP_ROOT,
    assert_has_log_file,
    reference_params,
    run_reference_ssp,
)

# REG-003: the embrace fixture's BC FMU fails with "Failed to write string
# value to FMU" during simulator init. Postponed; see
# docs/breakdown/04-verification/regressions.md.
EMBRACE_XFAIL_REASON = (
    "embrace BC FMU: write_string fails during init (REG-003)"
)


def _reference_params_with_known_failures():
    params = reference_params()
    for param in params:
        param_id = param.id
        if param_id and param_id.startswith("embrace"):
            param = pytest.param(
                param.values,
                marks=[pytest.mark.xfail(
                    strict=False,
                    reason=EMBRACE_XFAIL_REASON,
                )],
                id=param.id,
            )
        yield param


PARAMETRIZED_SSP_ROOTS = list(_reference_params_with_known_failures()) + [
    pytest.param(
        SOURCE_SSP_ROOT / "dcmotor" / "ssp",
        id="dcmotor_nested/baseline",
    ),
]


@pytest.mark.parametrize("ssp_root", PARAMETRIZED_SSP_ROOTS)
def test_reference_ssp_fully_simulates(ssp_root: Path, tmp_path: Path) -> None:
    """Description: For each co-simulation SSP, run simulation and verify output.
    Rationale: Core regression suite — every reference SSP must simulate without
    crashing and produce output.
    Creep flag: dcmotor nested SSP is hardcoded outside the discovery mechanism,
    creating two code paths for parameter generation.
    """
    workdir = run_reference_ssp(ssp_root, tmp_path)
    assert_has_log_file(workdir)