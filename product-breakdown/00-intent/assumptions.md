# Assumptions
<!-- Layer: 00-intent -->
<!-- Stable ID: INT-ASSUMPTIONS-001 -->

## Description

This artifact documents the assumptions the project depends on — conditions that must hold true for the project's purpose, outcomes, and design to be valid.

## Content

- SSP archives conform to the SSP 1.0 standard.
- FMUs provide valid model descriptions compliant with FMI 2.0.
- Host system has a C++23 toolchain (for source builds).
- Simulation is deterministic given the same SSP, configuration, and timing parameters.
- Users have basic familiarity with SSP, FMI, and co-simulation concepts.

## Traceability

- Backward: Defined by the product-breakdown 00-intent layer framework.
- Sources: `readme.md` (lines 11-12, 36), `docs/README.md` (lines 11-13).