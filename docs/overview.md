# Overview
<!-- Layer: 00-intent, 01-product -->


SSP4SIM is a C++23 simulation engine that executes SSP 1.0 archives
containing FMI 2.0 co-simulation FMUs. It supports Gauss-Jacobi and
Gauss-Seidel execution strategies (parallel and serial), CSV and DuckDB
recording, and provides a CLI (`sim_app`), a C API, and a Python API (`pyssp4sim`).

## Product

- [Scope](../product-breakdown/01-product/scope.md) — what is in and out of scope
- [Capabilities](../product-breakdown/01-product/capabilities.md) — full feature list
- [Domain Model](../product-breakdown/01-product/domain-model.md) — core concepts and their relationships
- [Glossary](../product-breakdown/01-product/glossary.md) — shared vocabulary

## Setup

- [Installation](installation.md) — prebuilt release tarballs and wheels
- [Build From Source](build_from_source.md) — local and container builds

## Usage

- [Usage](usage.md) — CLI and Python invocation, example configs, output formats
- [Configuration](configuration.md) — JSON key reference, types, defaults
- [Logging](logging_guidlines.md) — levels, hot-path guidance, sink setup
- [Profiling](profiling.md) — build and runtime profiling commands

## Development

- [Development Guide](development.md) — contributor workflow and conventions
- [Tests](../tests/README.md) — C++ and Python test layout and commands
- [Release Pipeline](linux_binary_distribution.md) — Linux binary and wheel packaging
