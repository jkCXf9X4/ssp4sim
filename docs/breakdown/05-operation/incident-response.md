---
id: INFO-033
type: info
title: Incident Response
summary: Triage steps for crash, hang, wrong results, and performance degradation, plus escalation
date: 2026-11-11
status: current
---

# Incident Response

Structured triage for common failure modes; step order matters within each procedure.

- **Simulation crash:**
  1. Collect the coredump or crash log.
  2. Check FMU version compatibility (FMI 2.0 co-simulation only).
  3. Reproduce with a minimal configuration (shorter time range, fewer FMUs).
  4. Check known issues on GitHub.
  5. Report with config, log output, SSP archive, expected versus actual behavior.
- **Simulation hang:**
  1. Check the log for the last successful simulation step.
  2. Check for recorder backpressure (rate-limited warnings in log).
  3. Check FMU timeout (reduce stop_time or increase timestep).
  4. Check thread configuration (reduce thread_pool_workers if oversubscribed).
  5. If reproducible, profile per the profiling leaf (`INFO-035`).
- **Wrong results:**
  1. Compare CSV output with a known reference (test resources).
  2. Check signal connections in the SSD (SSP System Structure Description).
  3. Verify start values match fixture expectations.
  4. Check for known xfails in the Python test markers.
- **Performance degradation:**
  1. Profile per the profiling leaf (`INFO-035`) with perf, valgrind, or heaptrack.
  2. Check thread-pool configuration (`executor.thread_pool_workers`).
  3. Check recording-sink throughput (backpressure warnings in log).
  4. Build with `SSP4SIM_LOG_HOT_PATH=OFF` for optimized runs.
  5. Compare with previous runs on similar hardware.
- **Escalation:**
  - Report via GitHub Issues with a minimal reproduction case, config file, log output, and SSP archive path.
  - For a known xfail, check whether the issue is already tracked.
