# Deployment Topology
<!-- Layer: 05-operation -->
<!-- Stable ID: OPS-DEPLOY-001 -->

## Description

This artifact documents the filesystem layout and deployment structure for SSP4SIM.

## Release Tarball Layout

```
ssp4sim-linux-x86_64-<version>/
├── bin/
│   └── sim_app             # CLI binary
├── lib/
│   └── libssp4sim.a        # Static library
├── include/
│   └── ssp4sim/
│       ├── simulator.hpp
│       ├── simulator_c_api.h
│       └── shared_config.hpp
├── python/
│   └── pyssp4sim-*.whl     # Python wheel
├── resources/
│   ├── embrace/
│   │   └── embrace.json
│   └── generic_config.json
├── readme.md
├── LICENSE
└── RELEASE.txt
```

## Installation Paths

- **System-wide**: Extract tarball to `/opt/ssp4sim/` or `/usr/local/`
- **User-local**: Extract to `~/.local/ssp4sim/`
- **Python**: Install wheel via `pip install python/pyssp4sim-*.whl`
- **From source**: Build with CMake presets, use build output directly

## Container Deployment

- Docker/Podman image: built from `containers/ubuntu22-gcc13/Containerfile`
- Reference: `docs/build_from_source.md` for container build commands

## Environment

- Linux x86_64 (Ubuntu 22.04 primary)
- No environment variables required (all configuration via JSON file)
- `working_dir` config key anchors output locations

## Traceability

- Backward: Implementation build environment (`product-breakdown/03-implementation/build-environment.md`).
- Sources: `docs/linux_binary_distribution.md`, `docs/installation.md`, `docs/build_from_source.md`.
