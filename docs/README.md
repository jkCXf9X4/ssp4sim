# Documentation
<!-- Layer: 00-intent, 01-product -->

This page indexes the SSP4SIM documentation. See [breakdown/README.md](../breakdown/README.md) for the project introduction and layered breakdown.

## Product

- [Scope](../breakdown/01-product/scope.md) — what is in and out of scope
- [Capabilities](../breakdown/01-product/capabilities.md) — full feature list
- [Domain Model](../breakdown/01-product/domain-model.md) — core concepts and their relationships
- [Glossary](../breakdown/01-product/glossary.md) — shared vocabulary

## Setup

- [Installation](installation.md) — prebuilt release tarballs and wheels
- [Build From Source](build_from_source.md) — local and container builds

## Usage

- [Usage](usage.md) — CLI and Python invocation, example configs, output formats
- [Configuration](configuration.md) — JSON key reference, types, defaults
- [Choosing an Executor Algorithm](executor_choice.md) — what each executor does and how to pick one
- [Logging](logging_guidlines.md) — levels, hot-path guidance, sink setup
- [Profiling](profiling.md) — build and runtime profiling commands

## Development

- [Development Guide](development.md) — contributor workflow and conventions
- [Tests](../tests/README.md) — C++ and Python test layout and commands
- [Test Levels Analysis](test-levels-analysis.md) — coverage map and future implementation
- [Release Pipeline](linux_binary_distribution.md) — Linux binary and wheel packaging
