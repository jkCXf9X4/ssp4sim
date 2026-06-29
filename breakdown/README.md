# Product Breakdown

SSP4SIM is a C++23 simulation engine that executes SSP 1.0 archives
containing FMI 2.0 co-simulation FMUs. It supports Gauss-Jacobi and
Gauss-Seidel execution strategies (parallel and serial), local database
recording with CSV export, tiered logging, and provides a CLI (`sim_app`), a
C API, and a Python API (`pyssp4sim`).

The product-breakdown structure organises project documentation into seven
layers. Each layer captures a distinct concern of the SSP4SIM project.

| Layer | Directory | What It Covers |
|---|---|---|
| 00 — Intent | [`00-intent/`](./00-intent/) | Project purpose, outcomes, users, assumptions, constraints, and intent-layer decisions |
| 01 — Product | [`01-product/`](./01-product/) | Scope, capabilities, domain model, glossary, use cases, requirements, and product decisions |
| 02 — Architecture | [`02-architecture/`](./02-architecture/) | Context, container, component views, data flow, quality attributes, and architecture decisions |
| 03 — Implementation | [`03-implementation/`](./03-implementation/) | Code structure, build environment, dependency policy, interfaces, configuration, and implementation decisions |
| 04 — Verification | [`04-verification/`](./04-verification/) | Test strategy, acceptance criteria, coverage targets, CI stages, regressions, traceability matrix, and verification decisions |
| 05 — Operation | [`05-operation/`](./05-operation/) | Deployment topology, release pipeline, runbook, monitoring, incident response, profiling, support model, and operation decisions |
| 06 — Evolution | [`06-evolution/`](./06-evolution/) | Improvement backlog, completed work, deferred items, contributing guide, and undeveloped suggestions |

For usage-oriented documentation (installation, CLI, Python API, configuration),
see [docs/README.md](../docs/README.md).
