---
id: INFO-015
type: info
title: Build environment
summary: Supported operating systems, toolchains, architectures, and build tools
date: 2026-11-11
status: current
---

# Build environment

Supported build environments: one primary platform, best-effort secondary platforms, no Windows or macOS.

- **Operating systems:** Ubuntu 22.04 (x86_64) primary, CI-tested, the release build target; other Linux (Fedora, Debian, Arch) secondary, expected to work, not CI-tested; Windows and macOS not supported.
- **Toolchain:** C++23; GCC 13+ primary (used in CI), Clang 16+ secondary, not CI-tested.
- **Architectures:** x86_64 primary, CI-tested, release target; aarch64 potential only, not tested.
- **Build tools:** CMake 3.25+ (required for presets), Ninja primary with Make as fallback, vcpkg package manager, Docker/Podman containers for reproducible builds, Python 3.9+ for Python bindings and workflow tests.
- **Reference command:** `cmake --preset=vcpkg && cmake --build build`.
- **CI environment:** GitHub Actions on an ubuntu-22.04 runner, container Ubuntu 22.04 + GCC 13 (`containers/ubuntu22-gcc13/Containerfile`); the release pipeline owns CI orchestration (`INFO-036`).
