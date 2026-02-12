set(SSP4SIM_PY_LIMITED_API "0x03090000" CACHE STRING "Py_LIMITED_API floor for abi3 builds")

find_package(Python REQUIRED COMPONENTS Interpreter Development)

set(PY_SSP4SIM_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/ssp4sim_python_api_abi3.c"
)

add_library(py_ssp4sim MODULE ${PY_SSP4SIM_SRC})
set_target_properties(
    py_ssp4sim
    PROPERTIES
        PREFIX ""
        SUFFIX ".abi3.so"
)
target_compile_definitions(py_ssp4sim PRIVATE Py_LIMITED_API=${SSP4SIM_PY_LIMITED_API})
target_link_options(py_ssp4sim PRIVATE -static-libstdc++ -static-libgcc)

if(TARGET Python::Module)
  target_link_libraries(py_ssp4sim PRIVATE Python::Module)
elseif(TARGET Python::Python)
  target_link_libraries(py_ssp4sim PRIVATE Python::Python)
else()
  target_include_directories(py_ssp4sim PRIVATE ${Python_INCLUDE_DIRS})
  target_link_libraries(py_ssp4sim PRIVATE ${Python_LIBRARIES})
endif()

target_link_libraries(py_ssp4sim PRIVATE ssp4sim_lib)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(py_ssp4sim PRIVATE nlohmann_json::nlohmann_json)

find_package(TBB CONFIG REQUIRED)
target_link_libraries(py_ssp4sim PRIVATE TBB::tbb TBB::tbbmalloc)

target_link_libraries(py_ssp4sim PRIVATE ssp4cpp::ssp4cpp)
target_link_libraries(py_ssp4sim PRIVATE ssp4sim::3rd::fmi4c)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")

set(PY_SSP4SIM_PKG_DIR "${CMAKE_CURRENT_BINARY_DIR}/pyssp4sim")

add_custom_command(
    TARGET py_ssp4sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${PY_SSP4SIM_PKG_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PY_SSP4SIM_PKG_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
            ${PY_SSP4SIM_PKG_DIR}/__init__.py
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:py_ssp4sim>
            ${PY_SSP4SIM_PKG_DIR}/$<TARGET_FILE_NAME:py_ssp4sim>
)

install(
  TARGETS py_ssp4sim
  LIBRARY DESTINATION python/pyssp4sim
  RUNTIME DESTINATION python/pyssp4sim
  ARCHIVE DESTINATION python/pyssp4sim
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
  DESTINATION python/pyssp4sim
)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/pyproject.toml
  DESTINATION python
)
