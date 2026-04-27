
## High-Level SSP Tests

The dedicated pytest suite for the checked-in SSP fixtures lives under
`tests/high_level/`.

Run it from the nested repository root:

```bash
cd tests/reference_ssp
pytest
```

The suite validates both archive structure and a Python API smoke test for the
unpacked SSP directories that are currently known to initialize through
`pyssp4sim`.
