# Release Pipeline
<!-- Layer: 05-operation -->
<!-- Stable ID: OPS-RELEASE-001 -->

## Description

The release pipeline produces Linux binaries and a Python wheel for distribution. Defined in `.github/workflows/linux-release.yml`.

## Stage 1: Distribution Target

Release artifacts (same tag/SHA):
- ssp4sim-linux-x86_64-\<version\>.tar.gz
- pyssp4sim-\<version\>-*.whl
- ssp4sim-linux-x86_64-latest.tar.gz (alias)
- pyssp4sim-0.0.0.dev0-cp39-abi3-linux_x86_64.whl (alias)

Tarball contents: bin/, lib/, include/, python/, resources/, readme.md, LICENSE, RELEASE.txt.

## Stage 2: CI Build and Test

- Workflow: .github/workflows/linux-release.yml
- Runs on ubuntu-22.04
- Builds Ubuntu 22.04 + GCC 13 container from containers/ubuntu22-gcc13/Containerfile
- Runs configure, build, test, install, packaging inside container
- Uses same commands as Build From Source and tests/README.md

## Stage 3: Install to Staging

Install rules in the CMakeLists.txt files copy build artifacts to a staging directory tree under `dist/`. See [Linux Binary Distribution](../../docs/linux_binary_distribution.md#stage-3-install-to-staging-directory) for the exact command.

## Stage 4: Package Artifacts

Creates in dist/:
- ssp4sim-linux-x86_64-\<version\>.tar.gz
- pyssp4sim-\<version\>-*.whl
- Aliases and SHA256SUMS

RELEASE.txt contains: version, commit SHA, UTC build timestamp.

## Stage 5: Publish

On push tags matching v*, publishes to GitHub Releases. On workflow_dispatch, uploads as build artifacts.

## Stage 6: Consumption

Consumers use the tarball and wheel as described in Installation and Usage docs.

## Local Release Parity

Use the same container scripts locally when investigating release failures.

## Traceability

- Backward: Deployment topology in `product-breakdown/05-operation/deployment-topology.md`.
- Sources: `docs/linux_binary_distribution.md`, `.github/workflows/linux-release.yml`.
