file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")

# message(Sim app sources = "${ALL_SRC}")

add_executable(sim_app ${ALL_SRC})

set(SSP4SIM_APP_VERSION "unknown")

find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE SSP4SIM_APP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE SSP4SIM_APP_VERSION_RESULT
  )

  if(NOT SSP4SIM_APP_VERSION_RESULT EQUAL 0 OR SSP4SIM_APP_VERSION STREQUAL "")
    set(SSP4SIM_APP_VERSION "unknown")
  endif()
endif()

target_compile_options(sim_app PRIVATE -Wfatal-errors)
target_compile_definitions(sim_app PRIVATE SSP4SIM_APP_VERSION="${SSP4SIM_APP_VERSION}")

target_link_libraries(sim_app PRIVATE ssp4sim_lib)


find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(sim_app PRIVATE nlohmann_json::nlohmann_json)

find_package(TBB CONFIG REQUIRED)
target_link_libraries(sim_app PRIVATE TBB::tbb TBB::tbbmalloc)

target_link_libraries(sim_app PRIVATE ssp4cpp::ssp4cpp)

target_link_libraries(sim_app PUBLIC ssp4sim::3rd::fmi4c)

install(
  TARGETS sim_app
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
