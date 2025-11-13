cmake_minimum_required(VERSION 3.10)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" projectVersion)
project(ssp4sim_prj VERSION ${projectVersion})
message("Current ssp4sim_prj version: ${projectVersion}\n")

set(CMAKE_CXX_STANDARD 23)

# OPTIONS
option(SSP4SIM_BUILD_TEST "Build test executable" OFF)
option(SSP4SIM_BUILD_PYTHON_API "Build python api" OFF)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Link static
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++")

# Debug builds keep symbols, no optimizations
# Dont use this... its not really faster
if(CMAKE_BUILD_TYPE STREQUAL "Debug")

  message("Building Debug!\n")

  set(CMAKE_CXX_FLAGS_DEBUG
      "-O1 -fno-lto -g"
      CACHE STRING "" FORCE)
  # -ftime-report
  set(CMAKE_C_FLAGS_DEBUG
      "-O1 -fno-lto -g"
      CACHE STRING "" FORCE)

  # Faster linker lld dos not work with flto
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(STATUS "Using lld for Debug builds")
    foreach(var IN
            ITEMS CMAKE_EXE_LINKER_FLAGS_DEBUG CMAKE_SHARED_LINKER_FLAGS_DEBUG
                  CMAKE_MODULE_LINKER_FLAGS_DEBUG)
      set(${var} "${${var}} -fuse-ld=lld")
    endforeach()
  endif()

  # Use ccache if possible
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER
        "${CCACHE_PROGRAM}"
        CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER_LAUNCHER
        "${CCACHE_PROGRAM}"
        CACHE STRING "" FORCE)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
  else()
    message(STATUS "ccache not found; compiler launcher disabled")
  endif()

  add_compile_definitions(_LOG_)
endif()

# Optimize release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message("Building Release!\n")

  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} -flto=auto -O3 -g -march=native -fno-omit-frame-pointer -DNDEBUG"
      CACHE STRING "" FORCE)
  set(CMAKE_C_FLAGS_RELEASE
      "${CMAKE_C_FLAGS_RELEASE} -flto=auto -O3 -g -march=native -fno-omit-frame-pointer -DNDEBUG"
      CACHE STRING "" FORCE)

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

if(SSP4SIM_BUILD_PYTHON_API)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Python build must be Release builds, no build will be generated")
    message("Something with debug symbols...")
  else()
    add_subdirectory(public/python_api)
  endif()
endif()

# Tests
if(SSP4SIM_BUILD_TEST)
  message("Building tests\n")
  add_subdirectory(tests)
endif()
