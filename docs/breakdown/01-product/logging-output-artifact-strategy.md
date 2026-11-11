---
id: INFO-008
type: info
title: Logging Output Artifact Strategy
summary: The log output tiers SSP4SIM produces and what each tier is for
date: 2026-11-11
status: current
---

# Logging Output Artifact Strategy

SSP4SIM produces four logging tiers, each targeting a distinct consumption mode.

- **Terminal / File Overview:** plain-text, configurable level; real-time operator awareness during a simulation.
- **Full JSON Archival:** machine-readable file for offline filtering, correlation, and automation; the canonical structured log artifact (`UC-008`).
- **Cutelog Live Navigation:** optional interactive stream for runtime log inspection with filtering and search; requires an external viewer.
- **OpenTelemetry Distributed Trace Export:** proposed tier for correlating observability across simulation instances; not yet implemented (`IMP-031`, `UC-010`).
- **Hot-Path Disable:** hot-path logging can be compiled out at build time with zero runtime overhead.
- **Consumption modes:** real-time awareness, offline machine analysis, interactive debugging, and distributed cross-process observability; the distributed tier complements the local tiers, it does not replace them.
