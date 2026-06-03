from __future__ import annotations

from pathlib import Path

import pytest

from ._helpers import (
    PYTHON_TEST_RESULTS,
    REFERENCE_SSP_ROOT,
    REFERENCE_SSP_ROOTS,
    SOURCE_SSP_ROOT,
    run_reference_ssp,
)


def assert_start_values_contain(
    start_values_text: str, expected_lines: list[str]
) -> None:
    for expected_line in expected_lines:
        assert expected_line in start_values_text, (
            f"Missing start value entry: {expected_line}"
        )


def resolve_test_root(ssp_root: Path) -> Path:
    for root in REFERENCE_SSP_ROOTS:
        try:
            return PYTHON_TEST_RESULTS / ssp_root.relative_to(root)
        except ValueError:
            continue
    return PYTHON_TEST_RESULTS / ssp_root.name


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
        pytest.param(
            SOURCE_SSP_ROOT / "dcmotor" / "ssp",
            [
                "0, SuT.edrive_mass.damper.d, 0.001000",
                "0, SuT.edrive_mass.inertia.J, 0.002000",
                "0, SuT.emachine_model.emf.k, 0.010000",
                "0, SuT.emachine_model.resistor.R, 1.000000",
                "0, stimuli_model.Voltage_step.height, 12.000000",
                "0, stimuli_model.MLoad.k, -0.500000",
            ],
            marks=pytest.mark.xfail(
                reason="Unsupported hierarchical or unresolved SSP connection: stimuli_model -> SuT"
            ),
            id="dcmotor_nested/baseline",
        ),
    ],
)
def test_reference_ssp_applies_parameter_set_variants(
    ssp_root: Path, expected_lines: list[str], tmp_path: Path
) -> None:
    tmp_path = resolve_test_root(ssp_root)
    workdir = run_reference_ssp(ssp_root, tmp_path)
    if expected_lines:
        start_values_text = (workdir / "start_values.csv").read_text()
        assert_start_values_contain(start_values_text, expected_lines)
