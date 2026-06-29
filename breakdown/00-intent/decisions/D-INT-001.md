# D-INT-001: Linux-First Platform Focus
<!-- Layer: 00-intent -->
<!-- Status: Accepted -->

## Context

SSP4SIM development and release workflows target Linux, with Ubuntu 22.04 x86_64 as the primary build and runtime platform. Container builds, CI pipelines, and binary distribution are Linux-only.

## Decision

Make Linux the sole supported platform. No Windows or macOS support is planned at the intent level. Platform portability is explicitly out of scope.

## Rationale

- The SSP and FMI ecosystem for co-simulation workflows is predominantly Linux-based (containerized CI, HPC environments, research pipelines).
- Narrowing platform scope keeps the codebase compact and reduces CI matrix complexity, consistent with the project's experimental, small-surface-area design posture.
- All existing build infrastructure (vcpkg presets, Containerfile, GitHub Actions workflows) is Linux-specific.

## Consequences

- Cross-platform patches will not be accepted unless the project's intent-level platform constraint is revisited.
- Users on non-Linux platforms must use containers or VMs.
- The release pipeline (`docs/linux_binary_distribution.md`) can remain Linux-only.

## Traceability

- Backward: Constraint documented in `product-breakdown/00-intent/constraints.md` (platform: Linux-only).
- Sources: `docs/development.md` (line 64, 87), `docs/linux_binary_distribution.md` (line 5), `docs/build_from_source.md` (line 13).