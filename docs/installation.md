# Installation
<!-- Layer: 05-operation -->


This guide covers ways to install or consume `ssp4sim` without developing the
library itself. For source builds, use [Build From Source](build_from_source.md).

## Install Linux Release Binaries

Tagged releases (`v*`) publish dual artifacts:

- Linux bundle: `ssp4sim-linux-x86_64-vX.Y.Z.tar.gz`
- Python wheel: `pyssp4sim-X.Y.Z-*.whl`
- Linux alias: `ssp4sim-linux-x86_64-latest.tar.gz`
- Wheel alias: `pyssp4sim-0.0.0+lat-cp39-abi3-linux_x86_64.whl`

The Linux tarball contains:

- `bin/sim_app`
- `python/pyssp4sim` (Python API package with native extension)
- [`readme.md`](../readme.md), [`LICENSE`](../LICENSE), `RELEASE.txt`

Download and verify the latest release:

https://github.com/jkCXf9X4/ssp4sim/releases/latest

```bash
curl -fLO \
  https://github.com/jkCXf9X4/ssp4sim/releases/latest/download/ssp4sim-linux-x86_64-latest.tar.gz

curl -fLO \
  https://github.com/jkCXf9X4/ssp4sim/releases/latest/download/SHA256SUMS

sha256sum -c SHA256SUMS

mkdir -p $HOME/.local/opt/ssp4sim
tar -xzf ssp4sim-linux-x86_64-latest.tar.gz -C $HOME/.local/opt
```

Add the CLI app to `PATH`:

```bash
export PATH="$HOME/.local/opt/ssp4sim/bin:$PATH"
```

Verify the installed CLI version:

```bash
sim_app --version
```

Run an example simulation:

```bash
sim_app $HOME/.local/opt/ssp4sim/resources/embrace/embrace.json
```

Usage details are in [Usage](usage.md).

Runtime notes:

- Releases are currently built on Ubuntu 22.04.
- The binary links `libstdc++` statically, but system compatibility can still
  depend on `glibc` and other runtime components.

## Install Python API From Release Wheel

```bash
pip install \
  https://github.com/jkCXf9X4/ssp4sim/releases/latest/download/pyssp4sim-0.0.0+lat-cp39-abi3-linux_x86_64.whl
```

This alias wheel uses a constant version and can be cached by `pip`. For upgrades, use:

```bash
pip install --upgrade --force-reinstall --no-cache-dir \
  https://github.com/jkCXf9X4/ssp4sim/releases/latest/download/pyssp4sim-0.0.0+lat-cp39-abi3-linux_x86_64.whl
```

Verify the package imports:

```bash
python -c "import pyssp4sim; print(pyssp4sim)"
```

Python usage details are in [Usage](usage.md). For source development setup and
build commands, continue with [Development Guide](development.md).
