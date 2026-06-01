# SSP4SIM
<!-- Layer: 00-intent, 01-product, 05-operation, 03-implementation -->


SSP4SIM is a C++23 simulation engine for SSP archives with local database result artifacts and CSV export. See [product-breakdown/00-intent/purpose.md](product-breakdown/00-intent/purpose.md) for the full project purpose.

See [product-breakdown/01-product/capabilities.md](product-breakdown/01-product/capabilities.md) for the full capabilities list.

Builds upon the [SSP4CPP](https://github.com/jkCXf9X4/ssp4cpp) XML deserializer
and uses [Quill](https://github.com/odygrd/quill) for logging.

## Quick Start

Install the latest release (see [Installation](docs/installation.md)) or
[build from source](docs/build_from_source.md), then run:

```bash
sim_app ./resources/embrace/embrace.json
```

Or use the Python API:

```python
import pyssp4sim
sim = pyssp4sim.Simulator("./resources/embrace/embrace.json")
sim.init()
sim.simulate()
```

## Documentation

For structured per-layer documentation, see the [product-breakdown](product-breakdown/) directory.

- [Overview](docs/overview.md): short feature-oriented summary and positioning.
- [Installation](docs/installation.md): release tarball and wheel consumption.
- [Build from source](docs/build_from_source.md): local and container builds.
- [Usage](docs/usage.md): CLI, Python API, examples, and result artifacts.
- [Configuration](docs/configuration.md): JSON keys, defaults, and supported values.
- [Testing](tests/README.md): C++ and Python test layout and commands.
- [Development](docs/development.md): contributor workflow and repository conventions.
- [Release pipeline](docs/linux_binary_distribution.md): Linux binary and wheel packaging.
- [Profiling](docs/profiling.md): build and runtime profiling commands.
- [Logging](docs/logging_guidlines.md): logging tiers, hot-path logging, and sink notes.

## Contributing

Contributions are welcome. Development details are in [docs/development.md](docs/development.md).

## License

This project is released under the MIT license. See [LICENSE](LICENSE) for details.
