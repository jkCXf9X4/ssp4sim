file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")

# message(ALL_SRC ssp4sim_lib ="${ALL_SRC}")

add_library(ssp4sim_lib STATIC ${ALL_SRC})

target_include_directories(
  ssp4sim_lib
  PUBLIC ${CMAKE_CURRENT_LIST_DIR}/public_include

  PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include
          ${CMAKE_CURRENT_LIST_DIR}/include/pre
          ${CMAKE_CURRENT_LIST_DIR}/include/pre/1_ssp_parser
          ${CMAKE_CURRENT_LIST_DIR}/include/pre/1_ssp_parser/elements
          ${CMAKE_CURRENT_LIST_DIR}/include/pre/1_ssp_parser/schema_extensions
          ${CMAKE_CURRENT_LIST_DIR}/include/pre/2_analysis
          ${CMAKE_CURRENT_LIST_DIR}/include/pre/2_analysis/elements
          ${CMAKE_CURRENT_LIST_DIR}/include/pre/3_simulation
          ${CMAKE_CURRENT_LIST_DIR}/include/pre/3_simulation/elements
          ${CMAKE_CURRENT_LIST_DIR}/include/utils
          ${CMAKE_CURRENT_LIST_DIR}/include/utils/fmi
          ${CMAKE_CURRENT_LIST_DIR}/include/utils/graph
          ${CMAKE_CURRENT_LIST_DIR}/include/utils/io
          ${CMAKE_CURRENT_LIST_DIR}/include/utils/memory
          ${CMAKE_CURRENT_LIST_DIR}/include/utils/primitives
          ${CMAKE_CURRENT_LIST_DIR}/include/utils/thread_pool
          ${CMAKE_CURRENT_LIST_DIR}/include/utils/time
          ${CMAKE_CURRENT_LIST_DIR}/include/simulation
          ${CMAKE_CURRENT_LIST_DIR}/include/simulation/graph_executor
          ${CMAKE_CURRENT_LIST_DIR}/include/simulation/graph_executor/execution
          ${CMAKE_CURRENT_LIST_DIR}/include/simulation/signal
          ${CMAKE_CURRENT_LIST_DIR}/include/simulation/signal/sinks
          )

# The library is linked into shared modules (Python extension), so force PIC.
set_target_properties(ssp4sim_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_options(ssp4sim_lib PRIVATE -Wfatal-errors)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(ssp4sim_lib PUBLIC nlohmann_json::nlohmann_json)

find_package(TBB CONFIG REQUIRED)
target_link_libraries(ssp4sim_lib PRIVATE TBB::tbb TBB::tbbmalloc)

find_package(quill CONFIG REQUIRED)
target_link_libraries(ssp4sim_lib PRIVATE quill::quill)

find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(ssp4sim_lib PRIVATE unofficial::sqlite3::sqlite3)



target_link_libraries(ssp4sim_lib PRIVATE ssp4cpp::ssp4cpp)

target_link_libraries(ssp4sim_lib PUBLIC ssp4sim::3rd::fmi4c)

# install(
#   TARGETS ssp4sim_lib
#   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
# )
# 
# install(
#   DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/public_include/
#   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ssp4sim
# )
