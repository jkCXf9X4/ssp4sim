
file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")

# message(Sim app sources = "${ALL_SRC}")

add_executable(sim_app ${ALL_SRC})

target_compile_options(sim_app PRIVATE -Wfatal-errors)

target_link_libraries(sim_app PRIVATE ssp4sim_lib)
