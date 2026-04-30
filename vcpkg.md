# vcpkg Setup

The repository CMake preset expects `VCPKG_ROOT` to point to a vcpkg checkout:

```bash
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT="$HOME/vcpkg"
```

Configure SSP4SIM with the preset:

```bash
cmake --preset=vcpkg
```

The project uses manifest mode through `vcpkg.json` for normal builds.

## Local `ssp4cpp` Overlay

NOT TESTED

If you need to install or test the local `ssp4cpp` overlay manually:

```bash
vcpkg install ssp4cpp --classic --overlay-ports=./3rdParty/ssp4cpp/ports/ssp4cpp/
```

Normal SSP4SIM configure/build commands should not need this manual install
step.
