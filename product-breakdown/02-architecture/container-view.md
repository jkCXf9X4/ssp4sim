# Container View
<!-- Layer: 02-architecture -->
<!-- Stable ID: ARCH-CONTAINER-001 -->

## Description

This artifact describes the deployable or runtime units that make up SSP4SIM. Each container is a separately deployable artifact.

## Containers

| Container | Type | Description | Build Target |
|---|---|---|---|
| **libssp4sim** | Static library (.a) | Core simulation engine — all module areas (analysis, execution, graph, handler, model, signal, utils, schema_extensions). Linked into all other containers. | `ssp4sim_lib` |
| **sim_app** | Command-line binary | CLI entry point — reads config, invokes Simulator lifecycle, reports progress. Links libssp4sim. | `ssp4sim_app` (in `public/ssp4sim_app/`) |
| **pyssp4sim** | Python extension module (.so) | Python bindings — wraps libssp4sim for import in Python workflows. | `pyssp4sim` (in `public/python_api/`) |
| **ssp4sim_tests** | Test binary | Catch2 test runner — exercises libssp4sim units and integration path. Not deployed. | `ssp4sim_tests` (in `tests/lib/`) |
| **FMU .so** | Shared library | Vendor-provided FMU implementation — loaded at runtime by fmi4c adapter. Not built by SSP4SIM. | External |

## Deployment Artifacts

Release tarball (`ssp4sim-linux-x86_64-<version>.tar.gz`) contains:
- `bin/sim_app` — CLI binary
- `lib/libssp4sim.a` — static library
- `include/ssp4sim/*.hpp`, `simulator_c_api.h` — public headers
- `python/pyssp4sim*.whl` — Python wheel
- `resources/` — example configs and SSP fixtures

## Traceability

- Backward: Traces to product scope in `product-breakdown/01-product/scope.md`.
- Sources: `lib/CMakeLists.txt`, `docs/linux_binary_distribution.md`, `public/` directory structure.
