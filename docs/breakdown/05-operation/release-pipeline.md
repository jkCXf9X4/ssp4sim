---
id: INFO-036
type: info
title: Release Pipeline
summary: Six-stage GitHub Actions release pipeline producing tarball and wheel, publishing on version tags
date: 2026-11-11
status: current
---

# Release Pipeline

The release pipeline builds, packages, and publishes the Linux tarball and Python wheel; defined in `.github/workflows/linux-release.yml`.

- **Artifacts** (same tag and SHA):
  - `ssp4sim-linux-x86_64-<version>.tar.gz` plus a `latest` alias.
  - `pyssp4sim-<version>-*.whl` plus a dev alias.
  - Tarball contents: bin/, lib/, include/, python/, resources/, readme.md, LICENSE, RELEASE.txt.
- **CI build and test:** the workflow runs on ubuntu-22.04, builds the Ubuntu 22.04 + GCC 13 container from `containers/ubuntu22-gcc13/Containerfile`, and runs configure, build, test, install, and packaging inside it with the same commands as the build-from-source and test docs.
- **Staging:** CMake install rules copy build artifacts to a staging tree under `dist/`.
- **Packaging:** creates tarball, wheel, aliases, and SHA256SUMS in `dist/`; RELEASE.txt records version, commit SHA, and UTC build timestamp.
- **Publish:** on a push of a tag matching `v*`, publishes to GitHub Releases; on workflow_dispatch, uploads as build artifacts.
- **Consumption:** consumers use the tarball and wheel as described in the installation and usage docs.
- **Local parity:** use the same container scripts locally when investigating release failures.
