# Logging Guidelines

SSP4SIM uses Quill through the shared `ssp4cpp::utils::log` wrapper. The
wrapper owns backend startup and logger construction; do not call
`quill::simple_logger()` directly.

The filename is intentionally left as `logging_guidlines.md` for now because
other docs link to it.

## Levels

Use the lowest level that still makes the event useful:

| Level | Use for |
|---|---|
| `TRACE_L2` | Extremely detailed diagnostics such as variable values inside tight loops. |
| `TRACE_L1` | Step-by-step execution tracing when debugging a specific path. |
| `DEBUG` | Developer-focused lifecycle details, configuration, state transitions, and branch decisions. |
| `INFO` | High-level application events such as startup, shutdown, selected scenario, and completed simulation. |
| `WARNING` | Unexpected conditions where simulation can continue, including dropped recorder events or questionable configuration. |
| `ERROR` | Failures that prevent the current operation from completing. |
| `CRITICAL` | Unrecoverable state, startup failure, or process-ending failure. |

## Hot Paths

Avoid normal `INFO`, `WARNING`, or high-volume `DEBUG` logging inside step,
substep, executor, thread-pool, recorder, and signal-storage inner loops.

Use `IF_LOG({ ... })` around detailed execution tracing so it can be compiled
out with:

```bash
cmake --preset=vcpkg -DSSP4SIM_LOG_HOT_PATH=OFF
```

Enable hot-path logging only for focused diagnosis:

```bash
cmake --preset=vcpkg -DSSP4SIM_LOG_HOT_PATH=ON
cmake --build build
```

Quill queue growth messages usually mean frontend threads are producing logs
faster than the backend can drain them. Reduce hot-path log volume before
tuning backend settings.

## Logger Construction

- Store logger pointers as `ssp4cpp::utils::log::Logger*`.
- Prefer constructing logger members in constructors after logging has been
  configured.
- Avoid header-initialized logger members when sinks are configured at runtime.
- Use the existing `LOG_*` macros and local patterns near the changed code.

## Message Content

Good log messages should be concise and include the context needed to diagnose
the event later:

```cpp
LOG_WARNING(log, "[{func}] Event queue full for storage {}", __func__, storage->name);
```

Avoid:

- Sensitive data, tokens, credentials, or personal information.
- Repeated per-signal messages at normal log levels.
- Generic messages such as `failed` without the operation and relevant object
  name.

## Cutelog Sink

`ssp4cpp::utils::log::add_cutelog_sink(host, port, level)` sends
length-prefixed JSON records to a cutelog TCP listener. Start cutelog first,
then configure this sink before constructing loggers.

## Current Backend Profile

The current Quill profile favors lower frontend latency by:

- Preallocating the thread-local queue before the first log event.
- Running the backend in busy-poll mode with `sleep_duration=0` and
  `enable_yield_when_idle=true`.
- Disabling timestamp grace-period ordering.
- Flushing sinks less often.

This improves throughput on heavily logged execution paths at the cost of
higher backend CPU usage.
