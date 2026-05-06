# Profiling

This page collects repeatable profiling commands for build-time and runtime
investigation. Prefer storing generated traces under `build/` so they stay out
of source-controlled documentation and fixtures.

## Build Profiling

Generate a Ninja build trace:

```bash
cmake --preset=vcpkg
cmake --build build
ninjatracing build/.ninja_log > build/trace.json
```

Open `build/trace.json` in a Chromium-based browser through
`chrome://tracing` or another trace viewer.

Collect Ninja build stats and verbose command logs:

```bash
ninja -C build clean
CCACHE_DISABLE=1 ninja -C build -d stats > build/build-stats.log
CCACHE_DISABLE=1 ninja -C build -v > build/commands.log
```

Inspect the generated Ninja graph:

```bash
ninja -C build -t browse
```

## Runtime Profiling With `perf`

Linux `perf` may require a less restrictive `perf_event_paranoid` setting:

```bash
sudo sysctl kernel.perf_event_paranoid=1
```

Build and record a sample profile:

```bash
cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Release
cmake --build build
perf record -F 99 -g -o build/perf.data \
  ./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace_profiling.json
```

Review the profile:

```bash
perf report -i build/perf.data --sort comm,dso,symbol
perf report --dsos=sim_app -i build/perf.data --sort comm,dso,symbol

```

For recorder or executor investigations, run once with
`-DSSP4SIM_LOG_HOT_PATH=OFF` and once with `-DSSP4SIM_LOG_HOT_PATH=ON` so log
overhead is visible in the profile instead of hidden in the measurement.

## Output Hygiene

Common profiling outputs are generated artifacts:

- `build/trace.json`
- `build/build-stats.log`
- `build/commands.log`
- `build/perf.data`

Do not commit these files unless a task explicitly asks for a captured
performance artifact.
