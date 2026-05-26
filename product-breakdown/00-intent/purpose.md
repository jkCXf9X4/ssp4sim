# Purpose
<!-- Layer: 00-intent -->
<!-- Stable ID: INT-PURPOSE-001 -->

## Description

This artifact captures the problem statement and rationale for the SSP4SIM project — why the project exists, what gap it fills, and the design posture it takes.

## Content

SSP4SIM is a C++23 library and application for simulating SSP (System Structure and Parameterization) archives. The goal is to provide a small experimental simulation engine for developing and testing simulation strategies.

The project focuses on a direct path from SSP input to an executable run, keeping the runtime surface area intentionally narrow. This enables embedding in C++ and Python workflows without a large external runtime while keeping the codebase compact and understandable.

## Traceability

- Backward: Defined by the product-breakdown 00-intent layer framework.
- Sources: `readme.md` (lines 5-7), `docs/overview.md` (lines 5-23).