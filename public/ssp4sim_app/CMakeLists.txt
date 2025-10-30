
file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")

# message(Sim app sources = "${ALL_SRC}")

add_executable(sim_app ${ALL_SRC})

target_compile_options(sim_app PRIVATE -Wfatal-errors)

target_link_libraries(sim_app PRIVATE ssp4sim_lib)


find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(sim_app PRIVATE nlohmann_json::nlohmann_json)

find_package(TBB CONFIG REQUIRED)
target_link_libraries(sim_app PRIVATE TBB::tbb TBB::tbbmalloc)

target_link_libraries(sim_app PRIVATE ssp4cpp::ssp4cpp)

target_link_libraries(sim_app PRIVATE cutecpp::cutecpp)

target_link_libraries(sim_app PUBLIC ssp4sim::3rd::fmi4c)
