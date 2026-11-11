---
title: Use Cases
summary: User-visible scenarios the product must support, one per leaf
---

# Use Cases

## Contents

<!-- pb:index:start -->
<!-- GENERATED FILE — do not edit. Regenerate with `pb registers`. -->
- **UC-001** [UC-001: Run a Co-Simulation from an SSP Archive](UC-001.md) — A user runs a complete co-simulation from an SSP archive
- **UC-002** [UC-002: Compare Execution Strategy Results](UC-002.md) — A developer runs the same SSP with different execution strategies to compare results and performance
- **UC-003** [UC-003: Embed the Engine in a Python Workflow](UC-003.md) — A user integrates simulation results into a Python analysis pipeline
- **UC-004** [UC-004: Record Results to CSV for External Analysis](UC-004.md) — A user records simulation results to CSV and processes them with external tools
- **UC-005** [UC-005: Query Local Result Database for Viewer and Run Comparison](UC-005.md) — A user records results to a local database so a viewer or comparison tool can query typed time-series data
- **UC-006** [UC-006: Ingest Results into a Remote Database](UC-006.md) — A user sends simulation results to a remote database shared across producers
- **UC-007** [UC-007: Review Simple Overview Logs During a Run](UC-007.md) — An operator watches terminal and plain-text file logs to confirm progress and diagnose a simple failure
- **UC-008** [UC-008: Analyze Full JSON Logs After a Run](UC-008.md) — A developer inspects the full structured JSON log file after a simulation to search, filter, and correlate runtime events
- **UC-009** [UC-009: Navigate Live Structured Logs in Cutelog](UC-009.md) — An operator watches a live structured log stream to jump through a running simulation without scanning raw text
- **UC-010** [UC-010: Correlate Distributed Simulation Behavior with OpenTelemetry](UC-010.md) — A developer or operator inspects a distributed simulation run through traces to understand cross-process timing and causality
- **UC-011** [UC-011: Pause, Resume, and Stop a Simulation](UC-011.md) — A user pauses, resumes, or stops a running simulation to control execution interactively
- **UC-012** [UC-012: Adjust Simulation Speed](UC-012.md) — A user adjusts the real-time speed of a running simulation relative to wall-clock time
- **UC-013** [UC-013: Inject a Direct Variable Override During Simulation](UC-013.md) — A user injects a fixed value override on a specified variable, immediately or at a scheduled simulation time
- **UC-014** [UC-014: Inject a Phase-Shifted Variable Signal During Simulation](UC-014.md) — A user injects a phase-shifted version of a variable's signal, immediately or at a scheduled simulation time
- **UC-015** [UC-015: Inject a Ramp/Linear Variable Signal During Simulation](UC-015.md) — A user injects a ramp signal kx + m on a variable, immediately or at a scheduled simulation time
- **UC-016** [UC-016: Schedule Simulation Lifecycle Operations with Tight Timing](UC-016.md) — A user schedules lifecycle operations at specific simulation times for a pre-planned run
- **UC-017** [UC-017: Embed the Engine via the C API](UC-017.md) — A C or C++ host application embeds the engine and drives it from its own lifecycle
- **UC-018** [UC-018: Run a Simulation Paced to Wall Clock](UC-018.md) — An operator runs a simulation paced to wall-clock time for a demonstration or operator-facing run
- **UC-019** [UC-019: Experiment with Executor Variants and Tuning](UC-019.md) — A developer runs the same SSP with different executor variants and tuning to study coupling, loop handling, and performance
- **UC-020** [UC-020: Record Inputs and Start Values for Analysis](UC-020.md) — A user records input signals alongside outputs and captures the applied initial start values
- **UC-021** [UC-021: Simulate a Parameterized SSP Scenario](UC-021.md) — A user runs a simulation whose parameterization is defined by the SSP and verifies those values reach the FMUs
- **UC-022** [UC-022: Produce Distinct Timestamped Artifacts per Run](UC-022.md) — A user running repeated simulations wants each run's artifacts to land in predictable, non-clobbering locations
- **UC-023** [UC-023: Run a Connected System Without Manual Ordering Configuration](UC-023.md) — A developer runs a multi-model SSP with interconnections and loops without specifying execution order or wiring manually
- **UC-024** [UC-024: Capture FMU-internal Log Output During a Run](UC-024.md) — A developer enables FMU-level logging so messages emitted by the FMU implementation are forwarded into the configured log sinks
- **UC-025** [UC-025: Run Concurrent Simulations for Parameter Sweeps](UC-025.md) — A user runs multiple simulations concurrently as separate processes with per-run result artifacts that do not collide
<!-- pb:index:end -->
