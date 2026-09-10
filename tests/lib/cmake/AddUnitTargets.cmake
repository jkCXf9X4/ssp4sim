# Build-time isolation for C++ library tests.
#
# Each unit test (<basename>.cpp) is compiled into its own executable together
# with the transitive include cone of its implementation sources. A test's
# executable therefore only builds the code it actually exercises; a broken
# implementation file elsewhere in lib/ no longer blocks kernel/parser/graph
# tests from compiling and linking.
#
# The cone rule relies on one invariant: implementation .cpp files sit adjacent
# to their .hpp under lib/include/** (normalized layout). Include resolution is
# soft: an include that cannot be resolved against the include set is treated as
# an external/system header and skipped. A wrong guess manifests as a per-binary
# link-time undefined reference, never a silent wrong cone.

include_guard(GLOBAL)

# Include search set shared with ssp4sim_lib and the retained integration binary.
set(SSP4SIM_TEST_INCLUDE_DIRS
  "${CMAKE_SOURCE_DIR}/lib/include"
  "${CMAKE_SOURCE_DIR}/lib/include/pre"
  "${CMAKE_SOURCE_DIR}/lib/include/pre/1_ssp_parser"
  "${CMAKE_SOURCE_DIR}/lib/include/pre/1_ssp_parser/elements"
  "${CMAKE_SOURCE_DIR}/lib/include/pre/1_ssp_parser/schema_extensions"
  "${CMAKE_SOURCE_DIR}/lib/include/pre/2_analysis"
  "${CMAKE_SOURCE_DIR}/lib/include/pre/2_analysis/elements"
  "${CMAKE_SOURCE_DIR}/lib/include/pre/3_simulation"
  "${CMAKE_SOURCE_DIR}/lib/include/pre/3_simulation/elements"
  "${CMAKE_SOURCE_DIR}/lib/include/simulation"
  "${CMAKE_SOURCE_DIR}/lib/include/simulation/graph_executor"
  "${CMAKE_SOURCE_DIR}/lib/include/simulation/graph_executor/execution"
  "${CMAKE_SOURCE_DIR}/lib/include/simulation/signal"
  "${CMAKE_SOURCE_DIR}/lib/include/simulation/signal/sinks"
  "${CMAKE_SOURCE_DIR}/lib/include/utils"
  "${CMAKE_SOURCE_DIR}/lib/include/utils/fmi"
  "${CMAKE_SOURCE_DIR}/lib/include/utils/graph"
  "${CMAKE_SOURCE_DIR}/lib/include/utils/io"
  "${CMAKE_SOURCE_DIR}/lib/include/utils/memory"
  "${CMAKE_SOURCE_DIR}/lib/include/utils/primitives"
  "${CMAKE_SOURCE_DIR}/lib/include/utils/thread_pool"
  "${CMAKE_SOURCE_DIR}/lib/include/utils/time"
  "${CMAKE_SOURCE_DIR}/lib/public_include"
)

# Remove C block comments (/* ... */), preserving newlines so the remaining
# text keeps its original line structure. This is what makes the include scan
# comment-safe: a `#include` directive sitting at line start *inside* a block
# comment must not enter a cone (matching this } would pull in a wrong sibling
# .cpp, which is a silent defect until a link error).
function(ssp4sim_strip_block_comments out_var content)
  set(_working "${content}")
  while(1)
    string(FIND "${_working}" "/*" _start)
    if(_start EQUAL -1)
      break()
    endif()
    string(SUBSTRING "${_working}" 0 ${_start} _prefix)
    string(SUBSTRING "${_working}" ${_start} -1 _tail)
    string(FIND "${_tail}" "*/" _rel_end)
    if(_rel_end EQUAL -1)
      # Unterminated block comment: the rest of the file is comment.
      string(REGEX MATCHALL "\n" _newlines "${_tail}")
      list(LENGTH _newlines _nl_count)
      if(_nl_count GREATER 0)
        string(REPEAT "\n" ${_nl_count} _pad)
      endif()
      set(_working "${_prefix}${_pad}")
      break()
    endif()
    math(EXPR _len "${_rel_end} + 2")
    math(EXPR _end "${_start} + ${_len}")
    string(SUBSTRING "${_working}" ${_end} -1 _suffix)
    string(SUBSTRING "${_working}" ${_start} ${_len} _removed)
    string(REGEX MATCHALL "\n" _newlines "${_removed}")
    list(LENGTH _newlines _nl_count)
    if(_nl_count GREATER 0)
      string(REPEAT "\n" ${_nl_count} _pad)
    else()
      set(_pad "")
    endif()
    set(_working "${_prefix}${_pad}${_suffix}")
  endwhile()
  set(${out_var} "${_working}" PARENT_SCOPE)
endfunction()

# Extract the targets of `#include "..."` and `#include <...>` directives from
# a source file. Block comments are stripped first (see above); line comments
# are excluded by requiring the directive to sit at line start. Angle-bracket
# targets are retained so local headers included that way (e.g.
# <utils/primitives/node.hpp>) enter the cone; system/external headers simply
# fail to resolve and are dropped.
function(ssp4sim_parse_quoted_includes out_var file_path)
  file(READ "${file_path}" _content)
  string(PREPEND _content "\n")
  ssp4sim_strip_block_comments(_content "${_content}")
  string(REGEX MATCHALL "[\n\r][ \t]*#[ \t]*include[ \t]+[\"<][^\"<>]+[\">]" _matches "${_content}")
  set(_incs "")
  foreach(_m IN LISTS _matches)
    string(REGEX REPLACE "^[\n\r][ \t]*#[ \t]*include[ \t]+[\"<]" "" _name "${_m}")
    string(REGEX REPLACE "[\">]$" "" _name "${_name}")
    list(APPEND _incs "${_name}")
  endforeach()
  set(${out_var} "${_incs}" PARENT_SCOPE)
endfunction()

# Resolve a quoted include against the including file's directory, then against
# the include set. Returns empty when unresolvable (external/system header).
function(ssp4sim_resolve_include out_var spec current_dir)
  if(EXISTS "${current_dir}/${spec}")
    set(${out_var} "${current_dir}/${spec}" PARENT_SCOPE)
    return()
  endif()
  foreach(_dir IN LISTS SSP4SIM_TEST_INCLUDE_DIRS)
    if(EXISTS "${_dir}/${spec}")
      set(${out_var} "${_dir}/${spec}" PARENT_SCOPE)
      return()
    endif()
  endforeach()
  set(${out_var} "" PARENT_SCOPE)
endfunction()

# Map a header to its same-name sibling implementation file; empty if header-only.
function(ssp4sim_sibling_cpp out_var hpp_path)
  set(_cpp "${hpp_path}")
  string(REGEX REPLACE "\\.hpp$" ".cpp" _cpp "${_cpp}")
  if(NOT EXISTS "${_cpp}")
    set(_cpp "")
  endif()
  set(${out_var} "${_cpp}" PARENT_SCOPE)
endfunction()

# Compute the transitive include cone of test_cpp.
# Returns the ordered list of test file + headers + implementation sources.
# When unresolved_out is given, it receives the list of *local-looking* include
# specs that failed to resolve (drift guard: a new lib directory that is not in
# SSP4SIM_TEST_INCLUDE_DIRS surfaces here instead of silently shrinking a cone).
# The third output is optional (CMake 3.22 rejects fewer-than-declared args, so
# it is passed/omitted via ARGC rather than a mandatory parameter).
function(ssp4sim_compute_cone out_var test_cpp)
  set(_unresolved_out "")
  if(ARGC GREATER 2)
    set(_unresolved_out "${ARGV2}")
  endif()
  set(_visited "${test_cpp}")
  set(_frontier "${test_cpp}")
  set(_unresolved_local "")
  while(_frontier)
    set(_next "")
    foreach(_file IN LISTS _frontier)
      get_filename_component(_file_abs "${_file}" ABSOLUTE)
      get_filename_component(_dir "${_file_abs}" DIRECTORY)
      ssp4sim_parse_quoted_includes(_incs "${_file_abs}")
      foreach(_spec IN LISTS _incs)
        ssp4sim_resolve_include(_hit "${_spec}" "${_dir}")
        if(NOT _hit)
          if(_unresolved_out)
            set(_local FALSE)
            if(_spec MATCHES "^(pre|utils|simulation|scheduling|1_ssp_parser|2_analysis|3_simulation)/")
              set(_local TRUE)
            endif()
            if(_spec MATCHES "^ssp4sim_definitions\\.hpp$")
              set(_local TRUE)
            endif()
            if(_spec MATCHES "^(shared_config|simulator|simulator_c_api)\\.(hpp|h)$")
              set(_local TRUE)
            endif()
            if(_local)
              list(APPEND _unresolved_local "${_spec}")
            endif()
          endif()
          continue()
        endif()
        get_filename_component(_hit "${_hit}" ABSOLUTE)
        if(NOT "${_hit}" IN_LIST _visited)
          list(APPEND _visited "${_hit}")
          list(APPEND _next "${_hit}")
        endif()
        ssp4sim_sibling_cpp(_cpp "${_hit}")
        if(_cpp)
          get_filename_component(_cpp "${_cpp}" ABSOLUTE)
          if(NOT "${_cpp}" IN_LIST _visited)
            list(APPEND _visited "${_cpp}")
            list(APPEND _next "${_cpp}")
          endif()
        endif()
      endforeach()
    endforeach()
    set(_frontier "${_next}")
  endwhile()
  set(${out_var} "${_visited}" PARENT_SCOPE)
  if(_unresolved_out)
    set(${_unresolved_out} "${_unresolved_local}" PARENT_SCOPE)
  endif()
endfunction()

# Configure-time consistency check for the per-test machinery. Emits a WARNING
# for every local include that a unit cone cannot resolve (the classic stale-
# cone / missing-directory drift) and a STATUS line summarizing the healthy case
# so CI has a single visible audit marker.
function(ssp4sim_cone_audit)
  set(_problems 0)
  set(_lib_sources 0)
  set(_test_count 0)
  foreach(_test IN LISTS ARGN)
    get_filename_component(_name "${_test}" NAME_WE)
    ssp4sim_compute_cone(_cone "${_test}" _unresolved)
    math(EXPR _test_count "${_test_count} + 1")
    foreach(_f IN LISTS _cone)
      if(_f MATCHES "\\.cpp$" AND _f MATCHES "/lib/include/")
        math(EXPR _lib_sources "${_lib_sources} + 1")
      endif()
    endforeach()
    foreach(_u IN LISTS _unresolved)
      message(WARNING
        "[cone audit] ${_name}: unresolvable local include '${_u}'. "
        "Directory is missing from SSP4SIM_TEST_INCLUDE_DIRS or the include is misspelled; "
        "this cone may be silently smaller than intended.")
      math(EXPR _problems "${_problems} + 1")
    endforeach()
  endforeach()
  if(_problems GREATER 0)
    message(WARNING "[cone audit] ${_problems} unresolved local include(s) across ${_test_count} unit cones. See messages above.")
  else()
    message(STATUS "[cone audit] OK: ${_test_count} unit cones, ${_lib_sources} lib sources compiled into standalone binaries, no unresolvable local includes.")
  endif()
endfunction()

# Add a per-test executable: <basename> = stem of <cpp>, built from its cone.
function(add_unit_test test_cpp)
  get_filename_component(_name "${test_cpp}" NAME_WE)
  if(TARGET "${_name}")
    message(FATAL_ERROR "add_unit_test: target name collision '${_name}' from ${test_cpp}")
  endif()

  ssp4sim_compute_cone(_cone "${test_cpp}")

  set(_sources "${CMAKE_SOURCE_DIR}/tests/lib/kernel_main.cpp" "${test_cpp}")
  foreach(_f IN LISTS _cone)
    if(NOT _f STREQUAL "${test_cpp}")
      list(APPEND _sources "${_f}")
    endif()
  endforeach()

  add_executable(${_name} ${_sources})

  # NOTE: LTO cannot be disabled per-target. ssp4cpp (linked into every unit
  # binary) is compiled with -flto=auto by its own CMakeLists, so the final link
  # must invoke the LTO plugin to resolve those objects; a -fno-lto LINK_FLAGS
  # here produces "plugin needed to handle lto object" failures. The clean-build
  # cost of repeated -O3 -flto work is accepted (see test-fragment-plan.md).
  target_include_directories(${_name} PRIVATE ${SSP4SIM_TEST_INCLUDE_DIRS})
  target_compile_definitions(${_name} PRIVATE SSP4SIM_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

  # Catch2 / nlohmann_json / Catch are found at the tests/lib scope (which runs
  # before the unit targets are added), so their targets resolve here by name.
  target_link_libraries(${_name} PRIVATE Catch2::Catch2)
  target_link_libraries(${_name} PRIVATE nlohmann_json::nlohmann_json)

  # ssp4cpp links quill PUBLIC, so this also brings logging in transitively.
  target_link_libraries(${_name} PRIVATE ssp4cpp::ssp4cpp)

  catch_discover_tests(${_name})
endfunction()