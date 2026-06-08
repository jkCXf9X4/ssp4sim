file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")

add_executable(ssp4sim_tests ${ALL_SRC})

target_include_directories(
  ssp4sim_tests
  PRIVATE ${CMAKE_SOURCE_DIR}/lib/include
          ${CMAKE_SOURCE_DIR}/lib/include/analysis
          ${CMAKE_SOURCE_DIR}/lib/include/execution
          ${CMAKE_SOURCE_DIR}/lib/include/graph
          ${CMAKE_SOURCE_DIR}/lib/include/graph/analysis
          ${CMAKE_SOURCE_DIR}/lib/include/handler
          ${CMAKE_SOURCE_DIR}/lib/include/model
          ${CMAKE_SOURCE_DIR}/lib/include/schema_extensions
          ${CMAKE_SOURCE_DIR}/lib/include/signal
          ${CMAKE_SOURCE_DIR}/lib/include/utils
          ${CMAKE_SOURCE_DIR}/lib/include/utils/thread_pool
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
