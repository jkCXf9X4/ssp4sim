find_package(Catch2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
include(Catch)

include(cmake/AddUnitTargets.cmake)

# Tests that legitimately depend on the full pipeline stay in the retained
# integration binary. Everything else is built as a standalone per-test
# executable from its own source cone (see AddUnitTargets.cmake).
set(SSP4SIM_INTEGRATION_TESTS
  ${CMAKE_CURRENT_SOURCE_DIR}/high_level/test_smoke_simulator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/test_data_recorder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/test_fmi4c_adapter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/test_sqlite_recorder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/graph/test_sim_graph_builder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/model/test_model_connection.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/scheduling/test_read_target_resolver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/simulation/test_sim_graph_builder.cpp
)

file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")
# NOTE: CONFIGURE_DEPENDS re-configures only when files are added/removed.
# Editing an `#include` inside an existing header does NOT retrigger it; the
# cone closure is recomputed only on the next explicit configure (or build).
# After adding/removing an include in a cone source, re-run `cmake --preset=vcpkg`.
list(REMOVE_ITEM ALL_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/kernel_main.cpp
)

set(SSP4SIM_UNIT_TESTS "")
foreach(TEST_SRC IN LISTS ALL_SRC)
  if(NOT TEST_SRC IN_LIST SSP4SIM_INTEGRATION_TESTS)
    add_unit_test(${TEST_SRC})
    list(APPEND SSP4SIM_UNIT_TESTS "${TEST_SRC}")
  endif()
endforeach()

# Configure-time consistency check for the cone machinery (unresolved local
# includes, per-test source counts). See AddUnitTargets.cmake.
ssp4sim_cone_audit(${SSP4SIM_UNIT_TESTS})

# --- Retained integration binary ---------------------------------------------------
add_executable(ssp4sim_tests test_main.cpp ${SSP4SIM_INTEGRATION_TESTS})

target_include_directories(ssp4sim_tests PRIVATE ${SSP4SIM_TEST_INCLUDE_DIRS})

target_link_libraries(ssp4sim_tests PUBLIC ssp4sim_lib)

# Catch2::Catch2, nlohmann_json and the Catch module were found above; the
# targets exist here by name.
target_link_libraries(ssp4sim_tests PRIVATE Catch2::Catch2)

target_link_libraries(ssp4sim_tests PRIVATE ssp4cpp::ssp4cpp)

target_link_libraries(ssp4sim_tests PUBLIC ssp4sim::3rd::fmi4c)

target_compile_definitions(ssp4sim_tests PRIVATE SSP4SIM_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

catch_discover_tests(ssp4sim_tests)