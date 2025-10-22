
option(FMI4C_BUILD_SHARED OFF)
option(FMI4C_USE_INCLUDED_ZLIB ON)

add_subdirectory(fmi4c)
add_library(ssp4sim::3rd::fmi4c ALIAS fmi4c)

set(SSP4CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CUTECPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_subdirectory(ssp4cpp)
