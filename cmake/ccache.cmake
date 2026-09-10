# Toolchain integration: wire ccache into the build as the compiler launcher.
#
# This file is loaded as `CMAKE_PROJECT_INCLUDE` by the vcpkg preset (it is
# automatically included after the vcpkg toolchain). The launcher is applied to
# every target (all languages, all subprojects including 3rdParty/ssp4cpp).
#
# ccache is ON BY DEFAULT for local builds: whenever the ccache binary is found
# on PATH it is wired in, which makes the per-test unit binaries (they recompile
# shared cone sources repeatedly) and clean rebuilds substantially faster.
# Disable with `-DCCACHE=OFF`. Without ccache installed, plain builds keep
# working.
#
# The cache lives in ccache's default location (`~/.cache/ccache`) or wherever
# CCACHE_DIR points. The container helpers mount the host store so container
# and CI builds start warm.

find_program(_SSP4SIM_CCACHE ccache)

option(CCACHE "Use ccache as the compiler launcher" ON)

if(CCACHE AND _SSP4SIM_CCACHE)
  set(CMAKE_C_COMPILER_LAUNCHER   "${_SSP4SIM_CCACHE}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${_SSP4SIM_CCACHE}")
  message(STATUS "ccache enabled: ${_SSP4SIM_CCACHE} (default; -DCCACHE=OFF disables)")
elseif(CCACHE AND NOT _SSP4SIM_CCACHE)
  message(STATUS "ccache not found on PATH; building without it (install ccache, or pass -DCCACHE=OFF to silence)")
else()
  message(STATUS "ccache integration disabled (-DCCACHE=OFF)")
endif()