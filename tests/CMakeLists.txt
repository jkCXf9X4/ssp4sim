   
    
file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")

add_executable(test_1 ${ALL_SRC})


target_link_libraries(test_1 PUBLIC ssp4sim_lib)

find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(test_1 PRIVATE Catch2::Catch2WithMain)

find_package(ssp4cpp CONFIG REQUIRED)
target_link_libraries(test_1 PRIVATE ssp4cpp::ssp4cpp)

find_package(cutecpp CONFIG REQUIRED)
target_link_libraries(test_1 PRIVATE cutecpp::cutecpp)

target_link_libraries(test_1 PUBLIC ssp4sim::3rd::fmi4c)

target_compile_definitions(test_1 PRIVATE SSP4SIM_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")


include(CTest)

enable_testing()
include(Catch)
catch_discover_tests(test_1)

add_test(NAME test_1 COMMAND $<TARGET_FILE:test_1>)
