# Incident Response
<!-- Layer: 05-operation -->
<!-- Stable ID: OPS-INCIDENT-001 -->

## Description

This artifact provides structured triage steps for common SSP4SIM failure modes.

## Simulation Crash

1. Collect coredump or crash log
2. Check FMU version compatibility (FMI 2.0 co-simulation only)
3. Reproduce with minimal configuration (shorter time range, fewer FMUs)
4. Check for known issues in GitHub Issues
5. Report with: config, log output, SSP archive, expected vs. actual behavior

## Simulation Hang

1. Check log for the last successful simulation step
2. Check for recorder backpressure (rate-limited warnings in log)
3. Check FMU timeout (reduce stop_time or increase timestep)
4. Check thread configuration (reduce thread_pool_workers if oversubscribed)
5. If reproducible, profile per `docs/profiling.md`

## Wrong Results

1. Compare CSV output with known reference (test resources)
2. Check signal connections in SSD (SSP System Structure Description)
3. Verify start values match fixture expectations
4. Check for known xfails in `tests/python/high_level/` test markers

## Performance Degradation

1. Profile per `docs/profiling.md` (perf, valgrind, heaptrack)
2. Check thread pool configuration (`executor.thread_pool_workers`)
3. Check recording sink throughput (backpressure warnings in log)
4. Build with `SSP4SIM_LOG_HOT_PATH=OFF` for optimized runs
5. Compare with previous runs on similar hardware

## Escalation

- Report issues via GitHub Issues
- Include: minimal reproduction case, config file, log output, SSP archive path
- For known xfails, check if the issue is already tracked

## Traceability

- Backward: Monitoring in `product-breakdown/05-operation/monitoring.md`.
- Sources: `docs/usage.md`, `docs/profiling.md`, `docs/logging_guidlines.md`.
