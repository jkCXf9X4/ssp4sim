from __future__ import annotations

from pathlib import Path

import pytest

from ._helpers import (
    SOURCE_SSP_ROOT,
    assert_has_log_file,
    reference_params,
    run_reference_ssp,
)


# signal_init_guard is a known, documented hard failure on the ssp4sim backend:
# ssp4sim initializes each FMU independently, so the guard's connected input u
# holds its default 0.0 at fmi2ExitInitializationMode and the InitGuard assert
# fires (resources/reference_ssp/models/ssp/signal_init_guard/FIXTURE.md). It is
# expected to fail (xfail) until IMP-048 lands; strict keeps it loud if it ever
# starts passing so the marker is revisited.
PARAMETRIZED_SSP_ROOTS = []
for _param in reference_params() + [
    pytest.param(
        SOURCE_SSP_ROOT / "dcmotor" / "ssp",
        id="dcmotor_nested/baseline",
    )
]:
    if getattr(_param, "id", None) and _param.id.startswith("signal_init_guard"):
        _param = _param._replace(
            marks=list(_param.marks)
            + [
                pytest.mark.xfail(
                    reason=(
                        "[exit_init] exit_initialization_mode failed for guard: "
                        "ssp4sim initializes each FMU independently, so guard.u "
                        "holds default 0.0 at fmi2ExitInitializationMode and the "
                        "InitGuard assert fires (FIXTURE.md; IMP-048)."
                    ),
                    strict=True,
                )
            ]
        )
    PARAMETRIZED_SSP_ROOTS.append(_param)


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