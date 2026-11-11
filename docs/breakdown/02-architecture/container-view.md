---
id: INFO-011
type: info
title: Container view
summary: Deployable runtime units of SSP4SIM and the release artifact
date: 2026-11-11
status: current
---

# Container view

Five deployable units, each a separately deployable artifact; one static core linked into every runnable container.

- **libssp4sim:** static library; the core engine linking all module areas; linked into all other containers.
- **sim_app:** command-line binary; reads a config file, runs the Simulator lifecycle, reports progress.
- **pyssp4sim:** Python extension module; Python bindings wrapping libssp4sim.
- **ssp4sim_tests:** Catch2 test runner; exercises libssp4sim units and integration paths; not deployed.
- **FMU .so:** vendor-provided FMU implementation, loaded at runtime by the fmi4c adapter; not built by SSP4SIM.
- **Release artifact:** tarball `ssp4sim-linux-x86_64-<version>.tar.gz` carrying bin, lib, include, python, and resources; packaging and release are operations (`INFO-036`).
- **Build mapping:** container-to-build-target mapping lives in the build environment (`INFO-015`).
