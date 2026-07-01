file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")

add_executable(ssp4sim_tests ${ALL_SRC})

target_include_directories(
  ssp4sim_tests
  PRIVATE ${CMAKE_SOURCE_DIR}/lib/include
          ${CMAKE_SOURCE_DIR}/lib/include/pre
          ${CMAKE_SOURCE_DIR}/lib/include/pre/1_ssp_parser
          ${CMAKE_SOURCE_DIR}/lib/include/pre/1_ssp_parser/elements
          ${CMAKE_SOURCE_DIR}/lib/include/pre/1_ssp_parser/schema_extensions
          ${CMAKE_SOURCE_DIR}/lib/include/pre/2_analysis
          ${CMAKE_SOURCE_DIR}/lib/include/pre/2_analysis/elements
          ${CMAKE_SOURCE_DIR}/lib/include/pre/3_simulation
          ${CMAKE_SOURCE_DIR}/lib/include/pre/3_simulation/elements
          ${CMAKE_SOURCE_DIR}/lib/include/simulation
          ${CMAKE_SOURCE_DIR}/lib/include/simulation/graph_executor
          ${CMAKE_SOURCE_DIR}/lib/include/simulation/graph_executor/execution
          ${CMAKE_SOURCE_DIR}/lib/include/simulation/signal
          ${CMAKE_SOURCE_DIR}/lib/include/simulation/signal/sinks
          ${CMAKE_SOURCE_DIR}/lib/include/utils
          ${CMAKE_SOURCE_DIR}/lib/include/utils/fmi
          ${CMAKE_SOURCE_DIR}/lib/include/utils/graph
          ${CMAKE_SOURCE_DIR}/lib/include/utils/io
          ${CMAKE_SOURCE_DIR}/lib/include/utils/memory
          ${CMAKE_SOURCE_DIR}/lib/include/utils/primitives
          ${CMAKE_SOURCE_DIR}/lib/include/utils/thread_pool
          ${CMAKE_SOURCE_DIR}/lib/include/utils/time
          ${CMAKE_SOURCE_DIR}/lib/public_include)

target_link_libraries(ssp4sim_tests PUBLIC ssp4sim_lib)

find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(ssp4sim_tests PRIVATE Catch2::Catch2)

target_link_libraries(ssp4sim_tests PRIVATE ssp4cpp::ssp4cpp)

target_link_libraries(ssp4sim_tests PUBLIC ssp4sim::3rd::fmi4c)

target_compile_definitions(ssp4sim_tests PRIVATE SSP4SIM_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

include(CTest)

enable_testing()
include(Catch)
catch_discover_tests(ssp4sim_tests)
