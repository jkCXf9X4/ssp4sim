# Build Environment
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-BUILDENV-001 -->

## Description

This artifact documents the supported build environments for SSP4SIM: operating systems, compilers, build tools, and architectures.

## Operating Systems

| OS | Status | Notes |
|---|---|---|
| Ubuntu 22.04 (x86_64) | Primary | Tested in CI; release build target |
| Other Linux (Fedora, Debian, Arch) | Secondary | Should work with appropriate toolchain; not CI-tested |
| Windows / macOS | Not supported | No CI, no testing, no planned support |

## Compilers

| Compiler | Minimum Version | C++ Standard | Status |
|---|---|---|---|
| GCC | 13+ | C++23 | Primary — used in CI (GCC 13 in Ubuntu 22.04 container) |
| Clang | 16+ | C++23 | Secondary — should work; not CI-tested |

## Architectures

| Architecture | Status | Notes |
|---|---|---|
| x86_64 | Primary | Tested in CI; release build target |
| aarch64 | Potential | Not tested; may work with appropriate toolchain |

## Build Tools

- **CMake**: 3.25+ (required for presets support)
- **Build system**: Ninja (primary), Make (fallback)
- **Package manager**: vcpkg (with CMake presets)
- **Container**: Docker/Podman (for reproducible builds, CI)
- **Python**: 3.9+ (for Python bindings and workflow tests)

## CI Environment

- Platform: GitHub Actions, ubuntu-22.04 runner
- Container: Ubuntu 22.04 + GCC 13 (from `containers/ubuntu22-gcc13/Containerfile`)
- Reference build command: `cmake --preset=vcpkg && cmake --build build`

## Traceability

- Backward: Architecture quality attributes (maintainability — cross-platform not a goal).
- Sources: `docs/build_from_source.md`, `CMakeLists.txt`, `CMakePresets.json`, `.github/workflows/linux-release.yml`.