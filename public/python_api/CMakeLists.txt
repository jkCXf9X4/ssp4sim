
file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")

# message(ALL_SRC sim ="${ALL_SRC}")


find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG)

# pybind11 method:
pybind11_add_module(py_ssp4sim ${ALL_SRC})
target_link_options(py_ssp4sim PRIVATE -static-libstdc++ -static-libgcc)

target_link_libraries(py_ssp4sim PRIVATE ssp4sim_lib)


find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(py_ssp4sim PRIVATE nlohmann_json::nlohmann_json)

find_package(TBB CONFIG REQUIRED)
target_link_libraries(py_ssp4sim PRIVATE TBB::tbb TBB::tbbmalloc)

target_link_libraries(py_ssp4sim PRIVATE ssp4cpp::ssp4cpp)

target_link_libraries(py_ssp4sim PRIVATE ssp4sim::3rd::fmi4c)


file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/pyssp4sim")


add_custom_command(
    TARGET py_ssp4sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:py_ssp4sim>
            ${CMAKE_CURRENT_BINARY_DIR}/pyssp4sim/$<TARGET_FILE_NAME:py_ssp4sim>
)
