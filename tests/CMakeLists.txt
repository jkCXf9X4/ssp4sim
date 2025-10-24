   
    
file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")

add_executable(ssp4sim_tests ${ALL_SRC})


target_link_libraries(ssp4sim_tests PUBLIC ssp4sim_lib)

find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(ssp4sim_tests PRIVATE Catch2::Catch2WithMain)

target_link_libraries(ssp4sim_tests PRIVATE ssp4cpp::ssp4cpp)

target_link_libraries(ssp4sim_tests PRIVATE cutecpp::cutecpp)

target_link_libraries(ssp4sim_tests PUBLIC ssp4sim::3rd::fmi4c)

target_compile_definitions(ssp4sim_tests PRIVATE SSP4SIM_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")


include(CTest)

enable_testing()
include(Catch)
catch_discover_tests(ssp4sim_tests)

add_test(NAME ssp4sim_tests COMMAND $<TARGET_FILE:ssp4sim_tests>)
