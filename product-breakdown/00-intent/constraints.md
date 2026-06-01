# Constraints
<!-- Layer: 00-intent -->
<!-- Stable ID: INT-CONSTRAINTS-001 -->

## Description

This artifact documents known limits, non-goals, and binding constraints that shape the project's scope and design decisions.

## Content

- **Platform**: Linux-only. Ubuntu 22.04 is the primary target; x86_64 architecture.
- **FMI scope**: FMI 2.0 co-simulation only. No model exchange support.
- **SSP scope**: SSP 1.0 archives only.
- **Maturity**: Experimental, not production-grade. No SLAs, no formal support.
- **Interface**: No GUI, no dashboard, no web interface.
- **Platform support**: No Windows or macOS support planned.
- **Recording**: Current releases provide file-based CSV and local database
  artifacts only. No streaming or remote database output is available.

## Traceability

- Backward: Defined by the product-breakdown 00-intent layer framework.
- Sources: `docs/development.md` (lines 49, 64, 87), `docs/linux_binary_distribution.md` (line 5), `docs/overview.md` (lines 5-6).
