---
id: INFO-018
type: info
title: Dependency policy
summary: External dependency catalog, version policy, vendoring, and generated-data rules
date: 2026-11-11
status: current
---

# Dependency policy

Two dependency classes — vendored and vcpkg-managed — with explicit version and generated-data rules.

- **Vendored:** ssp4cpp (SSP archive parsing) and fmi4c (FMI 2.0 adapter), both MIT, via git subtree; vendored because project-specific, co-developed with SSP4SIM, and without stable releases on vcpkg or other package managers.
- **vcpkg-managed:** nlohmann_json (MIT, config and SSP extensions), TBB/tbbmalloc (Apache-2.0, thread pool), quill (Apache-2.0, logging); well-established, versioned on vcpkg, pinned in `vcpkg.json`.
- **Version updates:** vcpkg deps updated deliberately when features or fixes are needed; vendored deps pulled from upstream when a specific fix or feature is required; no automatic update bots; bumps include a changelog note and verification against the full test suite.
- **Conflicts:** none known between current versions; if one arises, upgrade the conflicting dependency first, then adjust consuming code; vendored deps may be patched locally if upstream is unresponsive.
- **Generated data:** build artifacts, unpacked SSP archives and FMU binaries, result CSVs and SQLite databases, and log files are generated or fixture data — never normalized, repackaged, or rewritten unless the change requires it.
- **Mutable fixtures:** workflows assume Linux x86_64 binaries; preserve executable permissions under `binaries/`; copy fixtures to a temporary location when a loader may mutate files — the fmi4c loader marks Linux shared libraries executable before dlopen, so loading Git-tracked fixtures directly could dirty the working tree.
- **Python environment:** use the repo-local `venv` when present (`venv/bin/python` or `. venv/bin/activate`); Python dependencies, vcpkg features, and platform-specific FMU tooling are part of the reproducible environment.
