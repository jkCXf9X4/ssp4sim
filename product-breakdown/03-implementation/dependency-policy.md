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

## Traceability

- Backward: Architecture container view (deployment dependencies).
- Sources: `vcpkg.json`, `3rdParty/CMakeLists.txt`, `lib/CMakeLists.txt`.