
## High-Level SSP Tests

The C++ high-level reference sweep lives in `tests/lib/high_level/`.

Run it with:

```bash
./build/tests/lib/ssp4sim_tests "[references]"
```

The test iterates the unpacked SSP fixtures under
`tests/reference_ssp/build/models/*/ssp` and simulates each co-simulation SSP.
Model-exchange fixtures are excluded because the current runtime only supports
co-simulation FMUs.
