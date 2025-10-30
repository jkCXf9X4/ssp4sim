cmake_minimum_required(VERSION 3.10)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" projectVersion)
project(ssp4sim_prj VERSION ${projectVersion})
message("Current ssp4sim_prj version: ${projectVersion}\n")

set(CMAKE_CXX_STANDARD 23)

# OPTIONS
option(SSP4SIM_BUILD_TEST "Build test executable" OFF)
option(SSP4SIM_BUILD_PYTHON_API "Build python api" OFF)

# Use ccache if possible
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "" FORCE)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
else()
    message(STATUS "ccache not found; compiler launcher disabled")
endif()

## Link static
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++")

# Faster linker
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Prefer lld for faster link steps when supported
    message(STATUS "Using lld")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Skipping lld for GCC; GCC LTO requires the GNU linker plugin")
endif()


# COMPILER

# Optimize release builds
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto=auto -O3 -g -march=native -fno-omit-frame-pointer -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto=auto -O3 -g -march=native -fno-omit-frame-pointer -DNDEBUG" CACHE STRING "" FORCE)

# Debug builds keep symbols, no optimizations
set(CMAKE_CXX_FLAGS_DEBUG "-O3 -g" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG   "-O3 -g" CACHE STRING "" FORCE)

# Unsure how much this does
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -ffp-contract=fast -fno-math-errno -fno-trapping-math")


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(_LOG_)
endif()


# Status

message("CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}\n")
message("CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}\n")

message("CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}\n")
message("CMAKE_C_FLAGS_RELEASE: ${CMAKE_C_FLAGS_RELEASE}\n")

message("CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}\n")
message("CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}\n")

message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}\n")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}\n")
message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}\n")

# Subdirectories

add_subdirectory(3rdParty)

add_subdirectory(lib)

# Public artifacts
add_subdirectory(public/ssp4sim_app)

if (SSP4SIM_BUILD_PYTHON_API)
    add_subdirectory(public/python)
ENDIF ()

# Tests
if (SSP4SIM_BUILD_TEST)
    message("Building tests\n")
    add_subdirectory(tests)
ENDIF ()

