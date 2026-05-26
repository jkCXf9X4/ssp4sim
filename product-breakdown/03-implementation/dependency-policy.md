# Dependency Policy
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-DEPPOLICY-001 -->

## Description

This artifact catalogs all external dependencies and documents the version policy, vendoring decisions, and conflict resolution strategy.

## Dependency Catalog

| Dependency | Type | Source | License | Purpose |
|---|---|---|---|---|
| nlohmann_json | Build (vcpkg) | vcpkg | MIT | JSON parsing for configuration and SSP extensions |
| TBB/tbbmalloc | Build (vcpkg) | vcpkg | Apache 2.0 | Thread pool for parallel execution |
| quill | Build (vcpkg) | vcpkg | Apache 2.0 | Logging framework |
| DuckDB | Build (vcpkg) | vcpkg | MIT | Alternative recording backend |
| ssp4cpp | Vendored (3rdParty) | Git subtree | MIT | SSP archive parsing |
| fmi4c | Vendored (3rdParty) | Git subtree | MIT | FMI 2.0 adapter library |

## Vendoring Decisions

- **Why vendored**: ssp4cpp and fmi4c are vendored because they are project-specific, actively co-developed with SSP4SIM, and have no stable releases on vcpkg or other package managers.
- **Why vcpkg**: The remaining dependencies (nlohmann_json, TBB, quill, DuckDB) are well-established, versioned on vcpkg, and benefit from centralized version management.

## Version Policy

- vcpkg dependencies: Versions are pinned in `vcpkg.json`. Updates are made deliberately when new features or bug fixes are needed.
- Vendored dependencies: Updated by pulling from upstream when a specific fix or feature is required.
- No automatic dependency update bots.
- Version bumps should include a changelog note and verification against the full test suite.

## Conflict Resolution

- No known conflicts between current dependency versions.
- If conflicts arise: upgrade conflicting dependency first, then adjust consuming code.
- Vendored dependencies can be patched locally if upstream is unresponsive.

## Generated Data Policy

The following build artifacts and outputs are treated as generated or fixture data. Do not normalize, repackage, or rewrite them unless the requested change explicitly requires it:

- `build/` directory contents
- Unpacked SSP archives and FMU binaries
- Result CSVs and DuckDB databases
- Log files
- FMU/SSP binaries

### Handling Mutable Fixtures

- Many FMU and SSP workflows assume Linux x86_64 binaries.
- Preserve executable permissions for libraries under `binaries/`.
- Prefer copying fixtures to a temporary location when a loader or unpacking step may mutate files.
- The fmi4c loader marks Linux shared libraries executable before dlopen — loading Git-tracked fixtures directly could dirty the working tree. Prefer tests that copy mutable fixtures into a temporary directory before loading them.

### Python Environment

- Use the repo-local `venv` for Python commands when it exists.
- Prefer `. venv/bin/activate && <command>` or `venv/bin/python <command>` over system Python.
- Python dependencies, vcpkg features, and platform-specific FMU tooling are part of the reproducible environment.

## Traceability

- Backward: Architecture container view (deployment dependencies).
- Sources: `vcpkg.json`, `3rdParty/CMakeLists.txt`, `lib/CMakeLists.txt`.